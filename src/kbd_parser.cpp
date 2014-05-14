#include <sstream>		// std::string, std::stringstream, .find(), .insert(), .size()

#include "kbd_parser.hpp"
#include "window_utils.hpp"
#include "auth.hpp"


int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start)
{
/*
	Pobierz wpisane polecenie do bufora f_command (przekonwertowane na wielkie litery) oraz do f_command_org (bez konwersji).
	Polecenie może się zakończyć spacją (polecenie z parametrem ) lub końcem bufora (polecenie bez parametru).
*/

	// sprawdź, czy za poleceniem są jakieś znaki (sam znak / nie jest poleceniem)
	if(kbd_buf.size() <= 1)
	{
		return 1;
	}

	// sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
	if(kbd_buf[1] == ' ')
	{
		return 2;
	}

	size_t pos_command_end;		// pozycja, gdzie się kończy polecenie

	// wykryj pozycję końca polecenia (gdzie jest spacja za poleceniem)
	pos_command_end = kbd_buf.find(" ");

	// jeśli nie było spacji, koniec polecenia uznaje się za koniec bufora, czyli jego rozmiar
	if(pos_command_end == std::string::npos)
	{
		pos_command_end = kbd_buf.size();
	}

	f_command.clear();
	f_command.insert(0, kbd_buf, 1, pos_command_end - 1);	// wstaw szukane polecenie (- 1, bo pomijamy znak / )

	// znalezione polecenie zapisz w drugim buforze, który użyty zostanie, jeśli wpiszemy nieistniejące polecenie (pokazane będzie dokładnie tak,
	// jak je wpisaliśmy, bez konwersji małych liter na wielkie)
	f_command_org = f_command;

	// zamień małe litery w poleceniu na wielkie (łatwiej będzie wyszukać polecenie)
	for(int i = 0; i < static_cast<int>(f_command.size()); ++i)	// static_cast<int> - rzutowanie size_t (w tym przypadku) na int
	{
		if(islower(f_command[i]))
		{
			f_command[i] = toupper(f_command[i]);
		}
	}

	// gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w find_arg() )
	pos_arg_start = pos_command_end;

	return 0;
}


void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper)
{
/*
	Pobierz argument z bufora klawiatury od pozycji w pos_arg_start, jeśli go nie ma, w pos_arg_start będzie 0.
*/

	// wstępnie usuń ewentualną poprzednią zawartość po użyciu tej funkcji
	f_arg.clear();

	// jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu
	// (tym bardziej, gdy jest większa), więc zakończ
	if(pos_arg_start >= kbd_buf.size())
	{
		pos_arg_start = 0;	// 0 oznacza, że nie było argumentu
		return;
	}

	// pomiń spacje pomiędzy poleceniem a argumentem lub pomiędzy kolejnymi argumentami (z uwzględnieniem rozmiaru bufora, aby nie czytać poza niego)
	while(kbd_buf[pos_arg_start] == ' ' && pos_arg_start < kbd_buf.size())
	{
		++pos_arg_start;	// kolejny znak w buforze
	}

	// jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
	if(pos_arg_start == kbd_buf.size())
	{
		pos_arg_start = 0;	// 0 oznacza, że nie było argumentu
		return;
	}

	size_t pos_arg_end;

	// wykryj pozycję końca argumentu (gdzie jest spacja za poleceniem lub poprzednim argumentem)
	pos_arg_end = kbd_buf.find(" ", pos_arg_start);

	// jeśli nie było spacji, koniec argumentu uznaje się za koniec bufora, czyli jego rozmiar
	if(pos_arg_end == std::string::npos)
	{
		pos_arg_end = kbd_buf.size();
	}

	// wstaw szukany argument
	f_arg.insert(0, kbd_buf, pos_arg_start, pos_arg_end - pos_arg_start);

	// jeśli trzeba, zamień małe litery w argumencie na wielkie
	if(lower2upper)
	{
		for(int i = 0; i < static_cast<int>(f_arg.size()); ++i)
		{
			if(islower(f_arg[i]))
			{
				f_arg[i] = toupper(f_arg[i]);
			}
		}
	}

	// wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
	pos_arg_start = pos_arg_end;
}


