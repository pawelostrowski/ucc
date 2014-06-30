#include <string>		// std::string

#include "kbd_parser.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


int get_command(std::string &kbd_buf, std::string &g_command, std::string &g_command_org, size_t &pos_arg)
{
/*
	Pobierz wpisane polecenie do bufora g_command (przekonwertowane na wielkie litery) oraz do g_command_org (bez konwersji).
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

	// wstaw szukane polecenie
	g_command.clear();
	g_command.insert(0, kbd_buf, 1, pos_command_end - 1);	// - 1, bo pomijamy znak /

	// znalezione polecenie zapisz w drugim buforze, który użyty zostanie, jeśli wpiszemy nieistniejące polecenie (pokazane będzie dokładnie tak,
	// jak je wpisaliśmy, bez konwersji małych liter na wielkie)
	g_command_org = g_command;

	// zamień małe litery w poleceniu na wielkie (łatwiej będzie wyszukać polecenie)
	for(int i = 0; i < static_cast<int>(g_command.size()); ++i)
	{
		if(islower(g_command[i]))
		{
			g_command[i] = toupper(g_command[i]);
		}
	}

	// gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w get_arg() )
	pos_arg = pos_command_end;

	return 0;
}


std::string get_arg(std::string &kbd_buf, size_t &pos_arg, bool lower2upper)
{
/*
	Pobierz argument z bufora klawiatury od pozycji w pos_arg.
*/

	std::string g_arg;

	// jeśli pozycja w pos_arg jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa), więc zakończ
	if(pos_arg >= kbd_buf.size())
	{
		return g_arg;		// zwróć pusty bufor, oznaczający brak argumentu
	}

	// pomiń spacje pomiędzy poleceniem a argumentem lub pomiędzy kolejnymi argumentami (z uwzględnieniem rozmiaru bufora, aby nie czytać poza niego)
	while(pos_arg < kbd_buf.size() && kbd_buf[pos_arg] == ' ')
	{
		++pos_arg;		// kolejny znak w buforze
	}

	// jeśli po pominięciu spacji pozycja w pos_arg jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
	if(pos_arg == kbd_buf.size())
	{
		return g_arg;		// zwróć pusty bufor, oznaczający brak argumentu
	}

	// wykryj pozycję końca argumentu (gdzie jest spacja za poleceniem lub poprzednim argumentem)
	size_t pos_arg_end = kbd_buf.find(" ", pos_arg);

	// jeśli nie było spacji, koniec argumentu uznaje się za koniec bufora, czyli jego rozmiar
	if(pos_arg_end == std::string::npos)
	{
		pos_arg_end = kbd_buf.size();
	}

	// wstaw szukany argument
	g_arg.insert(0, kbd_buf, pos_arg, pos_arg_end - pos_arg);

	// jeśli trzeba, zamień małe litery w argumencie na wielkie
	if(lower2upper)
	{
		for(int i = 0; i < static_cast<int>(g_arg.size()); ++i)
		{
			if(islower(g_arg[i]))
			{
				g_arg[i] = toupper(g_arg[i]);
			}
		}
	}

	// wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
	pos_arg = pos_arg_end;

	// zwróć pobrany argument
	return g_arg;
}


std::string get_rest_args(std::string &kbd_buf, size_t pos_arg)
{
/*
	Pobierz resztę bufora od pozycji w pos_arg (czyli pozostałe argumenty lub argument).
*/

	std::string g_rest_args;

	// jeśli pozycja w pos_arg jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu lub argumentów
	// (tym bardziej, gdy jest większa), więc zakończ
	if(pos_arg >= kbd_buf.size())
	{
		return g_rest_args;	// zwróć pusty bufor, oznaczający brak argumentu lub argumentów
	}

	// znajdź miejsce, od którego zaczynają się znaki różne od spacji
	while(pos_arg < kbd_buf.size() && kbd_buf[pos_arg] == ' ')
	{
		++pos_arg;
	}

	// jeśli po pominięciu spacji pozycja w pos_arg jest równa wielkości bufora, oznacza to, że nie ma argumentu lub argumentów, więc zakończ
	if(pos_arg == kbd_buf.size())
	{
		return g_rest_args;	// zwróć pusty bufor, oznaczający brak argumentu lub argumentów
	}

	// wstaw pozostałą część bufora
	g_rest_args.insert(0, kbd_buf, pos_arg, kbd_buf.size() - pos_arg);

	// zwróć pobrany argument lub argumenty
	return g_rest_args;
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
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju czata.");
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

	size_t pos_arg = 1;		// pozycja początkowa kolejnego argumentu (od 1, bo pomijamy znak / )
	std::string g_command;		// znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
	std::string g_command_org;	// j/w, ale małe litery nie są zamieniane na wielkie

	// pobierz wpisane polecenie
	int g_command_status = get_command(kbd_buf, g_command, g_command_org, pos_arg);

	// wykryj błędnie wpisane polecenie
	if(g_command_status == 1)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Polecenie błędne, sam znak / nie jest poleceniem.");
		return;
	}

	else if(g_command_status == 2)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Polecenie błędne, po znaku / nie może być spacji.");
		return;
	}

