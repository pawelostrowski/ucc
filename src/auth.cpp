#include <cstring>		// memcpy(), strstr()
#include <fstream>		// std::string, std::ofstream

// -std=gnu++11 - free(), system(), std::to_string()

#include "auth.hpp"
#include "network.hpp"
#include "irc_parser.hpp"
#include "window_utils.hpp"
#include "ucc_global.hpp"


void auth_code(std::string &authkey)
{
	if(authkey.size() != 16)	// AUTHKEY musi mieć dokładnie 16 znaków
	{
		authkey.clear();	// w przypadku błędu zwróć pusty bufor
		return;
	}

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

	char c;
	char ai[16], ai1[16];

	for(int i = 0; i < 16; ++i)
	{
		c = authkey[i];		// std::string na char (po jednym znaku)
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
		authkey += ai[i];	// char na std::string (po jednym znaku)
	}
}


bool http_auth_init(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Pobierz pierwsze 4 ciastka (onet_ubi, onetzuo_ticket, onet_cid, onet_sgn).
*/

	long offset_recv;
	char *buffer_recv;
	std::string msg_err;

	// wyczyść bufor cookies przed zapoczątkowaniem połączenia
	ga.cookies.clear();

	buffer_recv = http_get_data("GET", "kropka.onet.pl", 80, "/_s/kropka/5?DV=czat/applet/FULL", "", ga.cookies, true, offset_recv, msg_err, "init:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# init: " + msg_err);
		return false;
	}

	// nie wolno zapomnieć o zwolnieniu pamięci! (w przypadku błędu w http_get_data() nie trzeba zwalniać pamięci, bo funkcja zrobiła to sama)
	free(buffer_recv);

	return true;
}


bool http_auth_getcaptcha(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Pobierz captcha i kolejne, piąte ciastko (onet_sid), jednocześnie wysyłając pobrane poprzednio 4 ciastka.
*/

	long offset_recv;
	char *buffer_recv;
	char *cap_ptr;
	std::string msg_err;

	buffer_recv = http_get_data("GET", "czat.onet.pl", 80, "/myimg.gif", "", ga.cookies, true, offset_recv, msg_err, "getCaptcha:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getCaptcha: " + msg_err);
		return false;
	}

	// daj wskaźnik na początek obrazka
	cap_ptr = strstr(buffer_recv, "GIF");

	if(cap_ptr == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie udało się pobrać obrazka z kodem do przepisania.");

		free(buffer_recv);
		return false;
	}

	// zapisz obrazek z captcha na dysku
	std::ofstream cap_file(CAPTCHA_FILE, std::ios::binary);

	if(cap_file.good())
	{
		// &buffer_recv[offset_recv] - cap_ptr  <--- <adres końca bufora> - <adres początku obrazka> = <rozmiar obrazka>
		cap_file.write(cap_ptr, &buffer_recv[offset_recv] - cap_ptr);

		cap_file.close();

		free(buffer_recv);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# Nie udało się uzyskać dostępu do " CAPTCHA_FILE " (sprawdź uprawnienia do zapisu).");

		free(buffer_recv);
		return false;
	}

	// wyświetl obrazek z kodem do przepisania
	if(int system_stat = system("/usr/bin/eog " CAPTCHA_FILE " 2>/dev/null &") != 0)	// to do poprawy, rozwiązanie tymczasowe!!!
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# Proces uruchamiający obrazek do przepisania zakończył się błędem numer: " + std::to_string(system_stat) + "\n"
				xRED "# Plik możesz otworzyć ręcznie, znajduje się w: " CAPTCHA_FILE);
	}

	return true;
}


bool http_auth_getsk(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	W przypadku nicka stałego nie trzeba pobierać captcha, ale potrzebne jest piąte ciastko (onet_sid), a można je uzyskać,
	pobierając z serwera mały obrazek.
*/

	long offset_recv;
	char *buffer_recv;
	std::string msg_err;

	buffer_recv = http_get_data("GET", "czat.onet.pl", 80, "/sk.gif", "", ga.cookies, true, offset_recv, msg_err, "getSk:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getSk: " + msg_err);
		return false;
	}

	free(buffer_recv);

	return true;
}


