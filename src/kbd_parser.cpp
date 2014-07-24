#include <string>		// std::string

#include "kbd_parser.hpp"
#include "chat_utils.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


std::string get_arg(std::string &kbd_buf, size_t &pos_arg_start, bool lower2upper)
{
/*
	Pobierz argument z bufora klawiatury od pozycji w pos_arg_start.
*/

	std::string arg;

	// jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa),
	// więc zakończ
	if(pos_arg_start >= kbd_buf.size())
	{
		return arg;		// zwróć pusty bufor, oznaczający brak argumentu
	}

	// pomiń spacje pomiędzy poleceniem a argumentem lub pomiędzy kolejnymi argumentami (z uwzględnieniem rozmiaru bufora, aby nie czytać poza niego)
	while(pos_arg_start < kbd_buf.size() && kbd_buf[pos_arg_start] == ' ')
	{
		++pos_arg_start;	// kolejny znak w buforze
	}

	// jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
	if(pos_arg_start == kbd_buf.size())
	{
		return arg;		// zwróć pusty bufor, oznaczający brak argumentu
	}

	// wykryj pozycję końca argumentu (gdzie jest spacja za poleceniem lub poprzednim argumentem)
	size_t pos_arg_end = kbd_buf.find(" ", pos_arg_start);

	// jeśli nie było spacji, koniec argumentu uznaje się za koniec bufora, czyli jego rozmiar
	if(pos_arg_end == std::string::npos)
	{
		pos_arg_end = kbd_buf.size();
	}

	// wstaw szukany argument
	arg.insert(0, kbd_buf, pos_arg_start, pos_arg_end - pos_arg_start);

	// jeśli trzeba, zamień małe litery w argumencie na wielkie (domyślnie nie zamieniaj)
	if(lower2upper)
	{
		arg = buf_lower2upper(arg);
	}

	// wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
	pos_arg_start = pos_arg_end;

	// zwróć pobrany argument
	return arg;
}


std::string get_rest_args(std::string &kbd_buf, size_t pos_arg_start)
{
/*
	Pobierz resztę bufora od pozycji w pos_arg_start (czyli pozostałe argumenty lub argument).
*/

	std::string rest_args;

	// jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu lub argumentów
	// (tym bardziej, gdy jest większa), więc zakończ
	if(pos_arg_start >= kbd_buf.size())
	{
		return rest_args;	// zwróć pusty bufor, oznaczający brak argumentu lub argumentów
	}

	// znajdź miejsce, od którego zaczynają się znaki różne od spacji
	while(pos_arg_start < kbd_buf.size() && kbd_buf[pos_arg_start] == ' ')
	{
		++pos_arg_start;	// kolejny znak w buforze
	}

	// jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma argumentu lub argumentów, więc zakończ
	if(pos_arg_start == kbd_buf.size())
	{
		return rest_args;	// zwróć pusty bufor, oznaczający brak argumentu lub argumentów
	}

	// wstaw pozostałą część bufora
	rest_args.insert(0, kbd_buf, pos_arg_start, kbd_buf.size() - pos_arg_start);

	// zwróć pobrany argument lub argumenty
	return rest_args;
}


void msg_err_first_login(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Najpierw zaloguj się.");
}


void msg_err_not_active_chan(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie jesteś w aktywnym pokoju czata.");
}


void msg_err_disconnect(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// informację pokaż we wszystkich pokojach (z wyjątkiem "Debug" i "RawUnknown")
	win_buf_all_chan_msg(ga, chan_parm, xBOLD_ON xRED "# Rozłączono.");
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

/*
	Jeśli nie wpisano / na początku, wpisano // na początku (ma znaczenie w przypadku emotikon) lub wpisano inne znaki, traktuj to jako normalne
	pisanie w pokoju czata.
*/
	else if(kbd_buf[0] != '/' || (kbd_buf.size() > 1 && kbd_buf[1] == '/'))
	{
		// jeśli brak połączenia z IRC, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		if(! ga.irc_ok)
		{
			msg_err_first_login(ga, chan_parm);
		}

		// gdy połączono z IRC oraz jesteśmy w aktywnym pokoju czata, przygotuj komunikat do wyświetlenia w terminalu (jeśli wpisano coś w
		// formatowaniu zwracanym przez serwer, funkcja form_from_chat() przekształci to tak, aby w terminalu wyświetlić to, jakby było to
		// odebrane z serwera)
		else if(ga.current < CHAN_CHAT)
		{
			std::string term_buf_tmp = buf_iso2utf(kbd_buf);

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xBOLD_ON "<" + buf_iso2utf(ga.zuousername) + ">" xNORMAL " " + form_from_chat(term_buf_tmp));

			// wyślij też komunikat do serwera IRC
			irc_send(ga, chan_parm, "PRIVMSG " + buf_utf2iso(chan_parm[ga.current]->channel) + " :" + form_to_chat(kbd_buf));
		}

		// jeśli nie jesteśmy w aktywnym pokoju czata, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, chan_parm);
		}
	}

