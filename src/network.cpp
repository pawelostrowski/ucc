#include <string>		// std::string
#include <netdb.h>		// gethostbyname(), socket(), htons(), connect(), send(), recv()
#include <openssl/ssl.h>	// poza SSL zawiera include do plików nagłówkowych, które zawierają m.in. bzero(), memcpy()
#include <unistd.h>		// close() - socket

#include "network.hpp"
#include "window_utils.hpp"
#include "enc_str.hpp"
#include "debug.hpp"
#include "ucc_global.hpp"


int socket_init(struct global_args &ga, struct channel_irc *chan_parm[], std::string host, short port, std::string msg_dbg)
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
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# " + msg_dbg + ": Nie udało się pobrać informacji o hoście: " + host + " (sprawdź swoje połączenie internetowe).");
		return 0;
	}

	// utwórz deskryptor gniazda (socket)
	socketfd = socket(AF_INET, SOCK_STREAM, 0);	// SOCK_STREAM - TCP, SOCK_DGRAM - UDP

	if(socketfd == -1)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# " + msg_dbg + ": Nie udało się utworzyć deskryptora gniazda.");
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
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# " + msg_dbg + ": Nie udało się połączyć z hostem: " + host);
		return 0;
	}

	return socketfd;
}


bool http_get_cookies(struct global_args &ga, struct channel_irc *chan_parm[], char *http_recv_buf, std::string msg_dbg_http)
{
	size_t pos_cookie_start, pos_cookie_end;

	const std::string cookie_string = "Set-Cookie:";	// celowo bez spacji na końcu, bo każde cookie będzie dopisywane ze spacją na początku

	// znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
	pos_cookie_start = std::string(http_recv_buf).find(cookie_string);	// std::string(http_recv_buf) zamienia C string na std::string

	if(pos_cookie_start == std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# " + msg_dbg_http + ": Nie znaleziono żadnego cookie.");
		return false;
	}

	do
	{
		// szukaj ";" od pozycji początku cookie
		pos_cookie_end = std::string(http_recv_buf).find(";", pos_cookie_start);

		if(pos_cookie_end == std::string::npos)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Problem z cookie, brak wymaganego średnika na końcu.");
			return false;
		}

		// dopisz kolejne cookie do bufora
		ga.cookies.insert(ga.cookies.size(), std::string(http_recv_buf), pos_cookie_start + cookie_string.size(),
						pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

		// znajdź kolejne cookie
		pos_cookie_start = std::string(http_recv_buf).find(cookie_string, pos_cookie_start + cookie_string.size());

	} while(pos_cookie_start != std::string::npos);		// zakończ szukanie, gdy nie zostanie znalezione kolejne cookie

	return true;
}


