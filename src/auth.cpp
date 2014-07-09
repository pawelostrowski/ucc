#include <string>		// std::string
#include <cstring>		// memcpy(), strstr()

// -std=gnu++11 - free(), system(), std::to_string()

#include "auth.hpp"
#include "network.hpp"
#include "irc_parser.hpp"
#include "window_utils.hpp"
#include "enc_str.hpp"
#include "ucc_global.hpp"


std::string auth_code(std::string &authkey)
{
	// authKey musi mieć dokładnie 16 znaków, inaczej konwersja nie ma sensu
	if(authkey.size() == 16)
	{
		const int f1[] = {29, 43,  7,  5, 52, 58, 30, 59, 26, 35, 35, 49, 45,  4, 22,  4,
				   0,  7,  4, 30, 51, 39, 16,  6, 32, 13, 40, 44, 14, 58, 27, 41,
				  52, 33,  9, 30, 30, 52, 16, 45, 43, 18, 27, 52, 40, 52, 10,  8,
				  10, 14, 10, 38, 27, 54, 48, 58, 17, 34,  6, 29, 53, 39, 31, 35,
				  60, 44, 26, 34, 33, 31, 10, 36, 51, 44, 39, 53,  5, 56};

		const int f2[] = { 7, 32, 25, 39, 22, 26, 32, 27, 17, 50, 22, 19, 36, 22, 40, 11,
				  41, 10, 10,  2, 10,  8, 44, 40, 51,  7,  8, 39, 34, 52, 52,  4,
				  56, 61, 59, 26, 22, 15, 17,  9, 47, 38, 45, 10,  0, 12,  9, 20,
				  51, 59, 32, 58, 19, 28, 11, 40,  8, 28,  6,  0, 13, 47, 34, 60,
				   4, 56, 21, 60, 59, 16, 38, 52, 61, 44,  8, 35,  4, 11};

		const int f3[] = {60, 30, 12, 34, 33,  7, 15, 29, 16, 20, 46, 25,  8, 31,  4, 48,
				   6, 44, 57, 16, 12, 58, 48, 59, 21, 32,  2, 18, 51,  8, 50, 29,
				  58,  6, 24, 34, 11, 23, 57, 43, 59, 50, 10, 56, 27, 32, 12, 59,
				  16,  4, 40, 39, 26, 10, 49, 56, 51, 60, 21, 37, 12, 56, 39, 15,
				  53, 11, 33, 43, 52, 37, 30, 25, 19, 55,  7, 34, 48, 36};

		const int p1[] = {11,  9, 12,  0,  1,  4, 10, 13,  3,  6,  7,  8, 15,  5,  2, 14};

		const int p2[] = { 1, 13,  5,  8,  7, 10,  0, 15, 12,  3, 14, 11,  2,  9,  6,  4};

		int c;
		int ai[16], ai1[16];

		for(int i = 0; i < 16; ++i)
		{
			c = authkey[i];		// std::string na int (po jednym znaku)
			// ASCII:    9        Z        =        7        0
			ai[i] = c > 57 ? c > 90 ? c - 61 : c - 55 : c - 48;
		}

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = f1[ai[i] + i];
		}

		memcpy(ai1, ai, sizeof(ai1));	// skopiuj ai do ai1

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = (ai[i] + ai1[p1[i]]) % 62;
		}

		for(int i = 0; i < 16; ++i)
		{
			ai[i] = f2[ai[i] + i];
		}

		memcpy(ai1, ai, sizeof(ai1));	// skopiuj ai do ai1

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
			// ASCII:    \n         $        =        7        0
			ai[i] = c >= 10 ? c >= 36 ? c + 61 : c + 55 : c + 48;
			authkey += ai[i];	// int na std::string (po jednym znaku)
		}
	}

	// zwróć przekonwertowany authKey lub niezmieniony, jeśli authKey nie miał 16 znaków (błędny rozmiar zostanie wykryty poza funkcją)
	return authkey;
}