/*
	Dojście tu oznacza, że wpisano / na początku. Pobierz w takim razie wpisane polecenie za / (zwracając uwagę, czy to nie jedyny znak lub czy nie ma
	za nim spacji, bo to oznacza błędne polecenie).
*/
	else
	{
		// sprawdź, czy za poleceniem są jakieś znaki (sam znak / nie jest poleceniem)
		if(kbd_buf.size() <= 1)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Polecenie błędne, sam znak / nie jest poleceniem.");

			return;
		}

		// sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
		if(kbd_buf[1] == ' ')
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Polecenie błędne, po znaku / nie może być spacji.");

			return;
		}

		std::string command;		// znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
		std::string command_org;	// j/w, ale małe litery nie są zamieniane na wielkie
		size_t pos_command_end;		// pozycja, gdzie się kończy polecenie
		size_t pos_arg_start;		// pozycja początkowa kolejnego argumentu (w tym miejscu, o ile wpisano polecenie, będzie to jego koniec)

		// wykryj pozycję końca polecenia (gdzie jest spacja za poleceniem)
		pos_command_end = kbd_buf.find(" ");

		// jeśli nie było spacji, koniec polecenia uznaje się za koniec bufora, czyli jego rozmiar
		if(pos_command_end == std::string::npos)
		{
			pos_command_end = kbd_buf.size();
		}

		// wstaw szukane polecenie
		command.insert(0, kbd_buf, 1, pos_command_end - 1);	// 1, - 1, bo pomijamy znak /

		// znalezione polecenie zapisz w drugim buforze, który użyty zostanie, jeśli wpiszemy nieistniejące polecenie (pokazane będzie dokładnie tak,
		// jak je wpisaliśmy, bez konwersji małych liter na wielkie)
		command_org = command;

		// zamień małe litery w poleceniu na wielkie (łatwiej będzie wyszukać polecenie)
		command = buf_lower2upper(command);

		// gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w get_arg() )
		pos_arg_start = pos_command_end;