char *http_get_data(struct global_args &ga, struct channel_irc *chan_parm[], std::string method, std::string host, short port, std::string stock,
		std::string content, std::string &cookies, bool get_cookies, long &http_recv_offset, std::string msg_dbg_http)
{
	if(method != "GET" && method != "POST")
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# " + msg_dbg_http + ": Nieobsługiwana metoda: " + method);
		return NULL;
	}

	int socketfd;					// deskryptor gniazda (socket)
	int bytes_sent, bytes_recv;			// liczba wysłanych i pobranych bajtów
	std::string data_send;				// dane do wysłania do hosta
	char *http_recv_buf = NULL;			// wskaźnik na bufor pobranych danych
	char http_recv_buf_tmp[RECV_BUF_TMP_SIZE];	// bufor tymczasowy pobranych danych
	bool first_recv = true;				// czy to pierwsze pobranie w pętli

	// utwórz gniazdo (socket) oraz połącz się z hostem
	socketfd = socket_init(ga, chan_parm, host, port, msg_dbg_http);

	if(socketfd == 0)
	{
		return NULL;
	}

	// utwórz dane do wysłania do hosta
	data_send = method + " " + stock + " HTTP/1.1\r\n"
			"Host: " + host + "\r\n"
			"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:" FF_VER ") Gecko/20100101 Firefox/" FF_VER "\r\n"
			"Accept-Language: pl\r\n";

	if(method == "POST")
	{
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
	http_recv_offset = 0;

	// połączenie na porcie różnym od 443 uruchomi transmisję nieszyfrowaną
	if(port != 443)
	{
		// wyślij dane do hosta
		bytes_sent = send(socketfd, data_send.c_str(), data_send.size(), 0);

		if(bytes_sent == -1)
		{
			close(socketfd);
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Nie udało się wysłać danych do hosta: " + host);
			return NULL;
		}

		else if(bytes_sent == 0)
		{
			close(socketfd);
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Podczas próby wysłania danych host " + host + " zakończył połączenie.");
			return NULL;
		}

		// sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
		else if(bytes_sent != static_cast<int>(data_send.size()))	// static_cast<int> - rzutowanie size_t (w tym przypadku) na int
		{
			close(socketfd);
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Nie udało się wysłać wszystkich danych do hosta: " + host);
			return NULL;
		}

		// poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie),
		// z wyjątkiem pierwszego obiegu
		do
		{
			// wstępnie zaalokuj 1500 bajtów na bufor (przy pierwszym obiegu pętli)
			if(http_recv_buf == NULL)
			{
				// reinterpret_cast<char *> - rzutowanie void* (w tym przypadku) na char*
				http_recv_buf = reinterpret_cast<char *>(malloc(RECV_BUF_TMP_SIZE));

				if(http_recv_buf == NULL)
				{
					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xRED "# " + msg_dbg_http + ": Błąd podczas alokacji pamięci przez malloc()");
					return NULL;
				}
			}

			// gdy danych do pobrania jest więcej, zwiększ rozmiar bufora
			else
			{
				http_recv_buf = reinterpret_cast<char *>(realloc(http_recv_buf, http_recv_offset + RECV_BUF_TMP_SIZE));

				if(http_recv_buf == NULL)
				{
					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xRED "# " + msg_dbg_http + ": Błąd podczas realokacji pamięci przez realloc()");
					return NULL;
				}
			}

			// pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
			bytes_recv = recv(socketfd, http_recv_buf_tmp, RECV_BUF_TMP_SIZE, 0);

			// sprawdź, czy pobieranie danych się powiodło
			if(bytes_recv == -1)
			{
				close(socketfd);
				free(http_recv_buf);	// w przypadku błędu zwolnij pamięć zajmowaną przez bufor
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
						xRED "# " + msg_dbg_http + ": Nie udało się pobrać danych z hosta: " + host);
				return NULL;
			}

			// sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
			else if(first_recv && bytes_recv == 0)
			{
				close(socketfd);
				free(http_recv_buf);
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
						xRED "# " + msg_dbg_http + ": Podczas próby pobrania danych host " + host + " zakończył połączenie.");
				return NULL;
			}

			first_recv = false;		// kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
			memcpy(http_recv_buf + http_recv_offset, http_recv_buf_tmp, bytes_recv);	// pobrane dane "dopisz" do bufora
			http_recv_offset += bytes_recv;	// zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)

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
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Błąd podczas tworzenia obiektu SSL_CTX");
			return NULL;
		}

		ssl_handle = SSL_new(ssl_context);

		if(ssl_handle == NULL)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Błąd podczas tworzenia struktury SSL.");
			return NULL;
		}

		if(! SSL_set_fd(ssl_handle, socketfd))
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Błąd podczas łączenia deskryptora SSL.");
			return NULL;
		}

		if(SSL_connect(ssl_handle) != 1)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Błąd podczas łączenia się z " + host + " przez SSL.");
			return NULL;
		}

		// wyślij dane do hosta
		bytes_sent = SSL_write(ssl_handle, data_send.c_str(), data_send.size());

		if(bytes_sent <= -1)
		{
			close(socketfd);
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Nie udało się wysłać danych do hosta: " + host + " (SSL).");
			return NULL;
		}

		else if(bytes_sent == 0)
		{
			close(socketfd);
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Podczas próby wysłania danych host " + host + " zakończył połączenie (SSL).");
			return NULL;
		}

		else if(bytes_sent != static_cast<int>(data_send.size()))
		{
			close(socketfd);
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xRED "# " + msg_dbg_http + ": Nie udało się wysłać wszystkich danych do hosta: " + host + " (SSL).");
			return NULL;
		}

		// pobierz odpowiedź
		do
		{
			if(http_recv_buf == NULL)
			{
				http_recv_buf = reinterpret_cast<char *>(malloc(RECV_BUF_TMP_SIZE));

				if(http_recv_buf == NULL)
				{
					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xRED "# " + msg_dbg_http + ": Błąd podczas alokacji pamięci przez malloc() (SSL).");
					return NULL;
				}
			}

			else
			{
				http_recv_buf = reinterpret_cast<char *>(realloc(http_recv_buf, http_recv_offset + RECV_BUF_TMP_SIZE));

				if(http_recv_buf == NULL)
				{
					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xRED "# " + msg_dbg_http + ": Błąd podczas realokacji pamięci przez realloc() (SSL).");
					return NULL;
				}
			}

			bytes_recv = SSL_read(ssl_handle, http_recv_buf_tmp, RECV_BUF_TMP_SIZE);

			if(bytes_recv <= -1)
			{
				close(socketfd);
				free(http_recv_buf);
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
						xRED "# " + msg_dbg_http + ": Nie udało się pobrać danych z hosta: " + host + " (SSL).");
				return NULL;
			}

			else if(first_recv && bytes_recv == 0)
			{
				close(socketfd);
				free(http_recv_buf);
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
						xRED "# " + msg_dbg_http + ": Podczas próby pobrania danych host " + host + " zakończył połączenie (SSL).");
				return NULL;
			}

			first_recv = false;
			memcpy(http_recv_buf + http_recv_offset, http_recv_buf_tmp, bytes_recv);
			http_recv_offset += bytes_recv;

		} while(bytes_recv > 0);

		// zamknij połączenie z hostem
		close(socketfd);

		SSL_shutdown(ssl_handle);
		SSL_free(ssl_handle);
		SSL_CTX_free(ssl_context);

	}	// else if(port == 443)

	// zakończ bufor kodem NULL
	http_recv_buf[http_recv_offset] = '\x00';