/*
	Wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej.
*/
	if(g_command == "AWAY")
	{
		kbd_command_away(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "BUSY")
	{
		kbd_command_busy(ga, chan_parm);
	}

	else if(g_command == "CAPTCHA" || g_command == "CAP")
	{
		kbd_command_captcha(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "CARD")
	{
		kbd_command_card(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "CONNECT" || g_command == "C")
	{
		kbd_command_connect(ga, chan_parm);
	}

	else if(g_command == "DISCONNECT" || g_command == "D")
	{
		kbd_command_disconnect(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "HELP" || g_command == "H")
	{
		kbd_command_help(ga, chan_parm);
	}

	else if(g_command == "JOIN" || g_command == "J")
	{
		kbd_command_join(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "ME")
	{
		kbd_command_me(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "NAMES" || g_command == "N")
	{
		kbd_command_names(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "NICK")
	{
		kbd_command_nick(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "PART" || g_command == "P")
	{
		kbd_command_part(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "PRIV")
	{
		kbd_command_priv(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "QUIT" || g_command == "Q")
	{
		kbd_command_quit(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "RAW")
	{
		kbd_command_raw(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "TIME")
	{
		kbd_command_time(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "TOPIC")
	{
		kbd_command_topic(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "VHOST")
	{
		kbd_command_vhost(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "WHOIS")
	{
		kbd_command_whois(ga, chan_parm, kbd_buf, pos_arg);
	}

	else if(g_command == "WHOWAS")
	{
		kbd_command_whowas(ga, chan_parm, kbd_buf, pos_arg);
	}

	// gdy nie znaleziono polecenia, pokaż je wraz z ostrzeżeniem
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nieznane polecenie: /" + buf_iso2utf(g_command_org));
	}
}


void msg_connect_irc_err(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Najpierw zaloguj się.");
}


/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie (w kolejności alfabetycznej).
*/


void kbd_command_away(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		std::string away_reason = get_rest_args(kbd_buf, pos_arg);

		// jeśli podano powód nieobecności, wyślij go
		if(away_reason.size() > 0)
		{
			irc_send(ga, chan_parm, "AWAY :" + away_reason);
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
		// zmień stan flagi busy (faktyczna zmiana zostanie odnotowana po odebraniu potwierdzenia z serwera w parserze IRC)
		if(ga.busy_state)
		{
			irc_send(ga, chan_parm, "BUSY 0");
		}

		else
		{
			irc_send(ga, chan_parm, "BUSY 1");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_captcha(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Już zalogowano się.");
	}

	// gdy jest gotowość do przepisania captcha
	else if(ga.captcha_ready)
	{
		// pobierz wpisany kod captcha
		std::string captcha = get_arg(kbd_buf, pos_arg, false);

		if(captcha.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano kodu, spróbuj jeszcze raz.");
		}

		else if(captcha.size() != 6)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Kod musi mieć 6 znaków, spróbuj jeszcze raz.");
		}

		// jeśli captcha ma 6 znaków, wykonaj sprawdzenie
		else
		{
			std::string msg;

			// gdy kod wpisano i ma 6 znaków, wyślij go na serwer
			if(! http_auth_checkcode(ga, captcha, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}

			if(! http_auth_getuokey(ga, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}
		}
	}

	// gdy brak gotowości do przepisania captcha
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Najpierw wpisz " xCYAN "/connect " xRED "lub " xCYAN "/c");
	}
}


void kbd_command_card(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// polecenie do IRC (można nie podawać nicka, wtedy pokaże własną wizytówkę)
		irc_send(ga, chan_parm, "NS INFO " + get_arg(kbd_buf, pos_arg, false));

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
	// gdy już połączono z czatem
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Już zalogowano się.");
	}

	// jeśli nie podano nicka
	else if(ga.my_nick.size() == 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie wpisano nicka.");
	}

	// jeśli podano nick i można rozpocząć łączenie z czatem
	else
	{
		std::string msg;

		// komunikat o łączeniu we wszystkich otwartych pokojach z wyjątkiem "Debug"
		for(int i = 0; i < CHAN_MAX - 1; ++i)
		{
			if(chan_parm[i])
			{
				win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, xBOLD_ON xGREEN "# Łączę z czatem...");
			}
		}

		// skasuj użycie wybranych flag poleceń w celu odpowiedniego sterowania wyświetlaniem komunikatów podczas logowania na czat
		ga.command_card = false;
		ga.command_join = false;
		ga.command_names = false;
		ga.command_vhost = false;

		// gdy wpisano nick, rozpocznij łączenie
		if(! http_auth_init(ga, msg))
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
			return;
		}

		// gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
		if(ga.my_password.size() == 0)
		{
			// pobierz captcha
			if(! http_auth_getcaptcha(ga, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xGREEN "# Przepisz kod z obrazka, w tym celu wpisz " xCYAN "/captcha kod_z_obrazka " xGREEN "lub "
					xCYAN "/cap kod_z_obrazka");
		}

		// gdy wpisano hasło, wykonaj część dla nicka stałego
		else
		{
			if(! http_auth_getsk(ga, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}

			if(! http_auth_mlogin(ga, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}

			if(! http_auth_getuokey(ga, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}

/*
			// dodać override jako polecenie, gdy wykryty zostanie zalogowany nick
			if(! http_auth_useroverride(ga, msg))
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, msg);
				return;
			}
*/
		}
	}
}


void kbd_command_disconnect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli było połączenie z czatem, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		std::string disconnect_reason = get_rest_args(kbd_buf, pos_arg);

		// jeśli podano argument (tekst pożegnalny), wstaw go
		if(disconnect_reason.size() > 0)
		{
			irc_send(ga, chan_parm, "QUIT :" + disconnect_reason);
		}

		// jeśli nie podano argumentu, wyślij samo polecenie
		else
		{
			irc_send(ga, chan_parm, "QUIT");
		}
	}

	// jeśli nie było połączenia z czatem, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie zalogowano się.");
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


void kbd_command_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string join_chan = get_arg(kbd_buf, pos_arg, false);

		if(join_chan.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano pokoju.");
		}

		else
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #, ale jeśli jest ^, to nie dodawaj (aby można było wejść na priv)
			if(join_chan[0] != '#' && join_chan[0] != '^')
			{
				join_chan.insert(0, "#");
			}

			// opcjonalnie można podać klucz do pokoju
			irc_send(ga, chan_parm, "JOIN " + join_chan + " " + get_arg(kbd_buf, pos_arg, false));

			ga.command_join = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_me(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// jeśli nie przebywamy w aktywnym pokoju czata, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		if(! chan_parm[ga.current]->channel_ok)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju czata.");
		}

		else
		{
			std::string me_reason = get_rest_args(kbd_buf, pos_arg);	// pobierz wpisany komunikat dla /me (nie jest niezbędny)

			// jeśli przebywamy w aktywnym pokoju czata, przygotuj komunikat do wyświetlenia w oknie terminala
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xBOLD_ON xMAGENTA "* " + buf_iso2utf(ga.zuousername) + xNORMAL " " + buf_iso2utf(me_reason));

			// wyślij też komunikat do serwera IRC
			irc_send(ga, chan_parm, "PRIVMSG " + buf_utf2iso(chan_parm[ga.current]->channel) + " :\x01" "ACTION " + me_reason + "\x01");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_names(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// /names może działać z parametrem lub bez (wtedy dotyczy aktywnego pokoju)
		std::string names_chan = get_arg(kbd_buf, pos_arg, false);

		// gdy podano pokój do sprawdzenia
		if(names_chan.size() > 0)
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #
			if(names_chan[0] != '#')
			{
				names_chan.insert(0, "#");
			}

			// wyślij komunikat do serwera IRC
			irc_send(ga, chan_parm, "NAMES " + names_chan);
		}

		// gdy nie podano pokoju sprawdź, czy pokój jest aktywny (bo /names dla "Status" lub "Debug" nie ma sensu)
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


void kbd_command_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// po połączeniu z IRC nie można zmienić nicka
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Po zalogowaniu się nie można zmienić nicka.");
	}

	// nick można zmienić tylko, gdy nie jest się połączonym z IRC
	else
	{
		std::string nick = get_arg(kbd_buf, pos_arg, false);

		if(nick.size() == 0)
		{
			// gdy nie wpisano nicka oraz wcześniej też żadnego nie podano
			if(ga.my_nick.size() == 0)
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano nicka.");
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
			}
		}

		else if(nick.size() < 3)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nick jest za krótki (minimalnie 3 znaki).");
		}

		else if(nick.size() > 32)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nick jest za długi (maksymalnie 32 znaki).");
		}

		// gdy wpisano nick mieszczący się w zakresie 3...32 znaków
		else
		{
			// przepisz nick
			ga.my_nick = nick;

			// pobierz hasło (jeśli wpisano, jeśli nie, bufor hasła będzie pusty)
			ga.my_password = get_arg(kbd_buf, pos_arg, false);

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
}


void kbd_command_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// wyślij polecenie, gdy to aktywny pokój czata, a nie "Status" lub "Debug"
		if(chan_parm[ga.current] && chan_parm[ga.current]->channel_ok)
		{
			std::string part_reason = get_rest_args(kbd_buf, pos_arg);

			// jeśli podano powód wyjścia, wyślij go
			if(part_reason.size() > 0)
			{
				irc_send(ga, chan_parm, "PART " + buf_utf2iso(chan_parm[ga.current]->channel) + " :" + part_reason);
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


void kbd_command_priv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// pobierz nick, który zapraszamy do rozmowy prywatnej
		std::string priv_nick = get_arg(kbd_buf, pos_arg, false);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(priv_nick.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano nicka zapraszanego do rozmowy prywatnej.");
		}

		else
		{
			// wyślij polecenie do IRC
			irc_send(ga, chan_parm, "PRIV " + priv_nick);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		std::string quit_reason = get_rest_args(kbd_buf, pos_arg);

		// jeśli podano powód wyjścia, wyślij go
		if(quit_reason.size() > 0)
		{
			irc_send(ga, chan_parm, "QUIT :" + quit_reason);
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


void kbd_command_raw(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		std::string raw_args = get_rest_args(kbd_buf, pos_arg);

		// jeśli nie podano parametrów, pokaż ostrzeżenie
		if(raw_args.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano parametrów.");
		}

		else
		{
			// polecenie do IRC
			irc_send(ga, chan_parm, raw_args);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_time(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{	// opcjonalnie można podać serwer
		irc_send(ga, chan_parm, "TIME " + get_arg(kbd_buf, pos_arg, false));
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		// temat można zmienić tylko w aktywnym oknie czata (nie w "Status" i "Debug)
		if(chan_parm[ga.current] && chan_parm[ga.current]->channel_ok)
		{
			// wyślij temat do IRC (można nie podawać tematu, wtedy zostanie on wyczyszczony)
			irc_send(ga, chan_parm, "TOPIC " + buf_utf2iso(chan_parm[ga.current]->channel) + " :" + get_rest_args(kbd_buf, pos_arg));
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


void kbd_command_vhost(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
	if(ga.irc_ok)
	{
		std::string vhost_nick = get_arg(kbd_buf, pos_arg, false);

		// jeśli nie podano parametrów, pokaż ostrzeżenie
		if(vhost_nick.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano parametrów (nicka i hasła dla VHost).");
		}

		// jeśli podano nick, sprawdź, czy podano hasło
		else
		{
			std::string vhost_password = get_arg(kbd_buf, pos_arg, false);

			// jeśli nie podano hasła, pokaż ostrzeżenie
			if(vhost_password.size() == 0)
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano hasła dla VHost.");
			}

			// jeśli podano hasło (i wcześniej nick), wyślij je na serwer
			else
			{
				// wyślij nick i hasło dla VHost
				irc_send(ga, chan_parm, "VHOST " + vhost_nick + " " + vhost_password);

				ga.command_vhost = true;
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_whois(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string whois_nick = get_arg(kbd_buf, pos_arg, false);

		// jeśli podano nick, to go użyj
		if(whois_nick.size() > 0)
		{
			irc_send(ga, chan_parm, "WHOIS " + whois_nick + " " + whois_nick);	// 2x nick, aby zawsze pokazało idle
		}

		// jeśli nie podano nicka, użyj własnego
		else
		{
			irc_send(ga, chan_parm, "WHOIS " + ga.zuousername + " " + ga.zuousername);	// 2x nick, aby zawsze pokazało idle
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}


void kbd_command_whowas(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string whowas_nick = get_arg(kbd_buf, pos_arg, false);

		// jeśli podano nick, to go użyj
		if(whowas_nick.size() > 0)
		{
			irc_send(ga, chan_parm, "WHOWAS " + whowas_nick);
		}

		// jeśli nie podano nicka, użyj własnego
		else
		{
			irc_send(ga, chan_parm, "WHOWAS " + ga.zuousername);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_connect_irc_err(ga, chan_parm);
	}
}