/*
	Wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej.
*/
		if(command == "AWAY")
		{
			command_away(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "BUSY")
		{
			command_busy(ga, chan_parm);
		}

		else if(command == "CAPTCHA" || command == "CAP")
		{
			command_captcha(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "CARD")
		{
			command_card(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "CONNECT" || command == "C")
		{
			command_connect(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "DISCONNECT" || command == "D")
		{
			command_disconnect(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "HELP" || command == "H")
		{
			command_help(ga, chan_parm);
		}

		else if(command == "JOIN" || command == "J")
		{
			command_join(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "KICK")
		{
			command_kick(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "LIST" || command == "L")
		{
			command_list(ga, chan_parm);
		}

		else if(command == "ME")
		{
			command_me(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "NAMES" || command == "N")
		{
			command_names(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "NICK")
		{
			command_nick(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "PART" || command == "P")
		{
			command_part(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "PRIV")
		{
			command_priv(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "QUIT" || command == "Q")
		{
			command_quit(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "RAW")
		{
			command_raw(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "TIME")
		{
			command_time(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "TOPIC")
		{
			command_topic(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "VHOST")
		{
			command_vhost(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "WHOIS")
		{
			command_whois(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		else if(command == "WHOWAS")
		{
			command_whowas(ga, chan_parm, kbd_buf, pos_arg_start);
		}

		// gdy nie znaleziono polecenia, pokaż je wraz z ostrzeżeniem
		else
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nieznane polecenie: /" + buf_iso2utf(command_org));
		}
	}
}


/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie (w kolejności alfabetycznej).
*/


void command_away(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string away = get_rest_args(kbd_buf, pos_arg_start);

		// jeśli podano powód nieobecności, wyślij go (co ustawi tryb nieobecności)
		if(away.size() > 0)
		{
			irc_send(ga, chan_parm, "AWAY :" + away);

			ga.msg_away = buf_iso2utf(away);
		}

		// przy braku powodu zostanie ustawiony tryb obecności
		else
		{
			irc_send(ga, chan_parm, "AWAY");
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_busy(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// zmień stan flagi busy (faktyczna zmiana zostanie odnotowana po odebraniu potwierdzenia z serwera w parserze IRC)
		if(ga.my_busy)
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
		msg_err_first_login(ga, chan_parm);
	}
}


void command_captcha(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Już zalogowano się.");
	}

	// gdy jest gotowość do przepisania captcha
	else if(ga.captcha_ready)
	{
		// pobierz wpisany kod captcha
		std::string captcha = get_arg(kbd_buf, pos_arg_start);

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
			// gdy kod wpisano i ma 6 znaków, wyślij go na serwer
			if(! auth_http_checkcode(ga, chan_parm, captcha))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, chan_parm);

				return;
			}

			if(! auth_http_getuokey(ga, chan_parm))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, chan_parm);

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


void command_card(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string card_nick = get_arg(kbd_buf, pos_arg_start);

		// można nie podawać nicka, wtedy pokaż własną wizytówkę
		if(card_nick.size() == 0)
		{
			card_nick = ga.zuousername;
		}

		// polecenie do IRC
		irc_send(ga, chan_parm, "NS INFO " + card_nick);

		ga.command_card = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}

}


void command_connect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
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

	// podano nick i można rozpocząć łączenie z czatem
	else
	{
		// komunikat o łączeniu we wszystkich otwartych pokojach (z wyjątkiem "Debug" i "RawUnknown")
		win_buf_all_chan_msg(ga, chan_parm, xBOLD_ON xGREEN "# Łączenie z czatem...");

		// skasuj użycie wybranych flag poleceń w celu odpowiedniego sterowania wyświetlaniem komunikatów podczas logowania na czat
		ga.command_card = false;
		ga.command_join = false;
		ga.command_names = false;
		ga.command_names_empty = false;
		ga.command_vhost = false;
		ga.command_whois_short = false;

		// rozpocznij łączenie z czatem
		if(! auth_http_init(ga, chan_parm))
		{
			// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
			msg_err_disconnect(ga, chan_parm);

			return;
		}

		// gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
		if(ga.my_password.size() == 0)
		{
			// pobierz captcha
			if(! auth_http_getcaptcha(ga, chan_parm))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, chan_parm);

				return;
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xGREEN "# Przepisz kod z obrazka, w tym celu wpisz " xCYAN "/captcha kod_z_obrazka " xGREEN "lub "
					xCYAN "/cap kod_z_obrazka");
		}

		// gdy wpisano hasło, wykonaj część dla nicka stałego
		else
		{
			if(! auth_http_getsk(ga, chan_parm))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, chan_parm);

				return;
			}

			if(! auth_http_mlogin(ga, chan_parm))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, chan_parm);

				return;
			}

			if(! auth_http_getuokey(ga, chan_parm))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, chan_parm);

				return;
			}

			// wpisanie 'o' w parametrze oznacza próbę zalogowania na zalogowanym już nicku, przydaje się po zmianie IP, aby nie czekać na timeout
			if(get_arg(kbd_buf, pos_arg_start, true) == "O" && ! auth_http_useroverride(ga, chan_parm))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu
				msg_err_disconnect(ga, chan_parm);

				// bez return, bo to koniec funkcji
			}
		}
	}
}


void command_disconnect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string disconnect_reason = get_rest_args(kbd_buf, pos_arg_start);

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

	// jeśli nie połączono z IRC, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie zalogowano się.");
	}
}


void command_help(struct global_args &ga, struct channel_irc *chan_parm[])
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
			xCYAN  "/kick\n"
			xCYAN  "/list " xGREEN "lub " xCYAN "/l\n"
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


void command_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string join_chan = get_arg(kbd_buf, pos_arg_start);

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
			std::string join_key = get_arg(kbd_buf, pos_arg_start);

			if(join_key.size() == 0)
			{
				irc_send(ga, chan_parm, "JOIN " + join_chan);
			}

			else
			{
				irc_send(ga, chan_parm, "JOIN " + join_chan + " " + join_key);
			}

			ga.command_join = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// w aktywnym pokoju czata można wyrzucić użytkownika
		if(ga.current < CHAN_CHAT)
		{
			// pobierz wpisany nick do wyrzucenia
			std::string kick_nick = get_arg(kbd_buf, pos_arg_start);

			// jeśli nie wpisano nicka, pokaż ostrzeżenie
			if(kick_nick.size() == 0)
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano nicka do wyrzucenia.");
			}

			// jeśli wpisano nick do wyrzucenia, wyślij polecenie do IRC (w aktualnie otwartym pokoju)
			else
			{
				// opcjonalnie można podać powód wyrzucenia
				irc_send(ga, chan_parm, "KICK " + buf_utf2iso(chan_parm[ga.current]->channel) + " " + kick_nick
					+ " :" + get_rest_args(kbd_buf, pos_arg_start));
			}
		}

		// jeśli nie przebywamy w aktywnym pokoju czata, nie można wyrzucić użytkownika, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, chan_parm);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_list(struct global_args &ga, struct channel_irc *chan_parm[])
{
	std::string chan_nr;

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xGREEN "# Aktualnie otwarte pokoje:");

	for(int i = 0; i < CHAN_MAX; ++i)
	{
		if(chan_parm[i])
		{
			if(i < CHAN_CHAT)
			{
				chan_nr = std::to_string(i + 1);
			}

			else if(i == CHAN_STATUS)
			{
				chan_nr = "s";
			}

			else if(i == CHAN_DEBUG_IRC)
			{
				chan_nr = "d";
			}

			else
			{
				chan_nr = "x";
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xBOLD_ON + chan_nr + xNORMAL "\t\t" + chan_parm[i]->channel);
		}
	}
}


