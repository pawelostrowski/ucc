#include <string>		// std::string
#include <netdb.h>		// gethostbyname(), socket(), htons(), connect(), send(), recv()
#include <openssl/ssl.h>	// poza SSL zawiera include do plików nagłówkowych, które zawierają m.in. bzero(), malloc(), realloc(), memcpy()
#include <unistd.h>		// close() - socket

#include "network.hpp"
#include "window_utils.hpp"
#include "enc_str.hpp"
#include "ucc_global.hpp"

#define BUF_SIZE (1500 * sizeof(char))

#if DBG_HTTP == 1
#include <fstream>		// std::fstream
#define FILE_DBG_HTTP "/tmp/ucc_dbg_http.log"
#endif		// DBG_HTTP


bool find_cookies(char *buffer_recv, std::string &cookies, std::string &msg_err)
{
	size_t pos_cookie_start, pos_cookie_end;
	std::string cookie_string;

	cookie_string = "Set-Cookie:";		// celowo bez spacji na końcu, bo każde cookie będzie dopisywane ze spacją na początku

	// znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
	pos_cookie_start = std::string(buffer_recv).find(cookie_string);	// std::string(buffer_recv) zamienia C string na std::string
	if(pos_cookie_start == std::string::npos)
	{
		msg_err = "Nie znaleziono żadnego cookie.";
		return false;
	}

	do
	{
		// szukaj ";" od pozycji początku cookie
		pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);
		if(pos_cookie_end == std::string::npos)
		{
			msg_err = "Problem z cookie, brak wymaganego średnika na końcu.";
			return false;
		}

		// dopisz kolejne cookie do bufora
		cookies.insert(cookies.size(), std::string(buffer_recv), pos_cookie_start + cookie_string.size(),
						pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

		// znajdź kolejne cookie
		pos_cookie_start = std::string(buffer_recv).find(cookie_string, pos_cookie_start + cookie_string.size());

	} while(pos_cookie_start != std::string::npos);		// zakończ szukanie, gdy nie znaleziono kolejnego cookie

	return true;
}


int socket_init(std::string host, short port, std::string &msg_err)
{
/*
	Utwórz gniazdo (socket) oraz połącz się z hostem.
*/

	int socketfd;

	struct hostent *host_info;
	struct sockaddr_in serv_info;

	// pobierz adres IP hosta
	host_info = gethostbyname(host.c_str());
	if(host_info == NULL)
	{
		msg_err = "Nie udało się pobrać informacji o hoście: " + host + " (sprawdź swoje połączenie internetowe).";
		return 0;
	}

	// utwórz deskryptor gniazda (socket)
	socketfd = socket(AF_INET, SOCK_STREAM, 0);	// SOCK_STREAM - TCP, SOCK_DGRAM - UDP
	if(socketfd == -1)
	{
		msg_err = "Nie udało się utworzyć deskryptora gniazda.";
		return 0;
	}

	serv_info.sin_family = AF_INET;
	serv_info.sin_port = htons(port);
	serv_info.sin_addr = *((struct in_addr *) host_info->h_addr);
	bzero(&(serv_info.sin_zero), 8);

	// połącz z hostem
	if(connect(socketfd, (struct sockaddr *) &serv_info, sizeof(struct sockaddr)) == -1)
	{
		close(socketfd);
		msg_err = "Nie udało się połączyć z hostem: " + host;
		return 0;
	}

	return socketfd;
}