bool auth_http_init(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Pobierz pierwsze 4 ciastka (onet_ubi, onetzuo_ticket, onet_cid, onet_sgn).
*/

	char *http_recv_buf;
	long http_recv_offset;

	// wyczyść bufor cookies przed zapoczątkowaniem połączenia
	ga.cookies.clear();

	http_recv_buf = http_get_data(ga, chan_parm, "GET", "kropka.onet.pl", 80, "/_s/kropka/5?DV=czat/applet/FULL", "",
				ga.cookies, true, http_recv_offset, "authHttpInit");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	// nie wolno zapomnieć o zwolnieniu pamięci! (w przypadku błędu w http_get_data() nie trzeba zwalniać pamięci, bo funkcja zrobiła to sama)
	free(http_recv_buf);

	return true;
}


bool auth_http_getcaptcha(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Pobierz captcha i kolejne, piąte ciastko (onet_sid), jednocześnie wysyłając pobrane poprzednio 4 ciastka.
*/

	char *http_recv_buf;
	long http_recv_offset;
	char *cap_ptr;

	http_recv_buf = http_get_data(ga, chan_parm, "GET", "czat.onet.pl", 80, "/myimg.gif", "", ga.cookies, true, http_recv_offset, "authHttpGetCaptcha");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	// daj wskaźnik na początek obrazka
	cap_ptr = strstr(http_recv_buf, "GIF");

	if(cap_ptr == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie udało się pobrać obrazka z kodem do przepisania.");

		free(http_recv_buf);
		return false;
	}

	// zapisz obrazek z captcha na dysku
	std::ofstream cap_file(CAPTCHA_FILE, std::ios::binary);

	if(cap_file.good())
	{
		// &http_recv_buf[http_recv_offset] - cap_ptr  <--- <adres końca bufora> - <adres początku obrazka> = <rozmiar obrazka>
		cap_file.write(cap_ptr, &http_recv_buf[http_recv_offset] - cap_ptr);
		cap_file.close();

		free(http_recv_buf);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# Nie udało się uzyskać dostępu do " CAPTCHA_FILE " (sprawdź uprawnienia do zapisu).");

		free(http_recv_buf);
		return false;
	}

	// wyświetl obrazek z kodem do przepisania
	if(int system_stat = system("/usr/bin/eog " CAPTCHA_FILE " 2>/dev/null &") != 0)	// do poprawy, rozwiązanie tymczasowe!!!
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# Proces uruchamiający obrazek z kodem do przepisania zakończył się błędem numer: " + std::to_string(system_stat) + "\n"
				xRED "# Plik możesz otworzyć ręcznie, znajduje się w: " CAPTCHA_FILE);
	}

	ga.captcha_ready = true;	// można przepisać kod i wysłać na serwer

	return true;
}


bool auth_http_getsk(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	W przypadku nicka stałego nie trzeba pobierać captcha, ale potrzebne jest piąte ciastko (onet_sid), a można je uzyskać,
	pobierając z serwera mały obrazek.
*/

	char *http_recv_buf;
	long http_recv_offset;

	http_recv_buf = http_get_data(ga, chan_parm, "GET", "czat.onet.pl", 80, "/sk.gif", "", ga.cookies, true, http_recv_offset, "authHttpGetSk");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	free(http_recv_buf);

	return true;
}