void command_me(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// w aktywnym pokoju czata można wysłać wiadomość
		if(ga.current < CHAN_CHAT)
		{
			// pobierz wpisany komunikat dla /me (nie jest niezbędny)
			std::string me_reason = get_rest_args(kbd_buf, pos_arg_start);

			// przygotuj komunikat do wyświetlenia w oknie terminala
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xBOLD_ON xMAGENTA "* " + buf_iso2utf(ga.zuousername) + xNORMAL " " + buf_iso2utf(me_reason));

			// wyślij też komunikat do serwera IRC
			irc_send(ga, chan_parm, "PRIVMSG " + buf_utf2iso(chan_parm[ga.current]->channel) + " :\x01" "ACTION " + me_reason + "\x01");
		}

		// jeśli nie przebywamy w aktywnym pokoju czata, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, chan_parm);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_names(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// /names może działać z parametrem lub bez (wtedy dotyczy aktywnego pokoju)
		std::string names_chan = get_arg(kbd_buf, pos_arg_start);

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

			ga.command_names = true;
		}

		// gdy nie podano pokoju sprawdź, czy pokój jest aktywny (bo /names dla "Status", "Debug" lub "RawUnknown" nie ma sensu)
		else
		{
			if(ga.current < CHAN_CHAT)
			{
				irc_send(ga, chan_parm, "NAMES " + buf_utf2iso(chan_parm[ga.current]->channel));

				ga.command_names = true;
				ga.command_names_empty = true;
			}

			else
			{
				msg_err_not_active_chan(ga, chan_parm);
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// po połączeniu z IRC nie można zmienić nicka
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Po zalogowaniu się nie można zmienić nicka.");
	}

	// nick można zmienić tylko, gdy nie jest się połączonym z IRC
	else
	{
		std::string nick = get_arg(kbd_buf, pos_arg_start);

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
			ga.my_password = get_arg(kbd_buf, pos_arg_start);

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


void command_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// w oknie "Status" i "Debug" pokaż ostrzeżenie, że tych okien nie można zamknąć
	if(ga.current == CHAN_STATUS || ga.current == CHAN_DEBUG_IRC)
	{
		msg_err_not_active_chan(ga, chan_parm);
	}

	// jako wyjątek /part w oknie "RawUnknown" zamyka je
	else if(ga.current == CHAN_RAW_UNKNOWN)
	{
		// tymczasowo przełącz na "Status", potem przerobić, aby przechodziło do poprzedniego, który był otwarty
		ga.current = CHAN_STATUS;
		win_buf_refresh(ga, chan_parm);

		// usuń kanał "RawUnknown"
		delete chan_parm[CHAN_RAW_UNKNOWN];

		// wyzeruj go w tablicy, w ten sposób wiadomo, że już nie istnieje
		chan_parm[CHAN_RAW_UNKNOWN] = 0;
	}

	// ta opcja dotyczy /part po zalogowaniu się na czat i dotyczy wyłącznie pokoi czata, ale nie trzeba już ich sprawdzać w warunku, bo wyżej zostało
	// wykluczone, że /part zadziała w niewłaściwym pokoju/oknie
	else if(ga.irc_ok)
	{
		std::string part_reason = get_rest_args(kbd_buf, pos_arg_start);

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

	// gdy jesteśmy rozłączeni, daj możliwość zamknięcia pokoi czata (wtedy nie dostaniemy odpowiedzi PART z serwera), nie trzeba sprawdzać warunku,
	// bo wyżej zostało wykluczone, że /part zadziała w niewłaściwym pokoju/oknie
	else
	{
		del_chan_chat(ga, chan_parm, chan_parm[ga.current]->channel);
	}
}


void command_priv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz nick, który zapraszamy do rozmowy prywatnej
		std::string priv_nick = get_arg(kbd_buf, pos_arg_start);

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
		msg_err_first_login(ga, chan_parm);
	}
}