char *http_get_data(std::string method, std::string host, short port, std::string stock, std::string content, std::string &cookies, bool get_cookies,
                    long &offset_recv, std::string &msg_err, std::string msg_dbg_http)
{
	if(method != "GET" && method != "POST")
	{
		msg_err = "Nieobsługiwana metoda: " + method;
		return NULL;
	}

	int socketfd;			// deskryptor gniazda (socket)
	int bytes_sent, bytes_recv;
	char *buffer_recv = NULL;
	char buffer_tmp[BUF_SIZE];	// bufor tymczasowy pobranych danych
	bool first_recv = true;		// czy to pierwsze pobranie w pętli
	std::string data_send;		// dane do wysłania do hosta

	// utwórz gniazdo (socket) oraz połącz się z hostem
	socketfd = socket_init(host, port, msg_err);
	if(socketfd == 0)
	{
		return NULL;	// zwróć komunikat błędu w msg_err
	}

	// utwórz dane do wysłania do hosta
	data_send =  method + " " + stock + " HTTP/1.1\r\n"
		    "Host: " + host + "\r\n"
		    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:29.0) Gecko/20100101 Firefox/29.0\r\n"
		    "Accept-Language: pl\r\n";

	if(method == "POST")
	{
		// użyte std::to_string(content.size())  <--- int na std::string wg standardu C++11
		data_send += "Content-Type: application/x-www-form-urlencoded\r\n"
			     "Content-Length: " + std::to_string(content.size()) + "\r\n";
	}

	if(cookies.size() > 0)
	{
		data_send += "Cookie:" + cookies + "\r\n";
	}

	data_send += "Connection: close\r\n\r\n";

	if(content.size() > 0)
	{
		data_send += content;
	}

	// offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
	offset_recv = 0;

	// połączenie na porcie różnym od 443 uruchomi transmisję nieszyfrowaną
	if(port != 443)
	{
		// wyślij dane do hosta
		bytes_sent = send(socketfd, data_send.c_str(), data_send.size(), 0);
		if(bytes_sent == -1)
		{
			close(socketfd);
			msg_err = "Nie udało się wysłać danych do hosta: " + host;
			return NULL;
		}

		if(bytes_sent == 0)
		{
			close(socketfd);
			msg_err = "Podczas próby wysłania danych host " + host + " zakończył połączenie.";
			return NULL;
		}

		// sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
		if(bytes_sent != static_cast<int>(data_send.size()))	// static_cast<int> - rzutowanie size_t (w tym przypadku) na int
		{
			close(socketfd);
			msg_err = "Nie udało się wysłać wszystkich danych do hosta: " + host;
			return NULL;
		}

		// poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie),
		// z wyjątkiem pierwszego obiegu
		do
		{
			// wstępnie zaalokuj 1500 bajtów na bufor (przy pierwszym obiegu pętli)
			if(buffer_recv == NULL)
			{
				buffer_recv = reinterpret_cast<char *>(malloc(BUF_SIZE));	// reinterpret_cast<char *> - rzutowanie void*
												// (w tym przypadku) na char*

				if(buffer_recv == NULL)
				{
					msg_err = "Błąd podczas alokacji pamięci przez malloc()";
					return NULL;
				}
			}

			// gdy danych do pobrania jest więcej, zwiększ rozmiar bufora
			else
			{
				buffer_recv = reinterpret_cast<char *>(realloc(buffer_recv, offset_recv + BUF_SIZE));
				if(buffer_recv == NULL)
				{
					msg_err = "Błąd podczas realokacji pamięci przez realloc()";
					return NULL;
				}
			}

			// pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
			bytes_recv = recv(socketfd, buffer_tmp, BUF_SIZE, 0);

			// sprawdź, czy pobieranie danych się powiodło
			if(bytes_recv == -1)
			{
				close(socketfd);
				free(buffer_recv);	// w przypadku błędu zwolnij pamięć zajmowaną przez bufor
				msg_err = "Nie udało się pobrać danych z hosta: " + host;
				return NULL;
			}

			// sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
			if(first_recv)
			{
				if(bytes_recv == 0)
				{
					close(socketfd);
					free(buffer_recv);
					msg_err = "Podczas próby pobrania danych host " + host + " zakończył połączenie.";
					return NULL;
				}
			}

			first_recv = false;		// kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
			memcpy(buffer_recv + offset_recv, buffer_tmp, bytes_recv);	// pobrane dane "dopisz" do bufora
			offset_recv += bytes_recv;	// zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)

		} while(bytes_recv > 0);

		// zamknij połączenie z hostem
		close(socketfd);

	}	// if(port != 443)

	// połączenie na porcie 443 uruchomi transmisję szyfrowaną (SSL)
	else if(port == 443)
	{
		SSL_CTX *ssl_context;
		SSL *ssl_handle;

		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();

		ssl_context = SSL_CTX_new(SSLv23_client_method());
		if(ssl_context == NULL)
		{
			msg_err = "Błąd podczas tworzenia obiektu SSL_CTX";
			return NULL;
		}

		ssl_handle = SSL_new(ssl_context);
		if(ssl_handle == NULL)
		{
			msg_err = "Błąd podczas tworzenia struktury SSL";
			return NULL;
		}

		if(! SSL_set_fd(ssl_handle, socketfd))
		{
			msg_err = "Błąd podczas łączenia desktyptora SSL";
			return NULL;
		}

		if(SSL_connect(ssl_handle) != 1)
		{
			msg_err = "Błąd podczas łączenia się z " + host + " przez SSL";
			return NULL;
		}

		// wyślij dane do hosta
		bytes_sent = SSL_write(ssl_handle, data_send.c_str(), data_send.size());
		if(bytes_sent <= -1)
		{
			close(socketfd);
			msg_err = "Nie udało się wysłać danych do hosta: " + host + " [SSL]";
			return NULL;
		}
		if(bytes_sent == 0)
		{
			close(socketfd);
			msg_err = "Podczas próby wysłania danych host " + host + " zakończył połączenie [SSL]";
			return NULL;
		}
		if(bytes_sent != static_cast<int>(data_send.size()))
		{
			close(socketfd);
			msg_err = "Nie udało się wysłać wszystkich danych do hosta: " + host + " [SSL]";
			return NULL;
		}

		// pobierz odpowiedź
		do
		{
			if(buffer_recv == NULL)
			{
				buffer_recv = reinterpret_cast<char *>(malloc(BUF_SIZE));
				if(buffer_recv == NULL)
				{
					msg_err = "Błąd podczas alokacji pamięci przez malloc() [SSL]";
					return NULL;
				}
			}

			else
			{
				buffer_recv = reinterpret_cast<char *>(realloc(buffer_recv, offset_recv + BUF_SIZE));
				if(buffer_recv == NULL)
				{
					msg_err = "Błąd podczas realokacji pamięci przez realloc() [SSL]";
					return NULL;
				}
			}

			bytes_recv = SSL_read(ssl_handle, buffer_tmp, BUF_SIZE);
			if(bytes_recv <= -1)
			{
				close(socketfd);
				free(buffer_recv);
				msg_err = "Nie udało się pobrać danych z hosta: " + host + " [SSL]";
				return NULL;
			}

			if(first_recv)
			{
				if(bytes_recv == 0)
				{
					close(socketfd);
					free(buffer_recv);
					msg_err = "Podczas próby pobrania danych host " + host + " zakończył połączenie [SSL]";
					return NULL;
				}
			}

			first_recv = false;
			memcpy(buffer_recv + offset_recv, buffer_tmp, bytes_recv);
			offset_recv += bytes_recv;

		} while(bytes_recv > 0);

		// zamknij połączenie z hostem
		close(socketfd);

		SSL_shutdown(ssl_handle);
		SSL_free(ssl_handle);
		SSL_CTX_free(ssl_context);

	}	// else if(port == 443)

	// zakończ bufor kodem NULL
	buffer_recv[offset_recv] = '\0';