/*
	DBG HTTP START
*/
	http_dbg_to_file(ga, data_send, std::string(http_recv_buf), host, port, stock, msg_dbg_http);
/*
	DBG HTTP END
*/

	// jeśli trzeba, wyciągnij cookies z bufora
	if(get_cookies && ! http_get_cookies(ga, chan_parm, http_recv_buf, msg_dbg_http))
	{
		free(http_recv_buf);
		return NULL;
	}

	return http_recv_buf;	// zwróć wskaźnik do bufora pobranych danych
}


void irc_send(struct global_args &ga, struct channel_irc *chan_parm[], std::string irc_send_buf, std::string msg_dbg_irc)
{
	// do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)
	irc_send_buf += "\r\n";

/*
	DBG IRC START
*/
	irc_sent_dbg_to_file(ga, irc_send_buf);

	// debug w oknie
	if(ga.ucc_dbg_irc)
	{
		std::string irc_send_buf_dbg = buf_iso2utf(irc_send_buf);
		win_buf_add_str(ga, chan_parm, "Debug", xYELLOW + irc_send_buf_dbg.erase(irc_send_buf_dbg.size() - 1, 1));
	}
/*
	DBG IRC END
*/

	// wyślij dane do hosta
	int bytes_sent = send(ga.socketfd_irc, irc_send_buf.c_str(), irc_send_buf.size(), 0);

	if(bytes_sent == -1)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# " + msg_dbg_irc + "Nie udało się wysłać danych do serwera (IRC).");
	}

	else if(bytes_sent == 0)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# " + msg_dbg_irc + "Podczas próby wysłania danych serwer zakończył połączenie (IRC).");
	}

	else if(bytes_sent != static_cast<int>(irc_send_buf.size()))
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# " + msg_dbg_irc + "Nie udało się wysłać wszystkich danych do serwera (IRC).");
	}
}


