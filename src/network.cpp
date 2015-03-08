/*
	Ucieszony Chat Client
	Copyright (C) 2013-2015 Paweł Ostrowski

	This file is part of Ucieszony Chat Client.

	Ucieszony Chat Client is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	Ucieszony Chat Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Ucieszony Chat Client (in the file LICENSE); if not,
	see <http://www.gnu.org/licenses/gpl-2.0.html>.
*/


#include <sstream>		// std::string, std::stringstream
#include <cstring>		// std::memset(), std::memcpy()
#include <netdb.h>		// gethostbyname(), socket(), htons(), connect(), send(), recv()
#include <unistd.h>		// close() - socket
#include <openssl/ssl.h>	// OpenSSL

// -std=c++11 - std::malloc(), std::realloc()

#include "network.hpp"
#include "window_utils.hpp"
#include "enc_str.hpp"
#include "debug.hpp"
#include "ucc_global.hpp"


int socket_init(std::string host, uint16_t port, std::string &msg_err)
{
/*
	Utwórz gniazdo (socket) oraz połącz się z hostem.
*/

	struct hostent *host_info;

	// pobierz adres IP hosta
	host_info = gethostbyname(host.c_str());

	if(host_info == NULL)
	{
		msg_err = "Nie udało się pobrać informacji o hoście: " + host + " (sprawdź swoje połączenie internetowe).";

		return 0;
	}

	// utwórz deskryptor gniazda (socket)
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);		// SOCK_STREAM - TCP, SOCK_DGRAM - UDP

	if(socketfd == -1)
	{
		msg_err = "Nie udało się utworzyć deskryptora gniazda.";

		return 0;
	}

	struct sockaddr_in serv_info;

	serv_info.sin_family = AF_INET;
	serv_info.sin_port = htons(port);
	serv_info.sin_addr = *((struct in_addr *) host_info->h_addr);
	std::memset(&(serv_info.sin_zero), 0, 8);

	// połącz z hostem
	if(connect(socketfd, (struct sockaddr *) &serv_info, sizeof(struct sockaddr)) == -1)
	{
		msg_err = "Nie udało się połączyć z hostem: " + host;

		close(socketfd);

		return 0;
	}

	return socketfd;
}