/*
	Jeśli trzeba, zapisz wysłane i pobrane dane do pliku. Przydatne podczas debugowania.
*/

#if DBG_HTTP == 1

	static bool dbg_first_save = true;	// pierwszy zapis nadpisuje starą zawartość pliku
	std::string data_sent = data_send;
	std::string data_recv = std::string(buffer_recv);

	std::fstream file_dbg;

	if(dbg_first_save)
	{
		file_dbg.open(FILE_DBG_HTTP, std::ios::out);
	}
	else
	{
		file_dbg.open(FILE_DBG_HTTP, std::ios::app | std::ios::out);
	}

	dbg_first_save = false;			// kolejne zapisy dopisują dane do pliku

	if(file_dbg.good() == false)
	{
		free(buffer_recv);
		msg_err = "Błąd podczas dostępu do pliku debugowania HTTP (" FILE_DBG_HTTP "), sprawdź uprawnienia do zapisu.";
		return NULL;
	}

	// jeśli wysyłane było hasło, ukryj je oraz długość zapytania (zdradza długość hasła)
	size_t exp_start, exp_end;
	std::string pwd_str = "haslo=";
	std::string con_str = "Content-Length: ";

	exp_start = data_sent.find(pwd_str);
	if(exp_start != std::string::npos)
	{
		exp_end = data_sent.find("&", exp_start);
		if(exp_end != std::string::npos)
		{
			data_sent.erase(exp_start + pwd_str.size(), exp_end - exp_start - pwd_str.size());
			data_sent.insert(exp_start + pwd_str.size(), "[hidden]");

			// było hasło, więc ukryj długość zapytania
			exp_start = data_sent.find(con_str);
			if(exp_start != std::string::npos)
			{
				exp_end = data_sent.find("\r\n", exp_start);
				if(exp_end != std::string::npos)
				{
					data_sent.erase(exp_start + con_str.size(), exp_end - exp_start - con_str.size());
					data_sent.insert(exp_start + con_str.size(), "[hidden]");
				}
			}

		}

	}

	// jeśli pobrano obrazek, zakończ string za wyrażeniem GIFxxx
	std::string gif_str = "GIF";

	exp_start = data_recv.find(gif_str);
	if(exp_start != std::string::npos)
	{
		data_recv.erase(exp_start + gif_str.size() + 3, data_recv.size() - exp_start - gif_str.size() - 3);
	}

	// usuń \r z buforów, aby w pliku nie było go
	while(data_sent.find("\r") != std::string::npos)
	{
		data_sent.erase(data_sent.find("\r"), 1);
	}

	while(data_recv.find("\r") != std::string::npos)
	{
		data_recv.erase(data_recv.find("\r"), 1);
	}

	// zapisz dane
	std::string s;

	if(port == 443)
	{
		s = "s";
	}

	file_dbg << "================================================================================\n";

	file_dbg << msg_dbg_http + "\n\n\n";

	file_dbg << ">>> SENT (http" + s + "://" + host + stock + "):\n\n";

	file_dbg << data_sent + "\n";

	if(data_sent[data_sent.size() - 1] != '\n')
	{
		file_dbg << "\n\n";
	}

	file_dbg << ">>> RECV:\n\n";

	file_dbg << data_recv + "\n";

	if(data_recv[data_recv.size() - 1] != '\n')
	{
		file_dbg << "\n\n";
	}

	file_dbg.close();

