#include <string>		// std::string

#include "kbd_parser.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


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
	while(pos_arg_start < kbd_buf.size() && kbd_buf[pos_arg_start] == ' ')
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
	while(pos_arg_start < kbd_buf.size() && kbd_buf[pos_arg_start] == ' ')
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


void kbd_parser(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf)
{
/*
	Prosty interpreter wpisywanych poleceń (zaczynających się od / na pierwszej pozycji).
*/

	// zapobiega wykonywaniu się reszty kodu, gdy w buforze nic nie ma
	if(kbd_buf.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd bufora klawiatury (bufor jest pusty)!");
		return;
	}

	// wykryj, czy wpisano polecenie (znak / na pierwszej pozycji o tym świadczy), dwa znaki // również traktuj jak zwykły tekst
	// (np. w przypadku emotikon)
	if(kbd_buf[0] != '/' || (kbd_buf.size() > 1 && kbd_buf[1] == '/'))
	{
		// jeśli brak połączenia z IRC, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		if(! ga.irc_ok)
		{
			msg_connect_irc_err(ga, chan_parm);
			return;
		}

		// jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else if(! chan_parm[ga.current]->channel_ok)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju.");
			return;
		}

		// gdy połączono z IRC oraz jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w terminalu (jeśli wpisano coś w formatowaniu
		// zwracanym przez serwer, funkcja form_from_chat() przekształci to tak, aby w terminalu wyświetlić to, jakby było to odebrane z serwera)
		std::string term_buf = buf_iso2utf(kbd_buf);

		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xBOLD_ON "<" + buf_iso2utf(ga.zuousername) + ">" xNORMAL " " + form_from_chat(term_buf));

		// wyślij też komunikat do serwera IRC
		irc_send(ga, chan_parm, "PRIVMSG " + buf_utf2iso(chan_parm[ga.current]->channel) + " :" + form_to_chat(kbd_buf));

		return;		// gdy wpisano zwykły tekst, zakończ
	}

/*
	Wpisano / na początku, więc pobierz polecenie do bufora.
*/

	int f_command_status;
	size_t pos_arg_start = 1;		// pozycja początkowa kolejnego argumentu
	std::string f_command;			// znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
	std::string f_command_org;		// j/w, ale małe litery nie są zamieniane na wielkie

	// pobierz wpisane polecenie
	f_command_status = find_command(kbd_buf, f_command, f_command_org, pos_arg_start);

	// wykryj błędnie wpisane polecenie
	if(f_command_status == 1)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Polecenie błędne, sam znak / nie jest poleceniem.");
		return;
	}

	else if(f_command_status == 2)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Polecenie błędne, po znaku / nie może być spacji.");
		return;
	}