bool auth_http_checkcode(struct global_args &ga, struct channel_irc *chan_parm[], std::string &captcha)
{
	char *http_recv_buf;
	long http_recv_offset;
	std::string err_code;

	ga.captcha_ready = false;	// zapobiega ponownemu wysłaniu kodu na serwer

	http_recv_buf = http_get_data(ga, chan_parm, "POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
				"api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha + "\";}",
				ga.cookies, false, http_recv_offset, "authHttpCheckCode");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	// sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE),
	// czyli pobierz wartość między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
	err_code = get_value_from_buf(std::string(http_recv_buf), "err_code=\"", "\"");

	free(http_recv_buf);

	if(err_code.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# authHttpCheckCode: Serwer nie zwrócił err_code.");

		return false;
	}

	// jeśli serwer zwrócił FALSE, oznacza to błędnie wpisany kod captcha
	if(err_code == "FALSE")
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# Wpisany kod jest błędny, aby zacząć od nowa, wpisz " xCYAN "/connect " xRED "lub " xCYAN "/c");

		return false;
	}

	// brak TRUE (lub wcześniejszego FALSE) oznacza błąd w odpowiedzi serwera
	if(err_code != "TRUE")
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# authHttpCheckCode: Serwer nie zwrócił oczekiwanego TRUE lub FALSE, zwrócona wartość: " + err_code);

		return false;
	}

	return true;
}


bool auth_http_mlogin(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	W tym miejscu (logowanie nicka stałego) pobierane są kolejne dwa ciastka (onet_uoi, onet_uid).
*/

	char *http_recv_buf;
	long http_recv_offset;

	http_recv_buf = http_get_data(ga, chan_parm, "POST", "secure.onet.pl", 443, "/mlogin.html",
				"r=&url=&login=" + ga.my_nick + "&haslo=" + ga.my_password + "&app_id=20&ssl=1&ok=1",
				ga.cookies, true, http_recv_offset, "authHttpMLogin");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	free(http_recv_buf);

	return true;
}


bool auth_http_useroverride(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Funkcja ta przydaje się, gdy nas rozłączy, ale nie wyrzuci jeszcze z serwera (łączy od razu, bez komunikatu błędu o zalogowanym już nicku),
	dotyczy nicka stałego.
*/

	char *http_recv_buf;
	long http_recv_offset;

	http_recv_buf = http_get_data(ga, chan_parm, "POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
				"api_function=userOverride&params=a:1:{s:4:\"nick\";s:" + std::to_string(ga.my_nick.size()) + ":\"" + ga.my_nick + "\";}",
				ga.cookies, false, http_recv_offset, "authHttpUserOverride");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	free(http_recv_buf);

	return true;
}


bool auth_http_getuokey(struct global_args &ga, struct channel_irc *chan_parm[])
{
	char *http_recv_buf;
	long http_recv_offset;
	std::string my_nick_c, my_nick_i;
	std::string err_code;

	// wykonaj "kopię" nicka, aby nie modyfikować go z punktu widzenia użytkownika (gdy trzeba usunąć ~)
	my_nick_c = ga.my_nick;

	// wykryj, czy nick jest stały, czy tymczasowy (na podstawie obecności hasła)
	if(ga.my_password.size() == 0)
	{
		my_nick_i = "1";	// tymczasowy

		// jeśli podano nick (tymczasowy) z tyldą na początku, usuń ją, bo serwer takiego nicka nie akceptuje, mimo iż potem taki nick zwraca po
		// zalogowaniu się
		if(my_nick_c.size() > 0 && my_nick_c[0] == '~')
		{
			my_nick_c.erase(0, 1);
		}
	}

	else
	{
		my_nick_i = "0";	// stały
	}

	http_recv_buf = http_get_data(ga, chan_parm, "POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
				"api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + std::to_string(my_nick_c.size()) + ":\"" + my_nick_c
				+ "\";s:8:\"tempNick\";i:" + my_nick_i + ";s:7:\"version\";s:" + std::to_string(sizeof(AP_VER) - 1) + ":\"" + AP_VER + "\";}",
				ga.cookies, false, http_recv_offset, "authHttpGetUoKey");

	if(http_recv_buf == NULL)
	{
		return false;
	}

	// pobierz kod błędu
	err_code = get_value_from_buf(std::string(http_recv_buf), "err_code=\"", "\"");

	if(err_code.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# authHttpGetUoKey: Serwer nie zwrócił err_code.");

		free(http_recv_buf);
		return false;
	}

	// sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
	if(err_code != "TRUE")
	{
		std::string err_text = get_value_from_buf(std::string(http_recv_buf), "err_text=\"", "\"");

		if(err_text.size() > 0)
		{
			err_text = buf_iso2utf(err_text);
		}

		else
		{
			err_text = "<serwer nie zwrócił komunikatu błędu>";
		}

		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# authHttpGetUoKey: Błąd serwera (" + err_code + "): " + err_text);

		free(http_recv_buf);
		return false;
	}

	// pobierz uoKey
	ga.uokey = get_value_from_buf(std::string(http_recv_buf), "<uoKey>", "</uoKey>");

	if(ga.uokey.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# authHttpGetUoKey: Serwer nie zwrócił uoKey.");

		free(http_recv_buf);
		return false;
	}

	// pobierz zuoUsername (nick, który zwrócił serwer)
	ga.zuousername = get_value_from_buf(std::string(http_recv_buf), "<zuoUsername>", "</zuoUsername>");

	free(http_recv_buf);

	if(ga.zuousername.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# authHttpGetUoKey: Serwer nie zwrócił zuoUsername.");

		return false;
	}

	ga.irc_ready = true;	// gotowość do połączenia z IRC

	return true;
}


