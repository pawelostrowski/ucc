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
#include <cstring>		// std::memcpy(), std::strstr()
#include <sys/stat.h>		// mkdir()

// -std=c++11 - std::free(), std::system(), std::to_string()

#include "auth.hpp"
#include "debug.hpp"
#include "network.hpp"
#include "window_utils.hpp"
#include "irc_parser.hpp"
#include "enc_str.hpp"
#include "ucc_global.hpp"


void auth_code(std::string &authkey)
{
	// authKey musi mieć dokładnie 16 znaków, inaczej konwersja nie ma sensu
	if(authkey.size() == 16)
	{
		const char f1[] = {29, 43,  7,  5, 52, 58, 30, 59, 26, 35, 35, 49, 45,  4, 22,  4,
				    0,  7,  4, 30, 51, 39, 16,  6, 32, 13, 40, 44, 14, 58, 27, 41,
				   52, 33,  9, 30, 30, 52, 16, 45, 43, 18, 27, 52, 40, 52, 10,  8,
				   10, 14, 10, 38, 27, 54, 48, 58, 17, 34,  6, 29, 53, 39, 31, 35,
				   60, 44, 26, 34, 33, 31, 10, 36, 51, 44, 39, 53,  5, 56};

		const char f2[] = { 7, 32, 25, 39, 22, 26, 32, 27, 17, 50, 22, 19, 36, 22, 40, 11,
				   41, 10, 10,  2, 10,  8, 44, 40, 51,  7,  8, 39, 34, 52, 52,  4,
				   56, 61, 59, 26, 22, 15, 17,  9, 47, 38, 45, 10,  0, 12,  9, 20,
				   51, 59, 32, 58, 19, 28, 11, 40,  8, 28,  6,  0, 13, 47, 34, 60,
				    4, 56, 21, 60, 59, 16, 38, 52, 61, 44,  8, 35,  4, 11};

		const char f3[] = {60, 30, 12, 34, 33,  7, 15, 29, 16, 20, 46, 25,  8, 31,  4, 48,
				    6, 44, 57, 16, 12, 58, 48, 59, 21, 32,  2, 18, 51,  8, 50, 29,
				   58,  6, 24, 34, 11, 23, 57, 43, 59, 50, 10, 56, 27, 32, 12, 59,
				   16,  4, 40, 39, 26, 10, 49, 56, 51, 60, 21, 37, 12, 56, 39, 15,
				   53, 11, 33, 43, 52, 37, 30, 25, 19, 55,  7, 34, 48, 36};

		const int p1[] =  {11,  9, 12,  0,  1,  4, 10, 13,  3,  6,  7,  8, 15,  5,  2, 14};

		const int p2[] =  { 1, 13,  5,  8,  7, 10,  0, 15, 12,  3, 14, 11,  2,  9,  6,  4};

		char c;
		char ai[16], ai1[16];

		for(int i = 0; i < 16; ++i)
		{
			c = authkey[i];

			// ASCII:     9         Z        =        7         0
			ai[i] = (c > 57 ? (c > 90 ? c - 61 : c - 55) : c - 48);
		}

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = f1[ai[i] + i];
		}

		std::memcpy(ai1, ai, sizeof(ai1));	// skopiuj ai do ai1

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = (ai[i] + ai1[p1[i]]) % 62;
		}

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = f2[ai[i] + i];
		}

		std::memcpy(ai1, ai, sizeof(ai1));	// skopiuj ai do ai1

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = (ai[i] + ai1[p2[i]]) % 62;
		}

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = f3[ai[i] + i];
		}

		authkey.clear();

		for(int i = 0; i < 16; ++i)
		{
			c = ai[i];

			// ASCII:     \n          $        =        7         0
			ai[i] = (c >= 10 ? (c >= 36 ? c + 61 : c + 55) : c + 48);

			authkey += ai[i];
		}
	}
}


