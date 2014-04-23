#include <sstream>		// std::string, std::stringstream, .find(), .insert(), .size()

#include "kbd_parser.hpp"
#include "auth.hpp"


bool kbd_parser(ucc_global_args *ucc_ga)
{
/*
	Prosty interpreter wpisywanych poleceń (zaczynających się od / na pierwszej pozycji).
*/

	// domyślnie nie zwracaj komunikatów (dodanie komunikatu następuje w obsłudze poleceń)
	ucc_ga->msg.clear();
	ucc_ga->msg_irc.clear();

	// domyślnie zakładamy, że wpisano polecenie
	ucc_ga->command_ok = true;

	// domyślnie zakładamy, że nie wpisaliśmy polecenia /me
	ucc_ga->command_me = false;

	// zapobiega wykonywaniu się reszty kodu, gdy w buforze nic nie ma
	if(ucc_ga->kbd_buf.size() == 0)
	{
		ucc_ga->msg = "# Błąd bufora klawiatury (bufor jest pusty)!";
		return false;
	}

	// wykryj, czy wpisano polecenie (znak / na pierwszej pozycji o tym świadczy)
	if(ucc_ga->kbd_buf[0] != '/')
	{
		// jeśli brak połączenia z IRC, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		if(! ucc_ga->irc_ok)
		{
			ucc_ga->msg = "# Najpierw się zaloguj.";
			return false;
		}

		// jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else if(! ucc_ga->channel_ok)
		{
			ucc_ga->msg = "# Nie jesteś w aktywnym pokoju.";
			return false;
		}

		// gdy połączono z IRC oraz jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w terminalu oraz polecenie do wysłania do IRC
		ucc_ga->command_ok = false;	// wpisano tekst do wysłania do aktywnego pokoju
		ucc_ga->msg = "<" + ucc_ga->zuousername + "> " + ucc_ga->kbd_buf;
		ucc_ga->msg_irc = "PRIVMSG " + ucc_ga->channel + " :" + ucc_ga->kbd_buf;
		return true;		// gdy wpisano zwykły tekst, zakończ
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
	f_command_status = find_command(ucc_ga->kbd_buf, f_command, f_command_org, pos_arg_start);

	// wykryj błędnie wpisane polecenie
	if(f_command_status == 1)
	{
		ucc_ga->msg = "# Polecenie błędne, sam znak / nie jest poleceniem.";
		return false;
	}

	else if(f_command_status == 2)
	{
		ucc_ga->msg = "# Polecenie błędne, po znaku / nie może być spacji.";
		return false;
	}

/*
	Wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej.
*/

	if(f_command == "CAPTCHA")
	{
		if(ucc_ga->irc_ok)
		{
			ucc_ga->msg = "# Już zalogowano się.";
			return false;
		}

		if(! ucc_ga->captcha_ready)
		{
			ucc_ga->msg = "# Najpierw wpisz /connect";
			return false;
		}

		// pobierz wpisany kod captcha
		find_arg(ucc_ga->kbd_buf, captcha, pos_arg_start, false);
		if(pos_arg_start == 0)
		{
			ucc_ga->msg = "# Nie podano kodu, spróbuj jeszcze raz.";
			return false;
		}

		if(captcha.size() != 6)
		{
			ucc_ga->msg = "# Kod musi mieć 6 znaków, spróbuj jeszcze raz.";
			return false;
		}

		// gdy kod wpisano i ma 6 znaków, wyślij go na serwer
		ucc_ga->captcha_ready = false;	// zapobiega ponownemu wysłaniu kodu na serwer
		if(! http_auth_checkcode(ucc_ga->cookies, captcha, ucc_ga->msg))
		{
			return false;		// w przypadku błędu wróć z komunikatem w msg
		}
		if(! http_auth_getuo(ucc_ga->cookies, ucc_ga->my_nick, ucc_ga->my_password, ucc_ga->zuousername, ucc_ga->uokey, ucc_ga->msg))
		{
			return false;		// w przypadku błędu wróć z komunikatem w msg
		}
		ucc_ga->irc_ready = true;	// gotowość do połączenia z IRC

	}	// CAPTCHA

	else if(f_command == "CONNECT")
	{
		if(ucc_ga->irc_ok)
		{
			ucc_ga->msg = "# Już zalogowano się.";
			return false;
		}

		if(ucc_ga->my_nick.size() == 0)
		{
			ucc_ga->msg = "# Nie wpisano nicka.";
			return false;
		}

		// gdy wpisano nick, rozpocznij łączenie
		if(! http_auth_init(ucc_ga->cookies, ucc_ga->msg))
		{
			return false;		// w przypadku błędu wróć z komunikatem w msg
		}

		// gdy wpisano hasło, wykonaj część dla nicka stałego
		if(ucc_ga->my_password.size() != 0)
		{
			if(! http_auth_getsk(ucc_ga->cookies, ucc_ga->msg))
			{
				return false;		// w przypadku błędu wróć z komunikatem w msg
			}
			if(! http_auth_mlogin(ucc_ga->cookies, ucc_ga->my_nick, ucc_ga->my_password, ucc_ga->msg))
			{
				return false;		// w przypadku błędu wróć z komunikatem w msg
			}
			if(! http_auth_getuo(ucc_ga->cookies, ucc_ga->my_nick, ucc_ga->my_password, ucc_ga->zuousername, ucc_ga->uokey, ucc_ga->msg))
			{
				return false;		// w przypadku błędu wróć z komunikatem w msg
			}
// dodać override jako polecenie, gdy wykryty zostanie zalogowany nick
/*
			if(! http_auth_useroverride(ucc_ga->cookies, ucc_ga->my_nick, ucc_ga->msg))
			{
				return false;		// w przypadku błędu wróć z komunikatem w msg
			}
*/
			ucc_ga->irc_ready = true;	// gotowość do połączenia z IRC
		}

		// gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
		else
		{
			// pobierz captcha
			if(! http_auth_getcaptcha(ucc_ga->cookies, ucc_ga->msg))
			{
				return false;		// w przypadku błędu wróć z komunikatem w msg
			}
			ucc_ga->msg = "# Przepisz kod z obrazka, w tym celu wpisz /captcha kod_z_obrazka";
			ucc_ga->captcha_ready = true;	// można przepisać kod i wysłać na serwer
		}

	}	// CONNECT

	else if(f_command == "DISCONNECT")
	{
		// jeśli nie ma połączenia z IRC, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
		if(! ucc_ga->irc_ok)
		{
			ucc_ga->msg = "# Nie zalogowano się.";
			return false;
		}

		// przygotuj polecenie do wysłania do IRC
		else
		{
			// jeśli podano argument (tekst pożegnalny), wstaw go
			if(rest_args(ucc_ga->kbd_buf, pos_arg_start, r_args))
			{
				ucc_ga->msg_irc = "QUIT :" + r_args;	// wstaw polecenie przed komunikatem pożegnalnym
			}
			// jeśli nie podano argumentu, wyślij samo polecenie
			else
			{
				ucc_ga->msg_irc = "QUIT";
			}
		}

	}	// DISCONNECT

	else if(f_command == "HELP")
	{
		ucc_ga->msg = "# Dostępne polecenia (w kolejności alfabetycznej):"
			      "\n/captcha"
			      "\n/connect"
			      "\n/disconnect"
			      "\n/help"
			      "\n/join lub /j"
			      "\n/me"
			      "\n/nick"
			      "\n/quit lub /q"
			      "\n/raw"
			      "\n/whois"
			      "\n/whowas";
			      // dopisać resztę poleceń

	}	// HELP

	else if(f_command == "JOIN" || f_command == "J")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(ucc_ga->irc_ok)
		{
			find_arg(ucc_ga->kbd_buf, ucc_ga->channel, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				ucc_ga->msg = "# Nie podano pokoju.";
				return false;
			}

			// gdy wpisano pokój, przygotuj komunikat do wysłania na serwer
			ucc_ga->channel_ok = true;	// nad tym jeszcze popracować, bo wpisanie pokoju wcale nie oznacza, że serwer go zaakceptuje
			// jeśli nie podano # przed nazwą pokoju, dodaj #
			if(ucc_ga->channel[0] != '#')
			{
				ucc_ga->channel.insert(0, "#");
			}
			ucc_ga->msg_irc = "JOIN " + ucc_ga->channel;
		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_connect_irc_err(ucc_ga->msg);
			return false;
		}

	}	// JOIN

	else if(f_command == "ME")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(ucc_ga->irc_ok)
		{
			// jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
			if(! ucc_ga->channel_ok)
			{
				ucc_ga->msg = "# Nie jesteś w aktywnym pokoju.";
				return false;
			}

			// jeśli jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w oknie terminala oraz polecenie dla IRC
			ucc_ga->command_me = true;	// polecenie to wymaga, aby komunikat wyświetlić zgodnie z kodowaniem bufora w ISO-8859-2
			rest_args(ucc_ga->kbd_buf, pos_arg_start, r_args);	// pobierz wpisany komunikat dla /me (nie jest niezbędny)
			ucc_ga->msg = "* " + ucc_ga->zuousername + " " + r_args;
			ucc_ga->msg_irc = "PRIVMSG " + ucc_ga->channel + " :\1ACTION " + r_args + "\1";
		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_connect_irc_err(ucc_ga->msg);
			return false;
		}

	}	// ME

	else if(f_command == "NICK")
	{
		// po połączeniu z IRC nie można zmienić nicka
		if(ucc_ga->irc_ok)
		{
			ucc_ga->msg = "# Po zalogowaniu się nie można zmienić nicka.";
			return false;
		}

		// nick można zmienić tylko, gdy nie jest się połączonym z IRC
		else
		{
			find_arg(ucc_ga->kbd_buf, f_arg, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				if(ucc_ga->my_nick.size() == 0)
				{
					ucc_ga->msg = "# Nie podano nicka.";
					return false;
				}
				// wpisanie /nick bez parametrów, gdy wcześniej był już podany, powoduje wypisanie aktualnego nicka
				else
				{
					if(ucc_ga->my_password.size() == 0)
					{
						ucc_ga->msg = "# Aktualny nick tymczasowy: " + ucc_ga->my_nick;
					}
					else
					{
						ucc_ga->msg = "# Aktualny nick stały: " + ucc_ga->my_nick;
					}
					return true;
				}
			}

			else if(f_arg.size() < 3)
			{
				ucc_ga->msg = "# Nick jest za krótki (minimalnie 3 znaki).";
				return false;
			}

			else if(f_arg.size() > 32)
			{
				ucc_ga->msg = "# Nick jest za długi (maksymalnie 32 znaki).";
				return false;
			}

			// jeśli za nickiem wpisano hasło, pobierz je do bufora
			find_arg(ucc_ga->kbd_buf, ucc_ga->my_password, pos_arg_start, false);

			// przepisz nick do zmiennej
			ucc_ga->my_nick = f_arg;
			if(ucc_ga->my_password.size() == 0)
			{
				ucc_ga->msg = "# Nowy nick tymczasowy: " + ucc_ga->my_nick;
			}
			else
			{
				ucc_ga->msg = "# Nowy nick stały: " + ucc_ga->my_nick;
			}

		}

	}	// NICK

	else if(f_command == "QUIT" || f_command == "Q")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(ucc_ga->irc_ok)
		{
			// jeśli podano argument (tekst pożegnalny), wstaw go
			if(rest_args(ucc_ga->kbd_buf, pos_arg_start, r_args))
			{
				ucc_ga->msg_irc = "QUIT :" + r_args;	// wstaw polecenie przed komunikatem pożegnalnym
			}
			// jeśli nie podano argumentu, wyślij samo polecenie
			else
			{
				ucc_ga->msg_irc = "QUIT";
			}
		}

		// zamknięcie programu po ewentualnym wysłaniu polecenia do IRC
		ucc_ga->ucc_quit = true;

	}	// QUIT

	else if(f_command == "RAW")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(ucc_ga->irc_ok)
		{
			// jeśli nie podano parametrów, pokaż ostrzeżenie
			if(! rest_args(ucc_ga->kbd_buf, pos_arg_start, r_args))
			{
				ucc_ga->msg = "# Nie podano parametrów.";
				return false;
			}
			// polecenie do IRC
			ucc_ga->msg_irc = r_args;
		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_connect_irc_err(ucc_ga->msg);
			return false;
		}

	}	// RAW

	else if(f_command == "WHOIS" || f_command == "WHOWAS")
	{
		// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
		if(ucc_ga->irc_ok)
		{
			find_arg(ucc_ga->kbd_buf, f_arg, pos_arg_start, false);
			if(pos_arg_start == 0)
			{
				ucc_ga->msg = "# Nie podano nicka do sprawdzenia.";
				return false;
			}

			// polecenie do IRC
			if(f_command == "WHOIS")	// jeśli WHOIS
			{
				ucc_ga->msg_irc = "WHOIS " + f_arg + " " + f_arg;	// 2x nick, aby pokazało idle
			}
			else		// jeśli WHOWAS (nie ma innej możliwości, dlatego bez else if)
			{
				ucc_ga->msg_irc = "WHOWAS " + f_arg;
			}

		}

		// jeśli nie połączono z IRC, pokaż ostrzeżenie
		else
		{
			msg_connect_irc_err(ucc_ga->msg);
			return false;
		}

	}	// WHOIS || WHOWAS

	// gdy nie znaleziono polecenia
	else
	{
		ucc_ga->msg = "# Nieznane polecenie: /" + f_command_org;	// tutaj pokaż oryginalnie wpisane polecenie
		return false;
	}

	return true;
}