bool rest_args(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest)
{
/*
	Pobierz resztę bufora od pozycji w pos_arg_start (czyli pozostałe argumenty lub argument).
*/

	// jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu
	// (tym bardziej, gdy jest większa), więc zakończ
	if(pos_arg_start >= kbd_buf.size())
	{
		return false;
	}

	// znajdź miejsce, od którego zaczynają się znaki różne od spacji
	while(kbd_buf[pos_arg_start] == ' ' && pos_arg_start < kbd_buf.size())
	{
		++pos_arg_start;
	}

	// jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
	if(pos_arg_start == kbd_buf.size())
	{
		return false;
	}

	// wstaw pozostałą część bufora
	f_rest.clear();
	f_rest.insert(0, kbd_buf, pos_arg_start, kbd_buf.size() - pos_arg_start);

	return true;
}


std::string msg_connect_irc_err()
{
	return get_time() + "\x3\x1# Aby wysłać polecenie przeznaczone dla IRC, musisz zalogować się.";
}


void kbd_parser(std::string &kbd_buf, std::string &msg_scr, std::string &msg_irc, std::string &my_nick, std::string &my_password, std::string &cookies,
		std::string &zuousername, std::string &uokey, bool &captcha_ready, bool &irc_ready, bool &irc_ok, bool &channel_ok, std::string &channel,
		bool &ucc_quit)
{
/*
	Prosty interpreter wpisywanych poleceń (zaczynających się od / na pierwszej pozycji).
*/

	// domyślnie nie zwracaj komunikatów (dodanie komunikatu następuje w obsłudze poleceń)
	msg_scr.clear();
	msg_irc.clear();

	// zapobiega wykonywaniu się reszty kodu, gdy w buforze nic nie ma
	if(kbd_buf.size() == 0)
	{
		msg_scr = get_time() + "\x3\x1# Błąd bufora klawiatury (bufor jest pusty)!";
		return;
	}

	// wykryj, czy wpisano polecenie (znak / na pierwszej pozycji o tym świadczy)
	if(kbd_buf[0] != '/')
	{
		// jeśli brak połączenia z IRC, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		if(! irc_ok)
		{
			msg_scr = get_time() + "\x3\x1# Najpierw zaloguj się.";
			return;
		}

		// jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else if(! channel_ok)
		{
			msg_scr = get_time() + "\x3\x1# Nie jesteś w aktywnym pokoju.";
			return;
		}

		// gdy połączono z IRC oraz jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w terminalu oraz polecenie do wysłania do IRC
//		command_ok = false;	// wpisano tekst do wysłania do aktywnego pokoju
		msg_scr = get_time() + "\x04<" + buf_iso2utf(zuousername) + ">\x05 " + buf_iso2utf(kbd_buf);
		msg_irc = "PRIVMSG " + channel + " :" + kbd_buf;
		return;		// gdy wpisano zwykły tekst, zakończ
	}

/*
	Wpisano / na początku, więc pobierz polecenie do bufora.
*/

	int f_command_status;
	size_t pos_arg_start = 1;		// pozycja początkowa kolejnego argumentu
	std::string f_command;			// znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
	std::string f_command_org;		// j/w, ale małe litery nie są zamieniane na wielkie
	std::string f_arg;			// kolejne argumenty podane za poleceniem
	std::string r_args;			// pozostałe argumentu lub argument od pozycji w pos_arg_start zwracane w rest_args()
	std::string captcha, err_code;
	std::stringstream http_status_str;	// użyty pośrednio do zamiany int na std::string

	// pobierz wpisane polecenie
	f_command_status = find_command(kbd_buf, f_command, f_command_org, pos_arg_start);

	// wykryj błędnie wpisane polecenie
	if(f_command_status == 1)
	{
		msg_scr = get_time() + "\x3\x1# Polecenie błędne, sam znak / nie jest poleceniem.";
		return;
	}

	else if(f_command_status == 2)
	{
		msg_scr = get_time() + "\x3\x1# Polecenie błędne, po znaku / nie może być spacji.";
		return;
	}

/*
	Wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej.
*/

	if(f_command == "CAPTCHA")
	{
		if(irc_ok)
		{
			msg_scr = get_time() + "\x3\x1# Już zalogowano się.";
			return;
		}

		if(! captcha_ready)
		{
			msg_scr = get_time() + "\x3\x1# Najpierw wpisz /connect";
			return;
		}

		// pobierz wpisany kod captcha
		find_arg(kbd_buf, captcha, pos_arg_start, false);
		if(pos_arg_start == 0)
		{
			msg_scr = get_time() + "\x3\x1# Nie podano kodu, spróbuj jeszcze raz.";
			return;
		}

		if(captcha.size() != 6)
		{
			msg_scr = get_time() + "\x3\x1# Kod musi mieć 6 znaków, spróbuj jeszcze raz.";
			return;
		}

		// gdy kod wpisano i ma 6 znaków, wyślij go na serwer
		captcha_ready = false;	// zapobiega ponownemu wysłaniu kodu na serwer
		if(! http_auth_checkcode(cookies, captcha, msg_scr))
		{
			return;		// w przypadku błędu wróć z komunikatem w msg_scr
		}
		if(! http_auth_getuo(cookies, my_nick, my_password, zuousername, uokey, msg_scr))
		{
			return;		// w przypadku błędu wróć z komunikatem w msg_scr
		}

		irc_ready = true;	// gotowość do połączenia z IRC

	}	// CAPTCHA

	else if(f_command == "CONNECT")
	{
		if(irc_ok)
		{
			msg_scr = get_time() + "\x3\x1# Już zalogowano się.";
			return;
		}

		if(my_nick.size() == 0)
		{
			msg_scr = get_time() + "\x3\x1# Nie wpisano nicka.";
			return;
		}

		// gdy wpisano nick, rozpocznij łączenie
		if(! http_auth_init(cookies, msg_scr))
		{
			return;		// w przypadku błędu wróć z komunikatem w msg_scr
		}

		// gdy wpisano hasło, wykonaj część dla nicka stałego
		if(my_password.size() != 0)
		{
			if(! http_auth_getsk(cookies, msg_scr))
			{
				return;		// w przypadku błędu wróć z komunikatem w msg_scr
			}

			if(! http_auth_mlogin(cookies, my_nick, my_password, msg_scr))
			{
				return;		// w przypadku błędu wróć z komunikatem w msg_scr
			}

			if(! http_auth_getuo(cookies, my_nick, my_password, zuousername, uokey, msg_scr))
			{
				return;		// w przypadku błędu wróć z komunikatem w msg_scr
			}

/*
			// dodać override jako polecenie, gdy wykryty zostanie zalogowany nick
			if(! http_auth_useroverride(cookies, my_nick, msg_scr))
			{
				return;		// w przypadku błędu wróć z komunikatem w msg_scr
			}
*/

			irc_ready = true;	// gotowość do połączenia z IRC
		}

		// gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
		else
		{
			// pobierz captcha
			if(! http_auth_getcaptcha(cookies, msg_scr))
			{
				return;		// w przypadku błędu wróć z komunikatem w msg_scr
			}

			msg_scr = get_time() + "\x3\x2# Przepisz kod z obrazka, w tym celu wpisz /captcha kod_z_obrazka";
			captcha_ready = true;	// można przepisać kod i wysłać na serwer
		}

	}	// CONNECT

	else if(f_command == "DISCONNECT")
	{
		// jeśli nie ma połączenia z IRC, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
		if(! irc_ok)
		{
			msg_scr = get_time() + "\x3\x1# Nie zalogowano się.";
			return;
		}

		// przygotuj polecenie do wysłania do IRC
		else
		{
			// jeśli podano argument (tekst pożegnalny), wstaw go
			if(rest_args(kbd_buf, pos_arg_start, r_args))
			{
				msg_irc = "QUIT :" + r_args;	// wstaw polecenie przed komunikatem pożegnalnym
			}

			// jeśli nie podano argumentu, wyślij samo polecenie
			else
			{
				msg_irc = "QUIT";
			}
		}

	}	// DISCONNECT

	else if(f_command == "HELP")
	{
		msg_scr =	get_time() + "\x3\x2# Dostępne polecenia (w kolejności alfabetycznej):" +
				get_time() + "\x3\x6/captcha" +
				get_time() + "\x3\x6/connect" +
				get_time() + "\x3\x6/disconnect" +
				get_time() + "\x3\x6/help" +
				get_time() + "\x3\x6/join lub /j" +
				get_time() + "\x3\x6/me" +
				get_time() + "\x3\x6/nick" +
				get_time() + "\x3\x6/quit lub /q" +
				get_time() + "\x3\x6/raw" +
				get_time() + "\x3\x6/vhost" +
				get_time() + "\x3\x6/whois" +
				get_time() + "\x3\x6/whowas";
				// dopisać resztę poleceń
		return;

	}	// HELP

	else if(f_command == "JOIN" || f_command == "J")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(irc_ok)
		{
			find_arg(kbd_buf, channel, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				msg_scr = get_time() + "\x3\x1# Nie podano pokoju.";
				return;
			}

			// gdy wpisano pokój, przygotuj komunikat do wysłania na serwer
			channel_ok = true;	// nad tym jeszcze popracować, bo wpisanie pokoju wcale nie oznacza, że serwer go zaakceptuje
			if(channel[0] != '#')	// jeśli nie podano # przed nazwą pokoju, dodaj #
			{
				channel.insert(0, "#");
			}

			msg_irc = "JOIN " + channel;
		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_scr = msg_connect_irc_err();
			return;
		}

	}	// JOIN

	else if(f_command == "ME")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(irc_ok)
		{
			// jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
			if(! channel_ok)
			{
				msg_scr = get_time() + "\x3\x1# Nie jesteś w aktywnym pokoju.";
				return;
			}

			// jeśli jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w oknie terminala oraz polecenie dla IRC
//			command_me = true;	// polecenie to wymaga, aby komunikat wyświetlić zgodnie z kodowaniem bufora w ISO-8859-2
			rest_args(kbd_buf, pos_arg_start, r_args);	// pobierz wpisany komunikat dla /me (nie jest niezbędny)
			msg_scr = get_time() + "\x3\x5\x4* " + buf_iso2utf(zuousername) + "\x3\x8\x5 " + buf_iso2utf(r_args);
			msg_irc = "PRIVMSG " + channel + " :\x1""ACTION " + r_args + "\x1";
		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_scr = msg_connect_irc_err();
			return;
		}

	}	// ME

	else if(f_command == "NICK")
	{
		// po połączeniu z IRC nie można zmienić nicka
		if(irc_ok)
		{
			msg_scr = get_time() + "\x3\x1# Po zalogowaniu się nie można zmienić nicka.";
			return;
		}

		// nick można zmienić tylko, gdy nie jest się połączonym z IRC
		else
		{
			find_arg(kbd_buf, f_arg, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				if(my_nick.size() == 0)
				{
					msg_scr = get_time() + "\x3\x1# Nie podano nicka.";
					return;
				}

				// wpisanie /nick bez parametrów, gdy wcześniej był już podany, powoduje wypisanie aktualnego nicka
				else
				{
					if(my_password.size() == 0)
					{
						msg_scr = get_time() + "\x3\x2# Aktualny nick tymczasowy: " + buf_iso2utf(my_nick);
					}

					else
					{
						msg_scr = get_time() + "\x3\x2# Aktualny nick stały: " + buf_iso2utf(my_nick);
					}

					return;
				}
			}

			else if(f_arg.size() < 3)
			{
				msg_scr = get_time() + "\x3\x1# Nick jest za krótki (minimalnie 3 znaki).";
				return;
			}

			else if(f_arg.size() > 32)
			{
				msg_scr = get_time() + "\x3\x1# Nick jest za długi (maksymalnie 32 znaki).";
				return;
			}

			// jeśli za nickiem wpisano hasło, pobierz je do bufora
			find_arg(kbd_buf, my_password, pos_arg_start, false);

			// przepisz nick do zmiennej
			my_nick = f_arg;
			if(my_password.size() == 0)
			{
				msg_scr = get_time() + "\x3\x2# Nowy nick tymczasowy: " + buf_iso2utf(my_nick);
			}

			else
			{
				msg_scr = get_time() + "\x3\x2# Nowy nick stały: " + buf_iso2utf(my_nick);
			}

		}

	}	// NICK

	else if(f_command == "QUIT" || f_command == "Q")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(irc_ok)
		{
			// jeśli podano argument (tekst pożegnalny), wstaw go
			if(rest_args(kbd_buf, pos_arg_start, r_args))
			{
				msg_irc = "QUIT :" + r_args;	// wstaw polecenie przed komunikatem pożegnalnym
			}

			// jeśli nie podano argumentu, wyślij samo polecenie
			else
			{
				msg_irc = "QUIT";
			}

		}

		// zamknięcie programu po ewentualnym wysłaniu polecenia do IRC
		ucc_quit = true;

	}	// QUIT

	else if(f_command == "RAW")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(irc_ok)
		{
			// jeśli nie podano parametrów, pokaż ostrzeżenie
			if(! rest_args(kbd_buf, pos_arg_start, r_args))
			{
				msg_scr = get_time() + "\x3\x1# Nie podano parametrów.";
				return;
			}

			// polecenie do IRC
			msg_irc = r_args;

		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_scr = msg_connect_irc_err();
			return;
		}

	}	// RAW

	else if(f_command == "VHOST")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(irc_ok)
		{
			// jeśli nie podano parametrów, pokaż ostrzeżenie
			find_arg(kbd_buf, f_arg, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				msg_scr = get_time() + "\x3\x1# Nie podano parametrów (nicka i hasła do VHost).";
				return;
			}

			// jeśli podano pierwszy argument (nick), przepisz go
			msg_irc = "VHOST " + f_arg + " ";

			// wykryj wpisanie hasła, jeśli nie wpisano go, pokaż ostrzeżenie
			find_arg(kbd_buf, f_arg, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				msg_scr = get_time() + "\x3\x1# Nie podano hasła do VHost.";
				msg_irc.clear();	// brak hasła, dlatego wyczyść bufor IRC, aby nie wysłać nic na serwer
				return;
			}

			// jeśli podano drugi argument (hasło), dopisz je
			msg_irc += f_arg;
		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_scr = msg_connect_irc_err();
			return;
		}

	}	// VHOST

	else if(f_command == "WHOIS" || f_command == "WHOWAS")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(irc_ok)
		{
			find_arg(kbd_buf, f_arg, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				msg_scr = get_time() + "\x3\x1# Nie podano nicka do sprawdzenia.";
				return;
			}

			// polecenie do IRC
			if(f_command == "WHOIS")	// jeśli WHOIS
			{
				msg_irc = "WHOIS " + f_arg + " " + f_arg;	// 2x nick, aby pokazało idle
			}

			else		// jeśli WHOWAS (nie ma innej możliwości, dlatego bez else if)
			{
				msg_irc = "WHOWAS " + f_arg;
			}

		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_scr = msg_connect_irc_err();
			return;
		}

	}	// WHOIS || WHOWAS

	// gdy nie znaleziono polecenia
	else
	{
		// tutaj pokaż oryginalnie wpisane polecenie z uwzględnieniem przekodowania na UTF-8
		msg_scr = get_time() + "\x3\x1# Nieznane polecenie: /" + buf_iso2utf(f_command_org);
	}

}