bool auth_http_init(struct global_args &ga, struct channel_irc *ci[])
{
/*
	Pobierz pierwsze 4 ciastka: onet_ubi, onetzuo_ticket, onet_cid, onet_sgn
	Ciastko onet_sgn nie będzie jednak później potrzebne, bo należy do subdomeny kropka.onet.pl, a nie, jak pozostałe, do .onet.pl (nic więcej nie
	będzie pobierane ani wysyłane do kropka.onet.pl).
*/

	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpInit...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpInit");

	// wyczyść bufor cookies przed zapoczątkowaniem połączenia
	ga.cookies.clear();

	char *http_recv_buf_ptr = http_get_data(ga, "GET", "kropka.onet.pl", (ga.all_auth_https ? 443 : 80), "/_s/kropka/5?DV=czat/applet/FULL",
				"", true, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	// nie wolno zapomnieć o zwolnieniu pamięci! (w przypadku błędu w http_get_data() nie trzeba zwalniać pamięci, bo funkcja zrobiła to sama)
	std::free(http_recv_buf_ptr);

	return true;
}


bool auth_http_getcaptcha(struct global_args &ga, struct channel_irc *ci[])
{
/*
	Pobierz CAPTCHA i kolejne, piąte ciastko: onet_sid
*/

	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpGetCaptcha...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpGetCaptcha");

	char *http_recv_buf_ptr = http_get_data(ga, "GET", "czat.onet.pl", (ga.all_auth_https ? 443 : 80), "/myimg.gif",
				"", true, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	// daj wskaźnik na początek obrazka
	char *captcha_ptr = std::strstr(http_recv_buf_ptr, "GIF");

	if(captcha_ptr == NULL)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie udało się pobrać obrazka z kodem do przepisania.");

		std::free(http_recv_buf_ptr);
		return false;
	}

	// zapisz obrazek z CAPTCHA na dysku
	std::ofstream captcha_f(CAPTCHA_FILE, std::ios::binary);

	if(captcha_f.good())
	{
		// przy pobieraniu przez HTTPS serwer zwraca rozmiary bloków wewnątrz obrazka, trzeba więc wyłuskać obrazek z ich wnętrza
		char *block_rnrn_ptr = std::strstr(http_recv_buf_ptr, "\r\n\r\n");// za tym ciągiem znajdują się dane obrazka lub rozmiar pierwszego bloku

		// jeśli adres za ciągiem "\r\n\r\n" nie jest taki sam jak adres ciągu "GIF", oznacza to, że jest tam rozmiar pierwszego bloku
		// (pilnuj też, aby na pewno był znaleziony ciąg "\r\n\r\n" i nie wychodził on poza rozmiar pobranych danych)
		if(block_rnrn_ptr != NULL && block_rnrn_ptr + 4 < &http_recv_buf_ptr[bytes_recv_all] && block_rnrn_ptr + 4 != captcha_ptr)
		{
			// bufor dla rozmiaru bloku z ciągu ASCII
			std::string block_ascii;

			// rozmiar bloku do pobrania po przekształceniu ciągu ASCII na int
			int block_size;

			// wskaźnik na offset dla początku rozmiaru bloku
			char *block_offset_ptr = block_rnrn_ptr + 4;

			do
			{
				// tymczasowy strumień dla rozmiaru bloku, używany do konwersji ASCII na int
				std::stringstream block_stream;

				// wpisanie zera zabezpiecza przed błędem bufora, gdyby jakimś cudem od razu znalazł się tam "\r"
				block_ascii = "0";

				// wczytaj rozmiar bloku
				for(int i = 0; i < bytes_recv_all; ++i)
				{
					// pobieraj do napotkania "\r" (ogranicz możliwy ciąg do 6, gdyby wystąpił błąd w pobranych danych)
					if(i == 6 || block_offset_ptr[i] == '\r')
					{
						// wczytaj do strumienia pobrane dane, uwzględniając, że ciąg ASCII zapisany jest w postaci HEX
						block_stream << std::hex << block_ascii;

						// konwersja na int
						block_stream >> block_size;

						// zapisz blok danych do pliku, jeśli względem początku pobranych danych dodanie ciągu rozmiaru bloku,
						// rozmiaru samego bloku oraz ciągu "\r\n" za ciągiem rozmiaru bloku nie przekracza liczby pobranych danych
						if((block_offset_ptr - http_recv_buf_ptr) + i + block_size + 2 < bytes_recv_all)
						{
							// zapisz blok danych do pliku (pomiń rozmiar bloku oraz ciąg "\r\n" występujący za nim)
							captcha_f.write(block_offset_ptr + i + 2, block_size);

							// aby wyznaczyć kolejny offset, do obecnego offsetu dodaj rozmiar bloku, rozmiar zajmowany przez
							// ciąg zawierający rozmiar bloku oraz powtarzające się dwa ciągi "\r\n" (jeden za rozmiarem bloku,
							// drugi przed kolejnym rozmiarem bloku)
							block_offset_ptr += block_size + i + 4;
						}

						// jeśli z jakiegoś powodu pobrany ciąg rozmiaru bloku przekracza liczbę pobranych danych wpisz zero do
						// rozmiaru bloku, co zakończy pętlę do {} while();
						else
						{
							block_size = 0;
						}

						break;
					}

					// wczytaj kolejne znaki ciągu rozmiaru bloku
					block_ascii += block_offset_ptr[i];
				}

			} while(block_size != 0);
		}

		// jeśli nie było podanego rozmiaru bloków (przy HTTP), zapisz plik od ciągu GIF do końca rozmiaru pobranych danych
		else
		{
			// &http_recv_buf_ptr[bytes_recv_all] - captcha_ptr  <--- <adres końca bufora> - <adres początku obrazka> = <rozmiar obrazka>
			captcha_f.write(captcha_ptr, &http_recv_buf_ptr[bytes_recv_all] - captcha_ptr);
		}

		captcha_f.close();

		std::free(http_recv_buf_ptr);
	}

	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Brak dostępu do \"" CAPTCHA_FILE "\" (sprawdź uprawnienia do zapisu).");

		std::free(http_recv_buf_ptr);
		return false;
	}

	// wyświetl obrazek z kodem do przepisania
	int system_stat = std::system("/usr/bin/eog " CAPTCHA_FILE " 2>/dev/null &");	// do poprawy, rozwiązanie tymczasowe!!!

	if(system_stat != 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED "Proces uruchamiający obrazek z kodem do przepisania zakończył się błędem numer: "
				+ std::to_string(system_stat) + "\n"
				uINFOn xRED "Plik możesz otworzyć ręcznie, znajduje się w: " CAPTCHA_FILE);
	}

	// po pomyślnym pobraniu obrazka pokaż komunikat o przepisaniu
	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			uINFOn xGREEN "Przepisz kod z obrazka, w tym celu wpisz " xCYAN "/captcha kod_z_obrazka" xGREEN " lub " xCYAN "/cap kod_z_obrazka");

	// można przepisać kod i wysłać na serwer
	ga.captcha_ready = true;

	return true;
}