bool http_auth_checkcode(struct global_args &ga, struct channel_irc *chan_parm[], std::string &captcha)
{
	long offset_recv;
	char *buffer_recv;
	std::string err_code;
	std::string msg_err;

	buffer_recv =	http_get_data("POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
					"api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha + "\";}",
					ga.cookies, false, offset_recv, msg_err, "checkCode:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# checkCode: " + msg_err);
		return false;
	}

	// sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE),
	// czyli pobierz wartość między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
	err_code = get_value_from_buf(std::string(buffer_recv), "err_code=\"", "\"");

	free(buffer_recv);

	if(err_code.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# checkCode: Serwer nie zwrócił err_code.");
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
				xRED "# checkCode: Serwer nie zwrócił oczekiwanego TRUE lub FALSE, zwrócona wartość: " + err_code);

		return false;
	}

	return true;
}


bool http_auth_mlogin(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	W tym miejscu (logowanie nicka stałego) pobierane są kolejne dwa ciastka (onet_uoi, onet_uid).
*/

	long offset_recv;
	char *buffer_recv;
	std::string msg_err;

	buffer_recv =	http_get_data("POST", "secure.onet.pl", 443, "/mlogin.html",
					"r=&url=&login=" + ga.my_nick + "&haslo=" + ga.my_password + "&app_id=20&ssl=1&ok=1",
					ga.cookies, true, offset_recv, msg_err, "mLogin:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# mLogin: " + msg_err);
		return false;
	}

	free(buffer_recv);

	return true;
}


bool http_auth_useroverride(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Funkcja ta przydaje się, gdy nas rozłączy, ale nie wyrzuci jeszcze z serwera (łączy od razu, bez komunikatu błędu o zalogowanym już nicku),
	dotyczy nicka stałego.
*/

	long offset_recv;
	char *buffer_recv;
	std::string msg_err;

	buffer_recv =	http_get_data("POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
					"api_function=userOverride&params=a:1:{s:4:\"nick\";s:" + std::to_string(ga.my_nick.size())
					+ ":\"" + ga.my_nick + "\";}", ga.cookies, false, offset_recv, msg_err, "userOverride:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# userOverride: " + msg_err);
		return false;
	}

	free(buffer_recv);

	return true;
}


bool http_auth_getuokey(struct global_args &ga, struct channel_irc *chan_parm[])
{
	long offset_recv;
	char *buffer_recv;
	std::string my_nick_c, nick_i;
	std::string err_code;
	std::string msg_err;

	// wykonaj "kopię" nicka, aby nie modyfikować go z punktu widzenia użytkownika (gdy trzeba usunąć ~)
	my_nick_c = ga.my_nick;

	// wykryj, czy nick jest stały, czy tymczasowy (na podstawie obecności hasła)
	if(ga.my_password.size() == 0)
	{
		nick_i = "1";	// tymczasowy

		// jeśli podano nick (tymczasowy) z tyldą na początku, usuń ją, bo serwer takiego nicka nie akceptuje,
		//  mimo iż potem taki nick zwraca po zalogowaniu się
		if(my_nick_c.size() > 0 && my_nick_c[0] == '~')
		{
			my_nick_c.erase(0, 1);
		}
	}

	else
	{
		nick_i = "0";	// stały
	}

	buffer_recv =	http_get_data("POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
					"api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + std::to_string(my_nick_c.size()) + ":\"" + my_nick_c
					+ "\";s:8:\"tempNick\";i:" + nick_i + ";s:7:\"version\";s:" + std::to_string(sizeof(APPLET_VER) - 1)
					+ ":\"" + APPLET_VER + "\";}", ga.cookies, false, offset_recv, msg_err, "getUoKey:");

	if(buffer_recv == NULL)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getUoKey: " + msg_err);
		return false;
	}

	// pobierz kod błędu
	err_code = get_value_from_buf(std::string(buffer_recv), "err_code=\"", "\"");

	if(err_code.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getUoKey: Serwer nie zwrócił err_code.");

		free(buffer_recv);
		return false;
	}

	// sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
	if(err_code != "TRUE")
	{
		if(err_code == "-2")		// -2 oznacza nieprawidłowy nick (stały) lub hasło
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd serwera (-2): nieprawidłowy nick lub hasło.");
		}

		else if(err_code == "-4")	// -4 oznacza nieprawidłowe znaki w nicku tymczasowym
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd serwera (-4): nick zawiera niedozwolone znaki.");
		}

		else
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getUoKey: Nieznany błąd serwera, kod błędu: " + err_code);
		}

		free(buffer_recv);
		return false;
	}

	// pobierz uoKey
	ga.uokey = get_value_from_buf(std::string(buffer_recv), "<uoKey>", "</uoKey>");

	if(ga.uokey.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getUoKey: Serwer nie zwrócił uoKey.");

		free(buffer_recv);
		return false;
	}

	// pobierz zuoUsername (nick, który zwrócił serwer)
	ga.zuousername = get_value_from_buf(std::string(buffer_recv), "<zuoUsername>", "</zuoUsername>");

	if(ga.zuousername.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# getUoKey: Serwer nie zwrócił zuoUsername.");

		free(buffer_recv);
		return false;
	}

	free(buffer_recv);

	return true;
}