bool http_get_cookies(std::string http_recv_buf_str, std::vector<std::string> &cookies, std::string &msg_err)
{
	std::string cookie;

	const std::string cookie_string = "Set-Cookie: ";

	// znajdź pozycję pierwszego cookie (od miejsca: "Set-Cookie: ")
	size_t pos_cookie_start = http_recv_buf_str.find(cookie_string);

	if(pos_cookie_start == std::string::npos)
	{
		msg_err = "Nie znaleziono żadnego cookie.";

		return false;
	}

	size_t pos_cookie_end;

	do
	{
		// szukaj ";" od pozycji początku cookie
		pos_cookie_end = http_recv_buf_str.find(";", pos_cookie_start);

		if(pos_cookie_end == std::string::npos)
		{
			msg_err = "Problem z cookie, brak wymaganego średnika na końcu.";

			return false;
		}

		cookie.clear();

		// wstaw cookie do bufora pomocniczego (+ 1, by dopisać też średnik)
		cookie.insert(0, http_recv_buf_str, pos_cookie_start + cookie_string.size(), pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

		// dopisz cookie do bufora głównego, pomiń onet_sgn
		if(cookie.find("onet_sgn") == std::string::npos)
		{
			cookies.push_back(cookie);
		}

		// znajdź kolejne cookie
		pos_cookie_start = http_recv_buf_str.find(cookie_string, pos_cookie_start + cookie_string.size());

	} while(pos_cookie_start != std::string::npos);		// zakończ szukanie, gdy nie zostanie znalezione kolejne cookie

	return true;
}


char *http_get_data(struct global_args &ga, std::string method, std::string host, uint16_t port, std::string stock,
	std::string content, bool get_cookies, int &bytes_recv_all, std::string &msg_err)
{
	if(method != "GET" && method != "POST")
	{
		msg_err = "Nieobsługiwana metoda: " + method;

		return NULL;
	}

	// utwórz deskryptor gniazda (socket) oraz połącz się z hostem
	int socketfd = socket_init(host, port, msg_err);

	if(socketfd == 0)
	{
		// opis błędu jest w msg_err

		return NULL;
	}

	int bytes_sent, bytes_recv;			// liczba wysłanych i pobranych bajtów
	char *http_recv_buf_ptr = NULL;			// wskaźnik na bufor pobranych danych
	char http_recv_buf_tmp[RECV_BUF_TMP_SIZE];	// bufor tymczasowy pobranych danych
	bool first_recv = true;				// czy to pierwsze pobranie w pętli

	// utwórz dane do wysłania do hosta
	std::string data_send = method + " " + stock + " HTTP/1.1\r\n"
				"Host: " + host + "\r\n"
				"User-Agent: " BROWSER_USER_AGENT "\r\n"
				"Accept-Language: pl\r\n";

	if(method == "POST")
	{
		data_send += "Content-Type: application/x-www-form-urlencoded\r\n"
			     "Content-Length: " + std::to_string(content.size()) + "\r\n";
	}

	if(ga.cookies.size() > 0)
	{
		data_send += "Cookie:";

		int cookies_len = ga.cookies.size();

		for(int i = 0; i < cookies_len; ++i)
		{
			data_send += " " + ga.cookies[i];
		}

		data_send += "\r\n";
	}

	data_send += "Connection: close\r\n\r\n";

	if(content.size() > 0)
	{
		data_send += content;
	}

	// początkowo wyzeruj całkowity rozmiar pobranych danych (istotne do określenia później rozmiaru pobranych danych)
	bytes_recv_all = 0;

	// rozmiar danych do wysłania do hosta
	int data_send_len = data_send.size();

	// połączenie na porcie różnym od 443 uruchomi transmisję nieszyfrowaną
	if(port != 443)
	{
		// wyślij dane do hosta
		bytes_sent = send(socketfd, data_send.c_str(), data_send_len, 0);

		if(bytes_sent == -1)
		{
			msg_err = "Nie udało się wysłać danych do hosta: " + host;
		}

		else if(bytes_sent == 0)
		{
			msg_err = "Podczas próby wysłania danych host " + host + " zakończył połączenie.";
		}

		// sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
		else if(bytes_sent != data_send_len)
		{
			msg_err = "Nie udało się wysłać wszystkich danych do hosta: " + host;
		}

		// w przypadku błędu podczas wysyłania danych do hosta, zakończ
		if(bytes_sent != data_send_len)
		{
			close(socketfd);

			return NULL;
		}

		// poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie),
		// z wyjątkiem pierwszego obiegu
		do
		{
			// wstępnie zaalokuj tyle bajtów na bufor (przy pierwszym obiegu pętli), ile wskazuje RECV_BUF_TMP_SIZE
			if(http_recv_buf_ptr == NULL)
			{
				http_recv_buf_ptr = reinterpret_cast<char *>(std::malloc(RECV_BUF_TMP_SIZE));

				if(http_recv_buf_ptr == NULL)
				{
					msg_err = "Błąd podczas alokacji pamięci przez std::malloc()";
				}
			}

			// gdy danych do pobrania jest więcej, zwiększ rozmiar bufora
			else
			{
				http_recv_buf_ptr = reinterpret_cast<char *>(std::realloc(http_recv_buf_ptr, bytes_recv_all + RECV_BUF_TMP_SIZE));

				if(http_recv_buf_ptr == NULL)
				{
					msg_err = "Błąd podczas realokacji pamięci przez std::realloc()";
				}
			}

			// jeśli malloc() lub realloc() zwróciły NULL, zakończ
			if(http_recv_buf_ptr == NULL)
			{
				close(socketfd);

				return NULL;
			}

			// pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
			bytes_recv = recv(socketfd, http_recv_buf_tmp, RECV_BUF_TMP_SIZE - 1, 0);

			// sprawdź, czy pobieranie danych się powiodło
			if(bytes_recv == -1)
			{
				msg_err = "Nie udało się pobrać danych z hosta: " + host;
			}

			// sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
			else if(first_recv && bytes_recv == 0)
			{
				msg_err = "Podczas próby pobrania danych host " + host + " zakończył połączenie.";
			}

			// w przypadku błędu podczas pobierania danych od hosta, zakończ
			if(bytes_recv == -1 || (first_recv && bytes_recv == 0))
			{
				close(socketfd);

				std::free(http_recv_buf_ptr);	// w przypadku błędu zwolnij pamięć zajmowaną przez bufor

				return NULL;
			}

			first_recv = false;		// kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych

			std::memcpy(http_recv_buf_ptr + bytes_recv_all, http_recv_buf_tmp, bytes_recv);	// pobrane dane dopisz do bufora

			bytes_recv_all += bytes_recv;	// zwiększ całkowity rozmiar pobranych danych (sumaryczny, a nie w jednym obiegu pętli)

		} while(bytes_recv > 0);

		// zamknij połączenie z hostem
		close(socketfd);
	}

	// połączenie na porcie 443 uruchomi transmisję szyfrowaną
	else
	{
		SSL_CTX *ssl_context;
		SSL *ssl_handle;

		SSL_library_init();

		ssl_context = SSL_CTX_new(SSLv23_client_method());
		SSL_CTX_set_options(ssl_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);	// wyklucz użycie SSLv2 i SSLv3

		if(ssl_context == NULL)
		{
			msg_err = "Błąd podczas tworzenia obiektu SSL_CTX";

			return NULL;
		}

		ssl_handle = SSL_new(ssl_context);

		if(ssl_handle == NULL)
		{
			msg_err = "Błąd podczas tworzenia struktury SSL";

			SSL_CTX_free(ssl_context);

			return NULL;
		}

		if(! SSL_set_fd(ssl_handle, socketfd))
		{
			msg_err = "Błąd podczas łączenia deskryptora SSL";

			SSL_free(ssl_handle);
			SSL_CTX_free(ssl_context);

			return NULL;
		}

		if(SSL_connect(ssl_handle) != 1)
		{
			msg_err = "Błąd podczas łączenia się z " + host + " [SSL]";

			close(socketfd);

			SSL_shutdown(ssl_handle);
			SSL_free(ssl_handle);
			SSL_CTX_free(ssl_context);

			return NULL;
		}

		// wyślij dane do hosta
		bytes_sent = SSL_write(ssl_handle, data_send.c_str(), data_send_len);

		if(bytes_sent <= -1)
		{
			msg_err = "Nie udało się wysłać danych do hosta: " + host + " [SSL]";
		}

		else if(bytes_sent == 0)
		{
			msg_err = "Podczas próby wysłania danych host " + host + " zakończył połączenie. [SSL]";
		}

		else if(bytes_sent != data_send_len)
		{
			msg_err = "Nie udało się wysłać wszystkich danych do hosta: " + host + " [SSL]";
		}

		// w przypadku błędu podczas wysyłania danych do hosta, zakończ
		if(bytes_sent != data_send_len)
		{
			close(socketfd);

			SSL_shutdown(ssl_handle);
			SSL_free(ssl_handle);
			SSL_CTX_free(ssl_context);

			return NULL;
		}

		// pobierz odpowiedź
		do
		{
			if(http_recv_buf_ptr == NULL)
			{
				http_recv_buf_ptr = reinterpret_cast<char *>(std::malloc(RECV_BUF_TMP_SIZE));

				if(http_recv_buf_ptr == NULL)
				{
					msg_err = "Błąd podczas alokacji pamięci przez std::malloc() [SSL]";
				}
			}

			else
			{
				http_recv_buf_ptr = reinterpret_cast<char *>(std::realloc(http_recv_buf_ptr, bytes_recv_all + RECV_BUF_TMP_SIZE));

				if(http_recv_buf_ptr == NULL)
				{
					msg_err = "Błąd podczas realokacji pamięci przez std::realloc() [SSL]";
				}
			}

			// jeśli malloc() lub realloc() zwróciły NULL, zakończ
			if(http_recv_buf_ptr == NULL)
			{
				close(socketfd);

				SSL_shutdown(ssl_handle);
				SSL_free(ssl_handle);
				SSL_CTX_free(ssl_context);

				return NULL;
			}

			bytes_recv = SSL_read(ssl_handle, http_recv_buf_tmp, RECV_BUF_TMP_SIZE - 1);

			if(bytes_recv <= -1)
			{
				msg_err = "Nie udało się pobrać danych z hosta: " + host + " [SSL]";
			}

			else if(first_recv && bytes_recv == 0)
			{
				msg_err = "Podczas próby pobrania danych host " + host + " zakończył połączenie. [SSL]";
			}

			// w przypadku błędu podczas pobierania danych od hosta, zakończ
			if(bytes_recv <= -1 || (first_recv && bytes_recv == 0))
			{
				close(socketfd);

				SSL_shutdown(ssl_handle);
				SSL_free(ssl_handle);
				SSL_CTX_free(ssl_context);

				std::free(http_recv_buf_ptr);

				return NULL;
			}

			first_recv = false;

			std::memcpy(http_recv_buf_ptr + bytes_recv_all, http_recv_buf_tmp, bytes_recv);

			bytes_recv_all += bytes_recv;

		} while(bytes_recv > 0);

		// zamknij połączenie z hostem
		close(socketfd);

		SSL_shutdown(ssl_handle);
		SSL_free(ssl_handle);
		SSL_CTX_free(ssl_context);
	}

	// zakończ bufor kodem NULL
	http_recv_buf_ptr[bytes_recv_all] = '\x00';

/*
	DBG HTTP START
*/
	http_dbg_to_file(ga, data_send, std::string(http_recv_buf_ptr), host, port, stock);
/*
	DBG HTTP END
*/

	// jeśli trzeba, wyciągnij cookies z bufora
	if(get_cookies && ! http_get_cookies(std::string(http_recv_buf_ptr), ga.cookies, msg_err))
	{
		std::free(http_recv_buf_ptr);

		// opis błędu jest w msg_err

		return NULL;
	}

	// zwróć wskaźnik do bufora pobranych danych
	return http_recv_buf_ptr;
}


void irc_send(struct global_args &ga, struct channel_irc *ci[], std::string irc_send_buf, std::string dbg_irc_msg)
{
	// debug IRC w oknie (jeśli został włączony)
	if(ga.debug_irc)
	{
		std::stringstream irc_send_buf_stream(irc_send_buf);
		std::string irc_send_buf_line;

		size_t code_erase;

		while(std::getline(irc_send_buf_stream, irc_send_buf_line))
		{
			code_erase = irc_send_buf_line.find("\r");

			while(code_erase != std::string::npos)
			{
				irc_send_buf_line.erase(code_erase, 1);
				code_erase = irc_send_buf_line.find("\r");
			}

			win_buf_add_str(ga, ci, "DebugIRC", xYELLOW "> " + irc_send_buf_line, true, 1, true, false);
		}
	}

	// przekoduj UTF-8 na ISO-8859-2
	irc_send_buf = buf_utf_to_iso(irc_send_buf);

	// do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)
	irc_send_buf += "\r\n";

	// rozmiar danych do wysłania
	int irc_send_buf_len = irc_send_buf.size();

	// wyślij dane do hosta
	int bytes_sent = send(ga.socketfd_irc, irc_send_buf.c_str(), irc_send_buf_len, 0);

	if(bytes_sent == -1)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED + dbg_irc_msg + "Nie udało się wysłać danych do serwera (IRC).");

		ga.irc_ok = false;
	}

	else if(bytes_sent == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED + dbg_irc_msg + "Podczas próby wysłania danych serwer zakończył połączenie (IRC).");

		ga.irc_ok = false;
	}

	else if(bytes_sent != irc_send_buf_len)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED + dbg_irc_msg + "Nie udało się wysłać wszystkich danych do serwera (IRC).");

		ga.irc_ok = false;
	}
}