void irc_recv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &irc_recv_buf, std::string msg_dbg_irc)
{
	char irc_recv_buf_tmp[RECV_BUF_TMP_SIZE];

	// pozycja oraz bufor pomocniczy do zachowania niepełnego fragmentu ostatniego wiersza, jeśli nie został pobrany w całości w jednej ramce
	size_t pos_incomplete;
	static std::string irc_recv_buf_incomplete;

	// pobierz dane od hosta
	int bytes_recv = recv(ga.socketfd_irc, irc_recv_buf_tmp, RECV_BUF_TMP_SIZE - 1, 0);	// - 1, aby zostawić miejsce na wstawienie NULL

	if(bytes_recv == -1)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# " + msg_dbg_irc + "Nie udało się pobrać danych z serwera (IRC).");
	}

	else if(bytes_recv == 0)
	{
		close(ga.socketfd_irc);
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# " + msg_dbg_irc + "Podczas próby pobrania danych serwer zakończył połączenie (IRC).");
	}

	// gdy nie było błędu
	else
	{
		// zakończ tymczasowy bufor kodem NULL
		irc_recv_buf_tmp[bytes_recv] = '\x00';

		// odebrane dane zwróć w buforze std::string
		irc_recv_buf.clear();
		irc_recv_buf = std::string(irc_recv_buf_tmp);

		// usuń 0x02 z bufora (występuje zaraz po zalogowaniu się do IRC w komunikacie powitalnym)
		while(irc_recv_buf.find("\x02") != std::string::npos)
		{
			irc_recv_buf.erase(irc_recv_buf.find("\x02"), 1);
		}

		// usuń \r z bufora (w ncurses wyświetlenie tego na Linuksie powoduje, że linia jest niewidoczna)
		while(irc_recv_buf.find("\r") != std::string::npos)
		{
			irc_recv_buf.erase(irc_recv_buf.find("\r"), 1);
		}

		// serwer wysyła dane w kodowaniu ISO-8859-2, zamień je na UTF-8
		irc_recv_buf = buf_iso2utf(irc_recv_buf);

		// dopisz do początku bufora głównego ewentualnie zachowany niepełny fragment poprzedniego wiersza z bufora pomocniczego
		if(irc_recv_buf_incomplete.size() > 0)
		{
			irc_recv_buf.insert(0, irc_recv_buf_incomplete);
		}

		// po (ewentualnym) przepisaniu wyczyść bufor pomocniczy
		irc_recv_buf_incomplete.clear();

		// wykryj, czy w buforze głównym jest niepełny wiersz (brak \r\n na końcu), jeśli tak, przenieś go do bufora pomocniczego
		// ( \r\n w domyśle, bo \r został usunięty z bufora podczas pobierania danych z serwera nieco wyżej)
		pos_incomplete = irc_recv_buf.rfind("\n");

		// - 1, bo pozycja jest liczona od zera, a długość jest całkowitą liczbą zajmowanych bajtów
		if(pos_incomplete != irc_recv_buf.size() - 1)
		{
			// zachowaj ostatni niepełny wiersz
			irc_recv_buf_incomplete.insert(0, irc_recv_buf, pos_incomplete + 1, irc_recv_buf.size() - pos_incomplete - 1);

			// oraz usuń go z głównego bufora
			irc_recv_buf.erase(pos_incomplete + 1, irc_recv_buf.size() - pos_incomplete - 1);
		}

/*
	DBG IRC START
*/
		irc_recv_dbg_to_file(ga, irc_recv_buf);

		// debug w oknie
		if(ga.ucc_dbg_irc)
		{
			std::string irc_recv_buf_dbg = irc_recv_buf;
			win_buf_add_str(ga, chan_parm, "Debug", xWHITE + irc_recv_buf_dbg.erase(irc_recv_buf_dbg.size() - 1, 1));
		}
/*
	DBG IRC END
*/
	}
}