#endif		// DBG_HTTP

/*
	Koniec części odpowiedzialnej za zapis do pliku.
*/

	// jeśli trzeba, wyciągnij cookies z bufora
	if(get_cookies)
	{
		if(! find_cookies(buffer_recv, cookies, msg_err))
		{
			free(buffer_recv);
			return NULL;	// zwróć komunikat błędu w msg_err
		}
	}

	return buffer_recv;	// zwróć wskaźnik do bufora pobranych danych
}


void irc_send(struct global_args &ga, struct channel_irc *chan_parm[], std::string buffer_irc_send)
{
	int bytes_sent;

	// do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)
	buffer_irc_send += "\r\n";

/*
	DBG IRC START
*/
	std::string data_sent = buffer_irc_send;
	data_sent = buf_iso2utf(data_sent);	// zapis w UTF-8
	std::fstream file_dbg;

	while(data_sent.find("\r") != std::string::npos)
	{
		data_sent.erase(data_sent.find("\r"), 1);
	}

	data_sent.insert(0, ">>> ");

	file_dbg.open("/tmp/ucc_dbg_irc.log", std::ios::app | std::ios::out);

	file_dbg << data_sent;

	file_dbg.close();
/*
	DBG IRC END
*/

/*
	// debug w oknie
	if(ga.ucc_dbg_irc)
	{
		std::string buffer_irc_send_dbg = buffer_irc_send;
		win_buf_add_str(ga, chan_parm, "Debug", xYELLOW + buffer_irc_send_dbg.erase(buffer_irc_send_dbg.size() - 1, 1));
	}
*/

	// wyślij dane do hosta
	bytes_sent = send(ga.socketfd_irc, buffer_irc_send.c_str(), buffer_irc_send.size(), 0);

	if(bytes_sent == -1)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				xRED "# Nie udało się wysłać danych do serwera, rozłączono [IRC]");
	}

	else if(bytes_sent == 0)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				xRED "# Podczas próby wysłania danych serwer zakończył połączenie [IRC]");
	}

	else if(bytes_sent != static_cast<int>(buffer_irc_send.size()))
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				xRED "# Nie udało się wysłać wszystkich danych do serwera, rozłączono [IRC]");
	}
}