/*
	Wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej.
*/
	if(f_command == "AWAY")
	{
		kbd_command_away(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "BUSY")
	{
		kbd_command_busy(ga, chan_parm);
	}

	else if(f_command == "CAPTCHA" || f_command == "CAP")
	{
		kbd_command_captcha(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "CARD")
	{
		kbd_command_card(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "CONNECT" || f_command == "C")
	{
		kbd_command_connect(ga, chan_parm);
	}

	else if(f_command == "DISCONNECT" || f_command == "D")
	{
		kbd_command_disconnect(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "HELP" || f_command == "H")
	{
		kbd_command_help(ga, chan_parm);
	}

	else if(f_command == "JOIN" || f_command == "J")
	{
		kbd_command_join(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "ME")
	{
		kbd_command_me(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "NAMES" || f_command == "N")
	{
		kbd_command_names(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "NICK")
	{
		kbd_command_nick(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "PART" || f_command == "P")
	{
		kbd_command_part(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "PRIV")
	{
		kbd_command_priv(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "QUIT" || f_command == "Q")
	{
		kbd_command_quit(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "RAW")
	{
		kbd_command_raw(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "TIME")
	{
		kbd_command_time(ga, chan_parm);
	}

	else if(f_command == "TOPIC")
	{
		kbd_command_topic(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "VHOST")
	{
		kbd_command_vhost(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "WHOIS")
	{
		kbd_command_whois(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	else if(f_command == "WHOWAS")
	{
		kbd_command_whowas(ga, chan_parm, kbd_buf, pos_arg_start);
	}

	// gdy nie znaleziono polecenia, pokaż je wraz z ostrzeżeniem
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nieznane polecenie: /" + buf_iso2utf(f_command_org));
	}
}


void msg_connect_irc_err(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Najpierw zaloguj się.");
}


/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie (w kolejności alfabetycznej).
*/


void kbd_command_away(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// jeśli podano powód nieobecności, wyślij go
		if(rest_args(kbd_buf, pos_arg_start, r_args))
		{
			irc_send(ga, chan_parm, "AWAY :" + r_args);
		}

		// w przeciwnym razie wyślij bez powodu (co wyłączy status nieobecności)
		else
		{
			irc_send(ga, chan_parm, "AWAY");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_busy(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// zmień stan flagi busy
		if(ga.busy_state)
		{
			ga.busy_state = false;
			irc_send(ga, chan_parm, "BUSY 0");
		}

		else
		{
			ga.busy_state = true;
			irc_send(ga, chan_parm, "BUSY 1");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_captcha(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Już zalogowano się.");
		return;
	}

	if(! ga.captcha_ready)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Najpierw wpisz " xCYAN "/connect " xRED "lub " xCYAN "/c");
		return;
	}

	// pobierz wpisany kod captcha
	find_arg(kbd_buf, f_arg, pos_arg_start, false);

	if(pos_arg_start == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano kodu, spróbuj jeszcze raz.");
		return;
	}

	if(f_arg.size() != 6)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Kod musi mieć 6 znaków, spróbuj jeszcze raz.");
		return;
	}

	// gdy kod wpisano i ma 6 znaków, wyślij go na serwer
	ga.captcha_ready = false;	// zapobiega ponownemu wysłaniu kodu na serwer

	if(! http_auth_checkcode(ga, chan_parm, f_arg))
	{
		return;
	}

	if(! http_auth_getuokey(ga, chan_parm))
	{
		return;
	}

	ga.irc_ready = true;	// gotowość do połączenia z IRC
}


void kbd_command_card(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		// polecenie do IRC (można nie podawać nicka, wtedy pokaże własną wizytówkę)
		irc_send(ga, chan_parm, "NS INFO " + f_arg);

		ga.command_card = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}

}


void kbd_command_connect(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Już zalogowano się.");
		return;
	}

	if(ga.my_nick.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie wpisano nicka.");
		return;
	}

	// komunikat o łączeniu we wszystkich otwartych pokojach z wyjątkiem "Debug"
	for(int i = 0; i < CHAN_MAX - 1; ++i)
	{
		if(chan_parm[i])
		{
			win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, xBOLD_ON xGREEN "# Łączę z czatem...");

			// aktywność typu 1
			chan_act_add(chan_parm, chan_parm[i]->channel, 1);
		}
	}

	// skasuj użycie wybranych flag poleceń w celu odpowiedniego sterowania wyświetlaniem komunikatów podczas logowania na czat
	ga.command_card = false;
	ga.command_join = false;
	ga.command_names = false;
	ga.command_vhost = false;

	// gdy wpisano nick, rozpocznij łączenie
	if(! http_auth_init(ga, chan_parm))
	{
		return;
	}

	// gdy wpisano hasło, wykonaj część dla nicka stałego
	if(ga.my_password.size() != 0)
	{
		if(! http_auth_getsk(ga, chan_parm))
		{
			return;
		}

		if(! http_auth_mlogin(ga, chan_parm))
		{
			return;
		}

		if(! http_auth_getuokey(ga, chan_parm))
		{
			return;
		}

/*
		// dodać override jako polecenie, gdy wykryty zostanie zalogowany nick
		if(! http_auth_useroverride(ga, chan_parm))
		{
			return;
		}
*/

		ga.irc_ready = true;	// gotowość do połączenia z IRC
	}

	// gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
	else
	{
		// pobierz captcha
		if(! http_auth_getcaptcha(ga, chan_parm))
		{
			return;
		}

		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xGREEN "# Przepisz kod z obrazka, w tym celu wpisz " xCYAN "/captcha kod_z_obrazka " xGREEN "lub "
				xCYAN "/cap kod_z_obrazka");

		ga.captcha_ready = true;	// można przepisać kod i wysłać na serwer
	}
}


void kbd_command_disconnect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli nie ma połączenia z IRC, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
	if(! ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie zalogowano się.");
	}

	// przygotuj polecenie do wysłania do IRC
	else
	{
		// jeśli podano argument (tekst pożegnalny), wstaw go
		if(rest_args(kbd_buf, pos_arg_start, r_args))
		{
			irc_send(ga, chan_parm, "QUIT :" + r_args);
		}

		// jeśli nie podano argumentu, wyślij samo polecenie
		else
		{
			irc_send(ga, chan_parm, "QUIT");
		}
	}
}


void kbd_command_help(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xGREEN "# Dostępne polecenia (w kolejności alfabetycznej):\n"
				xCYAN  "/away\n"
				xCYAN  "/busy\n"
				xCYAN  "/captcha " xGREEN "lub " xCYAN "/cap\n"
				xCYAN  "/card\n"
				xCYAN  "/connect " xGREEN "lub " xCYAN "/c\n"
				xCYAN  "/disconnect " xGREEN "lub " xCYAN "/d\n"
				xCYAN  "/help " xGREEN "lub " xCYAN "/h\n"
				xCYAN  "/join " xGREEN "lub " xCYAN "/j\n"
				xCYAN  "/me\n"
				xCYAN  "/names " xGREEN "lub " xCYAN "/n\n"
				xCYAN  "/nick\n"
				xCYAN  "/part " xGREEN "lub " xCYAN "/p\n"
				xCYAN  "/priv\n"
				xCYAN  "/quit " xGREEN "lub " xCYAN "/q\n"
				xCYAN  "/raw\n"
				xCYAN  "/time\n"
				xCYAN  "/topic\n"
				xCYAN  "/vhost\n"
				xCYAN  "/whois\n"
				xCYAN  "/whowas");
				// dopisać resztę poleceń
}


void kbd_command_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		if(pos_arg_start == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano pokoju.");
			return;
		}

		// jeśli nie podano # przed nazwą pokoju, dodaj #, ale jeśli jest ^, to nie dodawaj (aby można było wejść na priv)
		if(f_arg[0] != '#' && f_arg[0] != '^')
		{
			f_arg.insert(0, "#");
		}

		irc_send(ga, chan_parm, "JOIN " + f_arg);

		ga.command_join = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_me(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		if(! chan_parm[ga.current]->channel_ok)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju.");
			return;
		}

		rest_args(kbd_buf, pos_arg_start, r_args);	// pobierz wpisany komunikat dla /me (nie jest niezbędny)

		// jeśli jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w oknie terminala
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xBOLD_ON xMAGENTA "* " + buf_iso2utf(ga.zuousername) + xNORMAL " " + buf_iso2utf(r_args));

		// wyślij też komunikat do serwera IRC
		irc_send(ga, chan_parm, "PRIVMSG " + buf_utf2iso(chan_parm[ga.current]->channel) + " :\x01" "ACTION " + r_args + "\x01");
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_names(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// /names może działać z parametrem lub bez (wtedy dotyczy aktywnego pokoju)
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		// gdy podano pokój do sprawdzenia
		if(pos_arg_start)
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #
			if(f_arg[0] != '#')
			{
				f_arg.insert(0, "#");
			}

			// wyślij komunikat do serwera IRC
			irc_send(ga, chan_parm, "NAMES " + f_arg);
		}

		// gdy nie podano sprawdź, czy pokój jest aktywny (bo /names dla "Status" lub "Debug" nie ma sensu)
		else
		{
			if(chan_parm[ga.current]->channel_ok)
			{
				irc_send(ga, chan_parm, "NAMES " + buf_utf2iso(chan_parm[ga.current]->channel));
			}

			else
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# To nie jest aktywny pokój czata.");
			}
		}

		ga.command_names = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// po połączeniu z IRC nie można zmienić nicka
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Po zalogowaniu się nie można zmienić nicka.");
		return;
	}

	// nick można zmienić tylko, gdy nie jest się połączonym z IRC
	else
	{
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		if(pos_arg_start == 0)
		{
			if(ga.my_nick.size() == 0)
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano nicka.");
				return;
			}

			// wpisanie /nick bez parametrów, gdy wcześniej był już podany, powoduje wypisanie aktualnego nicka
			else
			{
				if(ga.my_password.size() == 0)
				{
					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xGREEN "# Aktualny nick tymczasowy: " + buf_iso2utf(ga.my_nick));
				}

				else
				{
					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xGREEN "# Aktualny nick stały: " + buf_iso2utf(ga.my_nick));
				}

				return;
			}
		}

		else if(f_arg.size() < 3)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nick jest za krótki (minimalnie 3 znaki).");
			return;
		}

		else if(f_arg.size() > 32)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nick jest za długi (maksymalnie 32 znaki).");
			return;
		}

		// jeśli za nickiem wpisano hasło, pobierz je do bufora
		find_arg(kbd_buf, ga.my_password, pos_arg_start, false);

		// przepisz nick do zmiennej
		ga.my_nick = f_arg;

		// wyświetl wpisany nick
		if(ga.my_password.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xGREEN "# Nowy nick tymczasowy: " + buf_iso2utf(ga.my_nick));
		}

		else
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xGREEN "# Nowy nick stały: " + buf_iso2utf(ga.my_nick));
		}
	}
}