void irc_auth(struct global_args &ga, struct channel_irc *chan_parm[])
{
	std::string msg_err;

	// nie próbuj się znowu łączyć do IRC od zera
	ga.irc_ready = false;

	// zacznij od ustanowienia poprawności połączenia z IRC, zostanie ono zmienione na niepowodzenie,
	// gdy napotkamy błąd podczas któregoś etapu autoryzacji do IRC
	ga.irc_ok = true;

/*
	Część 1 autoryzacji.
*/
	// zainicjalizuj gniazdo oraz połącz z IRC
	ga.socketfd_irc = socket_init("czat-app.onet.pl", 5015, msg_err);

	// w przypadku błędu pokaż komunikat oraz zakończ
	if(ga.socketfd_irc == 0)
	{
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# " + msg_err + "\n" xRED "# Błąd wystąpił w: ircAuth1");
		return;
	}

	// pobierz pierwszą odpowiedź serwera po połączeniu:
	// :cf1f4.onet NOTICE Auth :*** Looking up your hostname...
	irc_parser(ga, chan_parm);

	// w przypadku błędu komunikat został wyświetlony w parserze, wyświetl drugą część i zakończ
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd wystąpił w: ircAuth1");
		return;
	}

/*
	Część 2 autoryzacji.
*/
	// wyślij:
	// NICK <zuoUsername>
	irc_send(ga, chan_parm, "NICK " + ga.zuousername);

	// w przypadku błędu w irc_send() wyświetli błąd oraz pokaż drugi komunikat, gdzie wystąpił błąd i zakończ
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd wystąpił w: ircAuth2");
		return;
	}

	// pobierz drugą odpowiedź serwera:
	// :cf1f4.onet NOTICE Auth :*** Found your hostname (ajs7.neoplus.adsl.tpnet.pl) -- cached
	irc_parser(ga, chan_parm);

	// w przypadku błędu komunikat został wyświetlony w parserze, wyświetl drugą część i zakończ
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd wystąpił w: ircAuth2");
		return;
	}

/*
	Część 3 autoryzacji.
*/
	// wyślij:
	// AUTHKEY
	irc_send(ga, chan_parm, "AUTHKEY");

	// w przypadku błędu w irc_send() wyświetli błąd oraz pokaż drugi komunikat, gdzie wystąpił błąd i zakończ
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd wystąpił w: ircAuth3a");
		return;
	}

	// pobierz trzecią odpowiedź serwera:
	// :cf1f4.onet 801 ucc_test :<authKey>
	// parser wyszuka kod authKey, przeliczy na nowy kod i wyśle do serwera:
	// AUTHKEY <nowy_authKey>
	irc_parser(ga, chan_parm);

	// w przypadku błędu komunikat został wyświetlony w parserze, wyświetl drugą część i zakończ
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd wystąpił w: ircAuth3b");
		return;
	}

/*
	Część 4 autoryzacji.
*/
	// wyślij (zamiast zuoUsername można wysłać inną nazwę):
	// USER * <uoKey> czat-app.onet.pl :<zuoUsername>
	// PROTOCTL ONETNAMESX
	irc_send(ga, chan_parm, "USER * " + ga.uokey + " czat-app.onet.pl :" + ga.zuousername + "\r\nPROTOCTL ONETNAMESX");

	// w przypadku błędu w irc_send() wyświetli błąd oraz pokaż drugi komunikat, gdzie wystąpił błąd i zakończ
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd wystąpił w: ircAuth4");
		// bez return; bo to i tak koniec funkcji
	}

	// jeśli na którymś etapie wystąpił błąd, funkcja irc_auth() zakończy się z ga.irc_ok = false
}