void irc_recv(struct global_args &ga, struct channel_irc *ci[], std::string &irc_recv_buf, std::string dbg_irc_msg)
{
	char irc_recv_buf_tmp[RECV_BUF_TMP_SIZE];

	// bufor pomocniczy do zachowania niepełnego fragmentu ostatniego wiersza, jeśli nie został pobrany w całości w jednej ramce
	static std::string irc_recv_buf_incomplete;

	// wstępnie załóż, że nie było niepełnego wiersza poprzednio, zmiana nastąpi, gdy był
	bool was_irc_recv_buf_incomplete = false;

	// wstępnie załóż, że nie będzie niepełnego wiersza, zmiana nastąpi, gdy taki wiersz będzie
	ga.is_irc_recv_buf_incomplete = false;

	// pobierz dane od hosta
	int bytes_recv = recv(ga.socketfd_irc, irc_recv_buf_tmp, RECV_BUF_TMP_SIZE - 1, 0);	// - 1, aby zostawić miejsce na wstawienie NULL

	if(bytes_recv == -1)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED + dbg_irc_msg + "Nie udało się pobrać danych z serwera (IRC).");

		ga.irc_ok = false;
	}

	else if(bytes_recv == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED + dbg_irc_msg + "Podczas próby pobrania danych serwer zakończył połączenie (IRC).");

		ga.irc_ok = false;
	}

	// gdy nie było błędu
	else
	{
		// zakończ bufor tymczasowy kodem NULL
		irc_recv_buf_tmp[bytes_recv] = '\x00';

		// odebrane dane zwróć w buforze std::string
		irc_recv_buf.clear();
		irc_recv_buf = std::string(irc_recv_buf_tmp);

		// usuń 0x02 z bufora (występuje zaraz po zalogowaniu się do IRC w komunikacie powitalnym)
		size_t code_erase = irc_recv_buf.find("\x02");

		while(code_erase != std::string::npos)
		{
			irc_recv_buf.erase(code_erase, 1);
			code_erase = irc_recv_buf.find("\x02");
		}

		// usuń \r z bufora (w ncurses wyświetlenie tego na Linuksie powoduje, że linia jest niewidoczna)
		code_erase = irc_recv_buf.find("\r");

		while(code_erase != std::string::npos)
		{
			irc_recv_buf.erase(code_erase, 1);
			code_erase = irc_recv_buf.find("\r");
		}

		// serwer wysyła dane w kodowaniu ISO-8859-2, zamień je na UTF-8
		irc_recv_buf = buf_iso_to_utf(irc_recv_buf);

		// dopisz do początku bufora głównego ewentualnie zachowany niepełny fragment poprzedniego wiersza z bufora pomocniczego
		if(irc_recv_buf_incomplete.size() > 0)
		{
			irc_recv_buf.insert(0, irc_recv_buf_incomplete);

			// po przepisaniu wyczyść bufor pomocniczy
			irc_recv_buf_incomplete.clear();

			// był niepełny wiersz poprzednio, jest to informacja potrzebna do ustalenia, czy odświeżyć okno "wirtualne" w pętli głównej
			was_irc_recv_buf_incomplete = true;
		}

		// wykryj, czy w buforze głównym jest niepełny wiersz (brak \r\n na końcu), jeśli tak, przenieś go do bufora pomocniczego
		// ( \r\n w domyśle, bo \r został usunięty z bufora podczas pobierania danych z serwera nieco wyżej)
		size_t pos_incomplete = irc_recv_buf.rfind("\n");

		// - 1, bo pozycja jest liczona od zera, a długość jest całkowitą liczbą zajmowanych bajtów
		if(pos_incomplete != irc_recv_buf.size() - 1)
		{
			// zachowaj ostatni niepełny wiersz
			irc_recv_buf_incomplete.insert(0, irc_recv_buf, pos_incomplete + 1, irc_recv_buf.size() - pos_incomplete - 1);

			// oraz usuń go z głównego bufora
			irc_recv_buf.erase(pos_incomplete + 1, irc_recv_buf.size() - pos_incomplete - 1);

			// był niepełny wiersz obecnie, jest to informacja potrzebna do ustalenia, czy odświeżyć okno "wirtualne" w pętli głównej
			// (jeśli wcześniej nie było niepełnego wiersza, a był teraz, nieodświeżenie okna "wirtualnego" przyspiesza wczytywanie)
			ga.is_irc_recv_buf_incomplete = true;
		}

		// jeśli wcześniej był niepełny wiersz, a teraz nie było, wymuś odświeżenie okna "wirtualnego" w pętli głównej, bo może się czasami zdarzyć,
		// że ostatni wiersz nie wymaga wyświetlenia i wtedy poprzedni nie pokaże się w odpowiednim momencie
		if(was_irc_recv_buf_incomplete && ! ga.is_irc_recv_buf_incomplete)
		{
			ga.win_chat_refresh = true;
		}

		// debug IRC w oknie (jeśli został włączony)
		if(ga.debug_irc)
		{
			std::stringstream irc_recv_buf_stream(irc_recv_buf);
			std::string irc_recv_buf_line;

			while(std::getline(irc_recv_buf_stream, irc_recv_buf_line))
			{
				win_buf_add_str(ga, ci, "DebugIRC", xWHITE + irc_recv_buf_line, true, 1, true, false);
			}
		}
	}
}