void kbd_command_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// wyślij polecenie, gdy to aktywny pokój czata, a nie "Status" lub "Debug"
		if(chan_parm[ga.current] && chan_parm[ga.current]->channel_ok)
		{
			// jeśli podano powód wyjścia, wyślij go
			if(rest_args(kbd_buf, pos_arg_start, r_args))
			{
				irc_send(ga, chan_parm, "PART " + buf_utf2iso(chan_parm[ga.current]->channel) + " :" + r_args);
			}

			// w przeciwnym razie wyślij samo polecenie
			else
			{
				irc_send(ga, chan_parm, "PART " + buf_utf2iso(chan_parm[ga.current]->channel));
			}
		}

		// w przeciwnym razie pokaż ostrzeżenie
		else
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju czata.");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_priv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// pobierz nick, który zapraszamy do rozmowy prywatnej
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(pos_arg_start == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano nicka zapraszanego do rozmowy prywatnej.");
			return;
		}

		// wyślij polecenie do IRC
		irc_send(ga, chan_parm, "PRIV " + f_arg);
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// jeśli podano powód wyjścia, wyślij go
		if(rest_args(kbd_buf, pos_arg_start, r_args))
		{
			irc_send(ga, chan_parm, "QUIT :" + r_args);
		}

		// jeśli nie podano argumentu, wyślij samo polecenie
		else
		{
			irc_send(ga, chan_parm, "QUIT");
		}
	}

	// zamknięcie programu po ewentualnym wysłaniu polecenia do IRC
	ga.ucc_quit = true;
}