void msg_connect_irc_err(std::string &msg)
{
	msg = "# Aby wysłać polecenie przeznaczone dla IRC, musisz się zalogować.";
}


int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start)
{
	// polecenie może się zakończyć spacją (polecenie z parametrem ) lub końcem bufora (polecenie bez parametru)

	// sprawdź, czy za poleceniem są jakieś znaki (sam znak / nie jest poleceniem)
	if(kbd_buf.size() <= 1)
		return 1;

	// sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
	if(kbd_buf[1] == ' ')
		return 2;

	size_t pos_command_end;		// pozycja, gdzie się kończy polecenie

	// wykryj pozycję końca polecenia
	pos_command_end = kbd_buf.find(" ");	// wykryj, gdzie jest spacja za poleceniem
	if(pos_command_end == std::string::npos)
		pos_command_end = kbd_buf.size();	// jeśli nie było spacji, koniec polecenia uznaje się za koniec bufora, czyli jego rozmiar

	f_command.clear();
	f_command.insert(0, kbd_buf, 1, pos_command_end - 1);	// wstaw szukane polecenie (- 1, bo pomijamy znak / )

	// znalezione polecenie zapisz w drugim buforze, który użyty zostanie, jeśli wpiszemy nieistniejące polecenie (pokazane będzie dokładnie tak,
	// jak je wpisaliśmy, bez konwersji małych liter na wielkie)
	f_command_org = f_command;

	// zamień małe litery w poleceniu na wielkie (łatwiej będzie je sprawdzać)
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

	// jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa),
	// więc zakończ
	if(pos_arg_start >= kbd_buf.size())
	{
		pos_arg_start = 0;	// 0 oznacza, że nie było argumentu
		return;
	}

	// pomiń spacje pomiędzy poleceniem a argumentem lub pomiędzy kolejnymi argumentami (z uwzględnieniem rozmiaru bufora, aby nie czytać poza nim)
	while(kbd_buf[pos_arg_start] == ' ' && pos_arg_start < kbd_buf.size())
		++pos_arg_start;	// kolejny znak w buforze

	// jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
	if(pos_arg_start == kbd_buf.size())
	{
		pos_arg_start = 0;	// 0 oznacza, że nie było argumentu
		return;
	}

	size_t pos_arg_end;

	// wykryj pozycję końca argumentu
	pos_arg_end = kbd_buf.find(" ", pos_arg_start);		// wykryj, gdzie jest spacja za poleceniem lub poprzednim argumentem
	if(pos_arg_end == std::string::npos)
		pos_arg_end = kbd_buf.size();	// jeśli nie było spacji, koniec argumentu uznaje się za koniec bufora, czyli jego rozmiar

	f_arg.insert(0, kbd_buf, pos_arg_start, pos_arg_end - pos_arg_start);	// wstaw szukany argument

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

	// jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa),
	// więc zakończ
	if(pos_arg_start >= kbd_buf.size())
		return false;

	// znajdź miejsce, od którego zaczynają się znaki różne od spacji
	while(kbd_buf[pos_arg_start] == ' ' && pos_arg_start < kbd_buf.size())
		++pos_arg_start;

	// jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
	if(pos_arg_start == kbd_buf.size())
		return false;

	// wstaw pozostałą część bufora
	f_rest.clear();
	f_rest.insert(0, kbd_buf, pos_arg_start, kbd_buf.size() - pos_arg_start);

	return true;
}