void auth_irc_all(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// nie próbuj ponownie łączyć się do IRC od zera
	ga.irc_ready = false;

	// zacznij od ustanowienia poprawności połączenia z IRC, zostanie ono zmienione na niepowodzenie,
	// gdy napotkamy błąd podczas któregoś etapu autoryzacji do IRC
	ga.irc_ok = true;

/*
	Część 1 autoryzacji.
*/
	// zainicjalizuj gniazdo oraz połącz z IRC
	ga.socketfd_irc = socket_init(ga, chan_parm, "czat-app.onet.pl", 5015, "authIrc1");

	// w przypadku błędu zakończ
	if(ga.socketfd_irc == 0)
	{
		ga.irc_ok = false;
		return;
	}

	// pobierz pierwszą odpowiedź serwera po połączeniu:
	// :cf1f4.onet NOTICE Auth :*** Looking up your hostname...
	irc_parser(ga, chan_parm, "authIrc1: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

/*
	Część 2 autoryzacji.
*/
	// wyślij:
	// NICK <zuoUsername>
	irc_send(ga, chan_parm, "NICK " + ga.zuousername, "authIrc2: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// pobierz drugą odpowiedź serwera:
	// :cf1f4.onet NOTICE Auth :*** Found your hostname (ajs7.neoplus.adsl.tpnet.pl) -- cached
	irc_parser(ga, chan_parm, "authIrc2: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

/*
	Część 3 autoryzacji.
*/
	// wyślij:
	// AUTHKEY
	irc_send(ga, chan_parm, "AUTHKEY", "authIrc3a: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// pobierz trzecią odpowiedź serwera:
	// :cf1f4.onet 801 ucc_test :authKey
	// parser wyszuka kod authKey, przekonwertuje i wyśle na serwer:
	// AUTHKEY authKey
	irc_parser(ga, chan_parm, "authIrc3b: ");

	// w przypadku błędu zakończ
	if(! ga.irc_ok)
	{
		return;
	}

/*
	Część 4 autoryzacji.
*/
	// wyślij (zamiast zuoUsername można wysłać inną nazwę):
	// USER * <uoKey> czat-app.onet.pl :<zuoUsername>
	// PROTOCTL ONETNAMESX
	irc_send(ga, chan_parm, "USER * " + ga.uokey + " czat-app.onet.pl :" + ga.zuousername + "\r\nPROTOCTL ONETNAMESX", "authIrc4: ");

	// jeśli na którymś etapie wystąpił błąd, funkcja auth_irc_all() zakończy się z ga.irc_ok = false

	return;
}