void kbd_command_raw(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// jeśli nie podano parametrów, pokaż ostrzeżenie
		if(! rest_args(kbd_buf, pos_arg_start, r_args))
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano parametrów.");
			return;
		}

		// polecenie do IRC
		irc_send(ga, chan_parm, r_args);
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_time(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		irc_send(ga, chan_parm, "TIME");
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string r_args;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// temat można zmienić tylko w aktywnym oknie czata (nie w "Status" i "Debug)
		if(chan_parm[ga.current] && chan_parm[ga.current]->channel_ok)
		{
			// można nie podawać tematu, wtedy zostanie on wyczyszczony
			rest_args(kbd_buf, pos_arg_start, r_args);

			// wyślij temat do IRC
			irc_send(ga, chan_parm, "TOPIC " + buf_utf2iso(chan_parm[ga.current]->channel) + " :" + r_args);
		}

		else
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju czata.");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_vhost(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// jeśli nie podano parametrów, pokaż ostrzeżenie
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		if(pos_arg_start == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano parametrów (nicka i hasła do VHost).");
			return;
		}

		std::string vhost_tmp;

		// jeśli podano pierwszy argument (nick), przepisz go
		vhost_tmp = "VHOST " + f_arg + " ";

		// wykryj wpisanie hasła, jeśli nie wpisano go, pokaż ostrzeżenie
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		if(pos_arg_start == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano hasła do VHost.");
			return;
		}

		// jeśli podano drugi argument (hasło), dopisz je
		irc_send(ga, chan_parm, vhost_tmp + f_arg);

		ga.command_vhost = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_whois(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		// jeśli nie podano nicka, użyj własnego
		if(pos_arg_start == 0)
		{
			irc_send(ga, chan_parm, "WHOIS " + ga.zuousername + " " + ga.zuousername);	// 2x nick, aby pokazało idle
		}

		else
		{
			irc_send(ga, chan_parm, "WHOIS " + f_arg + " " + f_arg);	// 2x nick, aby pokazało idle
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_whowas(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start)
{
	std::string f_arg;

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		find_arg(kbd_buf, f_arg, pos_arg_start, false);

		// jeśli nie podano nicka, użyj własnego
		if(pos_arg_start == 0)
		{
			irc_send(ga, chan_parm, "WHOWAS " + ga.zuousername);
		}

		else
		{
			irc_send(ga, chan_parm, "WHOWAS " + f_arg);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}