void command_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string quit_reason = get_rest_args(kbd_buf, pos_arg_start);

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


void command_raw(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string raw_args = get_rest_args(kbd_buf, pos_arg_start);

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
		msg_err_first_login(ga, chan_parm);
	}
}


void command_time(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// opcjonalnie można podać serwer
		irc_send(ga, chan_parm, "TIME " + get_arg(kbd_buf, pos_arg_start));
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// temat można zmienić tylko w aktywnym oknie czata (nie w "Status" i "Debug)
		if(ga.current < CHAN_CHAT)
		{
			// wyślij temat do IRC (można nie podawać tematu, wtedy zostanie on wyczyszczony)
			irc_send(ga, chan_parm, "CS SET " + buf_utf2iso(chan_parm[ga.current]->channel) + " TOPIC " + get_rest_args(kbd_buf, pos_arg_start));
		}

		else
		{
			msg_err_not_active_chan(ga, chan_parm);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_vhost(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string vhost_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie podano parametrów, pokaż ostrzeżenie
		if(vhost_nick.size() == 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie podano nicka i hasła dla VHost.");
		}

		// jeśli podano nick, sprawdź, czy podano hasło
		else
		{
			std::string vhost_password = get_arg(kbd_buf, pos_arg_start);

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
		msg_err_first_login(ga, chan_parm);
	}
}


void command_whois(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string whois_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie podano nicka, użyj własnego
		if(whois_nick.size() == 0)
		{
			whois_nick = ga.zuousername;
		}

		// polecenie do IRC
		irc_send(ga, chan_parm, "WHOIS " + whois_nick + " " + whois_nick);	// 2x nick, aby zawsze pokazało idle

		// wpisanie 's' za nickiem wyświetli krótką formę (tylko nick, ZUO i host)
		if(get_arg(kbd_buf, pos_arg_start, true) == "S")
		{
                        ga.command_whois_short = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}


void command_whowas(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string whowas_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie podano nicka, użyj własnego
		if(whowas_nick.size() == 0)
		{
			whowas_nick = ga.zuousername;
		}

		// polecenie do IRC
		irc_send(ga, chan_parm, "WHOWAS " + whowas_nick);
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, chan_parm);
	}
}