void irc_recv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_recv)
{
	int bytes_recv;
	char buffer_tmp[BUF_SIZE];

	// pozycja oraz bufor pomocniczy do zachowania niepełnego fragmentu ostatniego wiersza, jeśli nie został pobrany w całości w jednej ramce
	size_t pos_incomplete;
	static std::string buffer_irc_recv_incomplete;

	// pobierz dane od hosta
	bytes_recv = recv(ga.socketfd_irc, buffer_tmp, BUF_SIZE - 1, 0);
	buffer_tmp[bytes_recv] = '\x00';

	if(bytes_recv == -1)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "# Nie udało się pobrać danych z serwera, rozłączono [IRC]");
		return;
	}

	else if(bytes_recv == 0)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "# Podczas próby pobrania danych serwer zakończył połączenie [IRC]");
		return;
	}

	// odebrane dane zwróć w buforze std::string
	buffer_irc_recv.clear();
	buffer_irc_recv = std::string(buffer_tmp);

	// usuń 0x02 z bufora (występuje zaraz po zalogowaniu się do IRC w komunikacie powitalnym)
	while(buffer_irc_recv.find("\x02") != std::string::npos)
	{
		buffer_irc_recv.erase(buffer_irc_recv.find("\x02"), 1);
	}

	// usuń \r z bufora (w ncurses wyświetlenie tego na Linuksie powoduje, że linia jest niewidoczna)
	while(buffer_irc_recv.find("\r") != std::string::npos)
	{
		buffer_irc_recv.erase(buffer_irc_recv.find("\r"), 1);
	}

	// serwer wysyła dane w kodowaniu ISO-8859-2, zamień je na UTF-8
	buffer_irc_recv = buf_iso2utf(buffer_irc_recv);

/*
	DBG IRC START
*/
	std::fstream file_dbg;

	file_dbg.open("/tmp/ucc_dbg_irc.log", std::ios::app | std::ios::out);

//		file_dbg << "<---new--->";
	file_dbg << buffer_irc_recv;

	file_dbg.close();
/*
	DBG IRC END
*/

	// dopisz do początku bufora głównego ewentualnie zachowany niepełny fragment poprzedniego wiersza z bufora pomocniczego
	if(buffer_irc_recv_incomplete.size() > 0)
	{
		buffer_irc_recv.insert(0, buffer_irc_recv_incomplete);
	}

	// po (ewentualnym) przepisaniu wyczyść bufor pomocniczy
	buffer_irc_recv_incomplete.clear();

	// wykryj, czy w buforze głównym jest niepełny wiersz (brak \r\n na końcu), jeśli tak, przenieś go do bufora pomocniczego
	pos_incomplete = buffer_irc_recv.rfind("\n");	// \r\n w domyśle, bo \r został usunięty z bufora podczas pobierania danych z serwera nieco wyżej

	if(pos_incomplete != buffer_irc_recv.size() - 1)	// - 1, bo pozycja jest liczona od zera, a długość jest całkowitą liczbą zajmowanych bajtów
	{
		// zachowaj ostatni niepełny wiersz
		buffer_irc_recv_incomplete.insert(0, buffer_irc_recv, pos_incomplete + 1, buffer_irc_recv.size() - pos_incomplete - 1);

		// oraz usuń go z głównego bufora
		buffer_irc_recv.erase(pos_incomplete + 1, buffer_irc_recv.size() - pos_incomplete - 1);
	}

/*
	// debug w oknie
	if(ga.ucc_dbg_irc)
	{
		std::string buffer_irc_rec_dbg = buffer_irc_recv;
		win_buf_add_str(ga, chan_parm, "Debug", xWHITE + buffer_irc_rec_dbg.erase(buffer_irc_rec_dbg.size() - 1, 1));
	}
*/
}