bool auth_http_checkcode(struct global_args &ga, struct channel_irc *ci[], std::string &captcha)
{
	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpCheckCode...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpCheckCode");

	// zapobiega ponownemu wysłaniu kodu na serwer
	ga.captcha_ready = false;

	char *http_recv_buf_ptr = http_get_data(ga, "POST", "czat.onet.pl", (ga.all_auth_https ? 443 : 80), "/include/ajaxapi.xml.php3",
				"api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha + "\";}", false, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	std::string http_recv_buf_str = std::string(http_recv_buf_ptr);

	// sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE),
	// czyli pobierz wartość między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
	std::string err_code = get_value_from_buf(http_recv_buf_str, "err_code=\"", "\"");

	std::free(http_recv_buf_ptr);

	if(err_code.size() == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Serwer nie zwrócił err_code.");

		return false;
	}

	// jeśli serwer zwrócił FALSE, oznacza to błędnie wpisany kod CAPTCHA
	if(err_code == "FALSE")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED "Wpisany kod jest błędny, aby spróbować ponownie, wpisz " xCYAN "/connect" xRED " lub " xCYAN "/c");

		return false;
	}

	// brak TRUE (lub wcześniejszego FALSE) oznacza błąd w odpowiedzi serwera
	if(err_code != "TRUE")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED "Serwer nie zwrócił oczekiwanego TRUE lub FALSE, zwrócona wartość: " + err_code);

		return false;
	}

	return true;
}


bool auth_http_getsk(struct global_args &ga, struct channel_irc *ci[])
{
/*
	W przypadku nicka stałego nie trzeba pobierać CAPTCHA, ale potrzebne jest piąte ciastko: onet_sid
	Można je uzyskać, pobierając z serwera mały obrazek.
*/

	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpGetSk...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpGetSk");

	char *http_recv_buf_ptr = http_get_data(ga, "GET", "czat.onet.pl", (ga.all_auth_https ? 443 : 80), "/sk.gif",
				"", true, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	std::free(http_recv_buf_ptr);

	return true;
}


bool auth_http_mlogin(struct global_args &ga, struct channel_irc *ci[])
{
/*
	W tym miejscu (logowanie nicka stałego) pobierane są kolejne dwa ciastka: onet_uoi, onet_uid, połączenie jest szyfrowane zawsze, niezależnie od
	wartości ga.all_auth_https
*/

	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpMLogin...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpMLogin");

	char *http_recv_buf_ptr = http_get_data(ga, "POST", "secure.onet.pl", 443, "/mlogin.html",
				"r=&url=&login=" + buf_utf_to_iso(ga.my_nick) + "&haslo=" + buf_utf_to_iso(ga.my_password) + "&app_id=20&ssl=1&ok=1",
				true, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	std::free(http_recv_buf_ptr);

	return true;
}


bool auth_http_useroverride(struct global_args &ga, struct channel_irc *ci[])
{
/*
	Funkcja ta przydaje się, gdy nas rozłączy, ale nie wyrzuci jeszcze z serwera (łączy od razu, bez komunikatu błędu o zalogowanym już nicku),
	dotyczy nicka stałego.
*/

	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpUserOverride...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpUserOverride");

	char *http_recv_buf_ptr = http_get_data(ga, "POST", "czat.onet.pl", (ga.all_auth_https ? 443 : 80), "/include/ajaxapi.xml.php3",
				"api_function=userOverride&params=a:1:{s:4:\"nick\";s:" + std::to_string(buf_chars(ga.my_nick)) + ":\""
				+ buf_utf_to_iso(ga.my_nick) + "\";}", false, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	std::free(http_recv_buf_ptr);

	return true;
}


bool auth_http_getuokey(struct global_args &ga, struct channel_irc *ci[])
{
	int bytes_recv_all;
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xDARK "authHttpGetUoKey...");
	win_buf_refresh(ga, ci);

	// zapisz nagłówek do pliku debugowania HTTP
	http_dbg_to_file_header(ga, "authHttpGetUoKey");

	// wykonaj "kopię" nicka, aby nie modyfikować go z punktu widzenia użytkownika (gdy trzeba usunąć ~)
	std::string my_nick_c = ga.my_nick;

	// wykryj, czy nick jest stały, czy tymczasowy (na podstawie obecności hasła), jeśli jest tymczasowy i podano tyldę na początku, to ja usuń,
	// bo serwer takiego nicka nie akceptuje, mimo iż potem taki nick zwraca po zalogowaniu się
	if(ga.my_password.size() == 0 && my_nick_c.size() > 0 && my_nick_c[0] == '~')
	{
		my_nick_c.erase(0, 1);
	}

	char *http_recv_buf_ptr = http_get_data(ga, "POST", "czat.onet.pl", (ga.all_auth_https ? 443 : 80), "/include/ajaxapi.xml.php3",
				"api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + std::to_string(buf_chars(my_nick_c)) + ":\"" + buf_utf_to_iso(my_nick_c)
				+ "\";s:8:\"tempNick\";i:" + (ga.my_password.size() == 0 ? "1" : "0") + ";s:7:\"version\";s:"
				+ std::to_string(sizeof(AP_VER) - 1) + ":\"" + AP_VER + "\";}", false, bytes_recv_all, msg_err);

	if(http_recv_buf_ptr == NULL)
	{
		// wyświetl zwrócony komunikat błędu
		win_buf_add_str(ga, ci, "Status", uINFOn xRED + msg_err, true, 3);

		return false;
	}

	std::string http_recv_buf_str = std::string(http_recv_buf_ptr);

	// pobierz kod błędu
	std::string err_code = get_value_from_buf(http_recv_buf_str, "err_code=\"", "\"");

	if(err_code.size() == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Serwer nie zwrócił err_code.");

		std::free(http_recv_buf_ptr);
		return false;
	}

	// sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
	if(err_code != "TRUE")
	{
		std::string err_text = get_value_from_buf(http_recv_buf_str, "err_text=\"", "\"");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				uINFOn xRED "Błąd serwera (" + err_code + "): "
				// wyświetl zwrócony błąd lub komunikat o błędzie (nie powinno się zdarzyć, ale lepiej się zabezpieczyć)
				+ (err_text.size() > 0 ? buf_iso_to_utf(err_text) : "<serwer nie zwrócił komunikatu błędu>"));

		std::free(http_recv_buf_ptr);
		return false;
	}

	// pobierz uoKey
	ga.uokey = get_value_from_buf(http_recv_buf_str, "<uoKey>", "</uoKey>");

	if(ga.uokey.size() == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Serwer nie zwrócił uoKey.");

		std::free(http_recv_buf_ptr);
		return false;
	}

	// pobierz zuoUsername (nick, który zwrócił serwer)
	ga.zuousername = get_value_from_buf(http_recv_buf_str, "<zuoUsername>", "</zuoUsername>");

	std::free(http_recv_buf_ptr);

	if(ga.zuousername.size() == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Serwer nie zwrócił zuoUsername.");

		return false;
	}

	// przekoduj zwrócony nick z ISO-8859-2 na UTF-8
	ga.zuousername = buf_iso_to_utf(ga.zuousername);

	// gotowość do połączenia z IRC
	ga.irc_ready = true;

	return true;
}


void auth_irc_all(struct global_args &ga, struct channel_irc *ci[])
{
	std::string msg_err;

	// komunikat początkowy wraz z natychmiastowym odświeżeniem, aby pokazał się od razu
	win_buf_add_str(ga, ci, "Status", uINFOb xWHITE "authIrcAll...");
	win_buf_refresh(ga, ci);

	// nie próbuj ponownie łączyć się do IRC od zera
	ga.irc_ready = false;

	// zacznij od ustanowienia poprawności połączenia z IRC, zostanie ono zmienione na niepowodzenie,
	// gdy napotkamy błąd podczas któregoś etapu autoryzacji do IRC
	ga.irc_ok = true;

	// załóż, że nick nie jest w użyciu
	ga.nick_in_use = false;

	// załóż, że authKey jest poprawny
	ga.authkey_failed = false;

	// zacznij od nieaktywnego away oraz busy
	ga.my_away = false;
	ga.my_busy = false;

	// wyczyść listę zalogowanych przyjaciół
	ga.my_friends_online.clear();

	// utwórz katalog użytkownika
	ga.user_dir = ga.ucc_home_dir + "/" + ga.zuousername;
	mkdir(ga.user_dir.c_str(), 0755);

	// pliki debugowania "Status" oraz "DebugIRC", otwierane tu, bo wcześniej nie jest znany ga.zuousername
	if(! ci[CHAN_STATUS]->chan_log.is_open())
	{
		ci[CHAN_STATUS]->chan_log.open(ga.user_dir + "/" + ci[CHAN_STATUS]->channel + ".txt", std::ios::out | std::ios::app);

		if(ci[CHAN_STATUS]->chan_log.good())
		{
			ci[CHAN_STATUS]->chan_log << LOG_STARTED;
		}
	}

	if(ga.debug_irc && ! ci[CHAN_DEBUG_IRC]->chan_log.is_open())
	{
		ci[CHAN_DEBUG_IRC]->chan_log.open(ga.user_dir + "/" + ci[CHAN_DEBUG_IRC]->channel + ".txt", std::ios::out | std::ios::app);

		if(ci[CHAN_DEBUG_IRC]->chan_log.good())
		{
			ci[CHAN_DEBUG_IRC]->chan_log << LOG_STARTED;
		}
	}

/*
	Część 1 autoryzacji.
*/
	// zainicjalizuj gniazdo oraz połącz z IRC
	ga.socketfd_irc = socket_init("czat-app.onet.pl", 5015, msg_err);

	// w przypadku błędu zakończ
	if(ga.socketfd_irc == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "authIrc1: " + msg_err);

		ga.irc_ok = false;

		return;
	}

	// pobierz pierwszą odpowiedź serwera po połączeniu:
	// :cf1f4.onet NOTICE Auth :*** Looking up your hostname...
	irc_parser(ga, ci, "authIrc1: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// pokaż od razu odpowiedź serwera (by nie było widać zwłoki)
	win_buf_refresh(ga, ci);

/*
	Część 2 autoryzacji.
*/
	// wyślij:
	// NICK <zuoUsername>
	irc_send(ga, ci, "NICK " + ga.zuousername, "authIrc2: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// pobierz drugą odpowiedź serwera:
	// :cf1f4.onet NOTICE Auth :*** Found your hostname (ajs7.neoplus.adsl.tpnet.pl)
	// lub w przypadku użycia już nicka:
	// :cf1f4.onet 433 * ucc_test :Nickname is already in use.
	irc_parser(ga, ci, "authIrc2: ");

	// w przypadku błędu lub gdy nick jest już w użyciu, zakończ
	if(! ga.irc_ok || ga.nick_in_use)
	{
		return;
	}

	// pokaż od razu odpowiedź serwera (by nie było widać zwłoki)
	win_buf_refresh(ga, ci);

/*
	Część 3 autoryzacji.
*/
	// wyślij:
	// AUTHKEY
	irc_send(ga, ci, "AUTHKEY", "authIrc3a: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// pobierz trzecią odpowiedź serwera:
	// :cf1f4.onet 801 ucc_test :authKey
	// parser wyszuka kod authKey, przekonwertuje i wyśle na serwer:
	// AUTHKEY authKey
	irc_parser(ga, ci, "authIrc3b: ");

	// w przypadku błędu lub gdy authKey nie jest poprawny, zakończ
	if(! ga.irc_ok || ga.authkey_failed)
	{
		return;
	}

/*
	Część 4 autoryzacji.
*/

	// wyślij (zamiast zuoUsername można wysłać inną nazwę):
	// USER * <uoKey> czat-app.onet.pl :zuoUsername
	// wyślij protokół wyświetlania (nick|flags,type):
	// PROTOCTL ONETNAMESX
	irc_send(ga, ci, "USER * " + ga.uokey + " czat-app.onet.pl :" + ga.zuousername + "\r\nPROTOCTL ONETNAMESX", "authIrc4: ");

	// jeśli na którymś etapie wystąpił błąd, funkcja auth_irc_all() zakończy się z ga.irc_ok = false
}
