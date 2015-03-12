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


#include <string>		// std::string

#include "kbd_parser.hpp"
#include "chat_utils.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


void msg_err_first_login(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Najpierw zaloguj się.", false);
}


void msg_err_already_logged_in(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Już zalogowano się.", false);
}


void msg_err_not_specified_nick(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka.", false);
}


void msg_err_not_active_chan(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie jesteś w aktywnym pokoju czata.", false);
}


void msg_err_disconnect(struct global_args &ga, struct channel_irc *ci[])
{
	// informację pokaż we wszystkich pokojach (z wyjątkiem "DebugIRC" i "RawUnknown")
	win_buf_all_chan_msg(ga, ci, uINFOb xRED "Rozłączono.");
}


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

	// pomiń spacje pomiędzy poleceniem a argumentem lub pomiędzy kolejnymi argumentami (z uwzględnieniem rozmiaru bufora, aby nie czytać poza nim)
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


void kbd_parser(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf)
{
/*
	Prosty interpreter wpisywanych poleceń (zaczynających się od / na pierwszej pozycji).
*/

	// skasuj ewentualne użycie wybranych poprzednio poleceń
	ga.cf = {};

	// zapobiega wykonywaniu się reszty kodu, gdy w buforze nic nie ma
	if(kbd_buf.size() == 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Błąd bufora klawiatury (bufor jest pusty)!");
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
			msg_err_first_login(ga, ci);
		}

		// gdy połączono z IRC oraz jesteśmy w aktywnym pokoju czata, przygotuj komunikat do wyświetlenia w terminalu (jeśli wpisano coś w
		// formatowaniu zwracanym przez serwer, funkcja form_from_chat() przekształci to tak, aby w terminalu wyświetlić to, jakby było to
		// odebrane z serwera)
		else if(ga.current < CHAN_CHAT)
		{
			std::string nick_stat;

			// jeśli pokazywanie statusu nicka jest włączone, dodaj je do nicka (busy również ma wpływ na nick)
			if(ga.show_stat_in_win_chat)
			{
				nick_stat = get_flags_nick(ga, ci, ga.current, buf_lower2upper(ga.zuousername));
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					xBOLD_ON "<" + nick_stat + xBOLD_ON + ga.zuousername + xTERMC ">" xNORMAL " " + form_from_chat(kbd_buf), true, 2);

			// wyślij też komunikat do serwera IRC
			irc_send(ga, ci, "PRIVMSG " + ci[ga.current]->channel + " :" + form_to_chat(kbd_buf));
		}

		// jeśli nie jesteśmy w aktywnym pokoju czata, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, ci);
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
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Polecenie błędne, sam znak / nie jest poleceniem.", false);
		}

		// sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
		else if(kbd_buf[1] == ' ')
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Polecenie błędne, po znaku / nie może być spacji.", false);
		}

		// pobierz wpisane polecenie i je zinterpretuj
		else
		{
			// wykryj pozycję końca polecenia (gdzie jest spacja za poleceniem)
			size_t pos_command_end = kbd_buf.find(" ");

			// jeśli nie było spacji, koniec polecenia uznaje się za koniec bufora, czyli jego rozmiar
			if(pos_command_end == std::string::npos)
			{
				pos_command_end = kbd_buf.size();
			}

			// wstaw szukane polecenie
			std::string command_org;
			command_org.insert(0, kbd_buf, 1, pos_command_end - 1);		// 1, - 1, bo pomijamy znak /

			// zamień małe litery w poleceniu na wielkie (łatwiej będzie wyszukać polecenie)
			std::string command = buf_lower2upper(command_org);

			// gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w get_arg() )
			size_t pos_arg_start = pos_command_end;

/*
	Wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej.
*/
			if(command == "AWAY")
			{
				command_away(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "BAN")
			{
				command_ban_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "BANIP")
			{
				command_ban_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "BUSY")
			{
				command_busy(ga, ci);
			}

			else if(command == "CAPTCHA" || command == "CAP")
			{
				command_captcha(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "CARD")
			{
				command_card(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "CONNECT" || command == "C")
			{
				command_connect(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "DBAN")
			{
				command_ban_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "DBANIP")
			{
				command_ban_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "DFAVOURITES" || command == "DFAV")
			{
				command_favourites_common(ga, ci, kbd_buf, pos_arg_start, false);
			}

			else if(command == "DFRIENDS" || command == "DF")
			{
				command_friends_common(ga, ci, kbd_buf, pos_arg_start, false);
			}

			else if(command == "DIGNORE" || command == "DIGN")
			{
				command_ignore_common(ga, ci, kbd_buf, pos_arg_start, false);
			}

			else if(command == "DISCONNECT" || command == "D")
			{
				command_disconnect(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "DOP")
			{
				command_op_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "DSOP")
			{
				command_op_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "DVIP")
			{
				command_vip_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "FAVOURITES" || command == "FAV")
			{
				command_favourites_common(ga, ci, kbd_buf, pos_arg_start, true);
			}

			else if(command == "FRIENDS" || command == "F")
			{
				command_friends_common(ga, ci, kbd_buf, pos_arg_start, true);
			}

			else if(command == "HELP" || command == "H")
			{
				command_help(ga, ci);
			}

			else if(command == "HOMES")
			{
				command_homes(ga, ci);
			}

			else if(command == "IGNORE" || command == "IGN")
			{
				command_ignore_common(ga, ci, kbd_buf, pos_arg_start, true);
			}

			else if(command == "INFO")
			{
				command_info(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "INVITE" || command == "INV")
			{
				command_invite(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "JOIN" || command == "J")
			{
				command_join(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "KBAN")
			{
				command_kban_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

/*
			else if(command == "KBANIP")
			{
				command_kban_common(ga, ci, kbd_buf, pos_arg_start, command);
			}
*/

			else if(command == "KICK")
			{
				command_kick(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "LIST" || command == "L")
			{
				command_list(ga, ci);
			}

			else if(command == "LUSERS" || command == "LU")
			{
				command_lusers(ga, ci);
			}

			else if(command == "ME")
			{
				command_me(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "NAMES" || command == "N")
			{
				command_names(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "NICK")
			{
				command_nick(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "OP")
			{
				command_op_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "PART" || command == "P")
			{
				command_part(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "PRIV")
			{
				command_priv(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "QUIT" || command == "Q")
			{
				command_quit(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "RAW")
			{
				command_raw(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "SET")
			{
				command_set(ga, kbd_buf, pos_arg_start);
			}

			else if(command == "SOP")
			{
				command_op_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "TIME")
			{
				command_time(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "TOPIC")
			{
				command_topic(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "VHOST")
			{
				command_vhost(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "VIP")
			{
				command_vip_common(ga, ci, kbd_buf, pos_arg_start, command);
			}

			else if(command == "WHOIS")
			{
				command_whois(ga, ci, kbd_buf, pos_arg_start);
			}

			else if(command == "WHOWAS")
			{
				command_whowas(ga, ci, kbd_buf, pos_arg_start);
			}

			// gdy nie znaleziono polecenia, pokaż je wraz z ostrzeżeniem
			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nieznane polecenie: /" + command_org, false);
			}
		}
	}
}


/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie (w kolejności alfabetycznej).
*/


void command_away(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string away = get_rest_args(kbd_buf, pos_arg_start);

		// jeśli podano powód nieobecności, wyślij go (co ustawi tryb nieobecności), przy braku powodu zostanie ustawiony tryb obecności
		irc_send(ga, ci, (away.size() > 0 ? "AWAY :" + away : "AWAY"));

		ga.away_msg = form_from_chat(away);
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_ban_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &ban_type)
{
	// ban_type może przyjąć następujące wartości: BAN, BANIP, DBAN, DBANIP
	if(ban_type != "BAN" && ban_type != "BANIP" && ban_type != "DBAN" && ban_type != "DBANIP")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nieprawidłowa opcja dla command_ban_common()");
	}

	// jeśli połączono z IRC
	else if(ga.irc_ok)
	{
		// pobierz wpisany nick do zbanowania
		std::string ban_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(ban_nick.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, (ban_type == "BAN" || ban_type == "BANIP"
					? uINFOn xRED "Nie podano nicka do zbanowania."
					: uINFOn xRED "Nie podano nicka do odbanowania."), false);
		}

		else
		{
			// pobierz opcjonalny pokój
			std::string ban_chan = get_arg(kbd_buf, pos_arg_start);

			// jeśli nie podano opcjonalnego pokoju
			if(ban_chan.size() == 0)
			{
				// bez podania pokoju banować lub odbanować można tylko w aktywnym pokoju czata, bo ten pokój jest wtedy pokojem,
				// w którym banujemy lub zdejmujemy bana
				if(ga.current < CHAN_CHAT)
				{
					irc_send(ga, ci,
						// BAN lub DBAN wpisuje polecenie BAN; BANIP lub DBANIP wpisuje polecenie BANIP
						(ban_type == "BAN" || ban_type == "DBAN" ? "CS BAN " : "CS BANIP ") + ci[ga.current]->channel
						// BAN lub BANIP wpisuje opcję ADD; DBAN lub DBANIP wpisuje opcję DEL
						+ (ban_type == "BAN" || ban_type == "BANIP" ? " ADD " : " DEL ") + ban_nick);
				}

				else
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel, (ban_type == "BAN" || ban_type == "BANIP"
								? uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Banować w takim pokoju możesz wtedy, "
								"gdy po nicku podasz pokój, w którym chcesz zbanować osobę."
								: uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Odbanować w takim pokoju możesz wtedy, "
								"gdy po nicku podasz pokój, w którym chcesz odbanować osobę."), false);
				}
			}

			// jeśli podano opcjonalny pokój
			else
			{
				// jeśli nie podano # przed nazwą pokoju, dodaj #
				if(ban_chan[0] != '#')
				{
					ban_chan.insert(0, "#");
				}

				irc_send(ga, ci,
					// BAN lub DBAN wpisuje polecenie BAN; BANIP lub DBANIP wpisuje polecenie BANIP
					(ban_type == "BAN" || ban_type == "DBAN" ? "CS BAN " : "CS BANIP ") + ban_chan
					// BAN lub BANIP wpisuje opcję ADD; DBAN lub DBANIP wpisuje opcję DEL
					+ (ban_type == "BAN" || ban_type == "BANIP" ? " ADD " : " DEL ") + ban_nick);
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_busy(struct global_args &ga, struct channel_irc *ci[])
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// zmień stan flagi busy (faktyczna zmiana zostanie odnotowana po odebraniu potwierdzenia z serwera w parserze IRC)
		irc_send(ga, ci, (ga.my_busy ? "BUSY 0" : "BUSY 1"));
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_captcha(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	if(ga.irc_ok)
	{
		msg_err_already_logged_in(ga, ci);
	}

	// gdy jest gotowość do przepisania CAPTCHA
	else if(ga.captcha_ready)
	{
		// pobierz wpisany kod CAPTCHA
		std::string captcha = get_arg(kbd_buf, pos_arg_start);

		if(captcha.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano kodu, spróbuj jeszcze raz.", false);
		}

		else if(captcha.size() != 6)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Kod musi mieć 6 znaków, spróbuj jeszcze raz.", false);
		}

		// gdy CAPTCHA ma 6 znaków, wykonaj sprawdzenie
		else
		{
			// odświeżenie stdscr ma na celu "zadziałanie" erase() po wpisaniu /cap, aby od razu zniknęło wpisane polecenie po wciśnięciu Enter
			refresh();

			if(! auth_http_checkcode(ga, ci, captcha))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, ci);

				return;
			}

			if(! auth_http_getuokey(ga, ci))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, ci);
			}
		}
	}

	// gdy brak gotowości do przepisania CAPTCHA
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Najpierw wpisz " xCYAN "/connect " xRED "lub " xCYAN "/c", false);
	}
}


void command_card(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string card_nick = get_arg(kbd_buf, pos_arg_start);

		// polecenie do IRC (można nie podawać nicka, wtedy pokaż własną wizytówkę)
		irc_send(ga, ci, "NS INFO " + (card_nick.size() > 0 ? card_nick : ga.zuousername));

		ga.cf.card = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}

}


void command_connect(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// gdy już połączono z czatem
	if(ga.irc_ok)
	{
		msg_err_already_logged_in(ga, ci);
	}

	// jeśli nie podano nicka
	else if(ga.my_nick.size() == 0)
	{
		msg_err_not_specified_nick(ga, ci);
	}

	// podano nick i można rozpocząć łączenie z czatem
	else
	{
		// komunikat o łączeniu we wszystkich otwartych pokojach (z wyjątkiem "DebugIRC" i "RawUnknown")
		win_buf_all_chan_msg(ga, ci, uINFOb xGREEN "Łączenie z czatem...");

		// rozpocznij łączenie z czatem
		if(! auth_http_init(ga, ci))
		{
			// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
			msg_err_disconnect(ga, ci);

			return;
		}

		// gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
		if(ga.my_password.size() == 0)
		{
			// pobierz CAPTCHA
			if(! auth_http_getcaptcha(ga, ci))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, ci);
			}
		}

		// gdy wpisano hasło, wykonaj część dla nicka stałego
		else
		{
			if(! auth_http_getsk(ga, ci))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, ci);

				return;
			}

			if(! auth_http_mlogin(ga, ci))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, ci);

				return;
			}

			if(! auth_http_getuokey(ga, ci))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu i zakończ
				msg_err_disconnect(ga, ci);

				return;
			}

			// wpisanie 'o' w parametrze oznacza próbę zalogowania na zalogowanym już nicku, przydaje się po zmianie IP, aby nie czekać na timeout
			if(get_arg(kbd_buf, pos_arg_start, true) == "O" && ! auth_http_useroverride(ga, ci))
			{
				// w przypadku błędu komunikat został wyświetlony, pokaż jeszcze drugi komunikat o rozłączeniu
				msg_err_disconnect(ga, ci);
			}
		}
	}
}


void command_disconnect(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string disconnect_reason = get_rest_args(kbd_buf, pos_arg_start);

		// komunikat o rozłączaniu we wszystkich otwartych pokojach (z wyjątkiem "DebugIRC" i "RawUnknown")
		win_buf_all_chan_msg(ga, ci, uINFOb xYELLOW_BLACK "Rozłączanie z czatem...");

		// jeśli podano argument (tekst pożegnalny), wstaw go, jeśli nie podano argumentu, wyślij samo polecenie
		irc_send(ga, ci, (disconnect_reason.size() > 0 ? "QUIT :" + disconnect_reason : "QUIT"));
	}

	// jeśli nie połączono z IRC, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie zalogowano się.", false);
	}
}


void command_favourites_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, bool add_del)
{
	// add_del: true - dodaje pokój do listy ulubionych, false - usuwa pokój z listy ulubionych

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz wpisany pokój
		std::string favourites_chan = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano pokoju, parser IRC pokaże listę ulubionych pokoi (tylko, gdy to dodawanie pokoju, przy usuwaniu pokaż ostrzeżenie)
		if(favourites_chan.size() == 0)
		{
			if(add_del)
			{
				irc_send(ga, ci, "NS FAVOURITES LIST");

				ga.cf.favourites_empty = true;
			}

			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano pokoju do usunięcia z listy ulubionych.", false);
			}
		}

		else
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #
			if(favourites_chan[0] != '#')
			{
				favourites_chan.insert(0, "#");
			}

			irc_send(ga, ci, (add_del ? "NS FAVOURITES ADD " : "NS FAVOURITES DEL ") + favourites_chan);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_friends_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, bool add_del)
{
	// add_del: true - dodaje osobę do listy przyjaciół, false - usuwa osobę z listy przyjaciół

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz wpisany nick
		std::string friends_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, parser IRC pokaże listę przyjaciół (tylko, gdy to dodawanie nicka, przy usuwaniu pokaż ostrzeżenie)
		if(friends_nick.size() == 0)
		{
			if(add_del)
			{
				irc_send(ga, ci, "NS FRIENDS LIST");

				ga.cf.friends_empty = true;
			}

			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do usunięcia z listy ulubionych.", false);
			}
		}

		else
		{
			irc_send(ga, ci, (add_del ? "NS FRIENDS ADD " : "NS FRIENDS DEL ") + friends_nick);

			ga.cf.friends = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_help(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			uINFOn xGREEN "Dostępne polecenia (w kolejności alfabetycznej):\n"
			xCYAN  "/away\n"
			xCYAN  "/ban\n"
			xCYAN  "/banip\n"
			xCYAN  "/busy\n"
			xCYAN  "/captcha" xGREEN "      lub  " xCYAN "/cap\n"
			xCYAN  "/card\n"
			xCYAN  "/connect" xGREEN "      lub  " xCYAN "/c\n"
			xCYAN  "/dban\n"
			xCYAN  "/dbanip\n"
			xCYAN  "/dfavourites" xGREEN "  lub  " xCYAN "/dfav\n"
			xCYAN  "/dfriends" xGREEN "     lub  " xCYAN "/df\n"
			xCYAN  "/dignore" xGREEN "      lub  " xCYAN "/dign\n"
			xCYAN  "/disconnect" xGREEN "   lub  " xCYAN "/d\n"
			xCYAN  "/dop\n"
			xCYAN  "/dsop\n"
			xCYAN  "/dvip\n"
			xCYAN  "/favourites" xGREEN "   lub  " xCYAN "/fav\n"
			xCYAN  "/friends" xGREEN "      lub  " xCYAN "/f\n"
			xCYAN  "/help" xGREEN "         lub  " xCYAN "/h\n"
			xCYAN  "/homes\n"
			xCYAN  "/ignore" xGREEN "       lub  " xCYAN "/ign\n"
			xCYAN  "/info\n"
			xCYAN  "/invite" xGREEN "       lub  " xCYAN "/inv\n"
			xCYAN  "/join" xGREEN "         lub  " xCYAN "/j\n"
			xCYAN  "/kban\n"
//			xCYAN  "/kbanip\n"
			xCYAN  "/kick\n"
			xCYAN  "/list" xGREEN "         lub  " xCYAN "/l\n"
			xCYAN  "/lusers" xGREEN "       lub  " xCYAN "/lu\n"
			xCYAN  "/me\n"
			xCYAN  "/names" xGREEN "        lub  " xCYAN "/n\n"
			xCYAN  "/nick\n"
			xCYAN  "/op\n"
			xCYAN  "/part" xGREEN "         lub  " xCYAN "/p\n"
			xCYAN  "/priv\n"
			xCYAN  "/quit" xGREEN "         lub  " xCYAN "/q\n"
			xCYAN  "/raw\n"
			xCYAN  "/sop\n"
			xCYAN  "/time\n"
			xCYAN  "/topic\n"
			xCYAN  "/vhost\n"
			xCYAN  "/vip\n"
			xCYAN  "/whois\n"
			xCYAN  "/whowas", false);
			// dopisać resztę poleceń
}


void command_homes(struct global_args &ga, struct channel_irc *ci[])
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		irc_send(ga, ci, "CS HOMES LIST");
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_ignore_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, bool add_del)
{
	// add_del: true - dodaje osobę do listy ignorowanych, false - usuwa osobę z listy ignorowanych

	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz wpisany nick
		std::string ignore_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, parser IRC pokaże listę ignorowanych (tylko, gdy to dodawanie nicka, przy usuwaniu pokaż ostrzeżenie)
		if(ignore_nick.size() == 0)
		{
			if(add_del)
			{
				irc_send(ga, ci, "NS IGNORE LIST");

				ga.cf.ignore_empty = true;
			}

			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do usunięcia z listy ignorowanych.", false);
			}
		}

		else
		{
			irc_send(ga, ci, (add_del ? "NS IGNORE ADD " : "NS IGNORE DEL ") + ignore_nick);

			ga.cf.ignore = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_info(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz wpisany pokój do pokazania
		std::string info_chan = get_arg(kbd_buf, pos_arg_start);

		// jeśli wpisano pokój do pokazania
		if(info_chan.size() > 0)
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #
			if(info_chan[0] != '#')
			{
				info_chan.insert(0, "#");
			}

			irc_send(ga, ci, "CS INFO " + info_chan);
		}

		// jeśli nie wpisano pokoju, pokaż info o aktualnie otwartym pokoju (o ile to aktywny pokój czata)
		else
		{
			if(ga.current < CHAN_CHAT)
			{
				irc_send(ga, ci, "CS INFO " + ci[ga.current]->channel);
			}

			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel,
						uINFOn xRED "Nie jesteś w aktywnym pokoju czata. W tej sytuacji musisz podać pokój, który chcesz sprawdzić.",
						false);
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_invite(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz wpisany nick do zaproszenia
		std::string invite_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(invite_nick.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do zaproszenia.", false);
		}

		else
		{
			// pobierz opcjonalny pokój
			std::string invite_chan = get_arg(kbd_buf, pos_arg_start);

			// jeśli nie podano opcjonalnego pokoju
			if(invite_chan.size() == 0)
			{
				// bez podania pokoju zapraszać można tylko z aktywnego pokoju czata, bo ten pokój jest wtedy pokojem, do którego zapraszamy
				if(ga.current < CHAN_CHAT)
				{
					irc_send(ga, ci, "INVITE " + invite_nick + " " + ci[ga.current]->channel);

					ga.cf.invite = true;
				}

				else
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel,
							uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Zapraszać z takiego pokoju możesz wtedy, gdy po "
							"nicku podasz pokój, do którego chcesz zaprosić osobę.", false);
				}
			}

			// jeśli podano opcjonalny pokój
			else
			{
				// jeśli nie podano # przed nazwą pokoju, dodaj #
				if(invite_chan[0] != '#')
				{
					invite_chan.insert(0, "#");
				}

				irc_send(ga, ci, "INVITE " + invite_nick + " " + invite_chan);

				ga.cf.invite = true;
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_join(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string join_chan = get_arg(kbd_buf, pos_arg_start);

		if(join_chan.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano pokoju.", false);
		}

		else
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #, ale tylko wtedy, gdy nie podano ^, aby można było wejść na priv
			if(join_chan[0] != '#' && join_chan[0] != '^')
			{
				join_chan.insert(0, "#");
			}

			// opcjonalnie można podać klucz do pokoju
			std::string join_key = get_arg(kbd_buf, pos_arg_start);

			irc_send(ga, ci, "JOIN " + join_chan + (join_key.size() > 0 ? " " + join_key : ""));

			ga.cf.join_priv = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_kban_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &kban_type)
{
	// kban_type może przyjąć następujące wartości: KBAN, KBANIP
	if(kban_type != "KBAN" && kban_type != "KBANIP")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nieprawidłowa opcja dla command_kban_common()");
	}

	// jeśli połączono z IRC
	else if(ga.irc_ok)
	{
		// w aktywnym pokoju czata można zbanować i wyrzucić użytkownika
		if(ga.current < CHAN_CHAT)
		{
			// pobierz wpisany nick do zbanowania i wyrzucenia
			std::string kban_nick = get_arg(kbd_buf, pos_arg_start);

			// jeśli wpisano nick do zbanowania i wyrzucenia, wyślij polecenie do IRC (w aktualnie otwartym pokoju), opcjonalnie można podać powód
			if(kban_nick.size() > 0)
			{
				std::string kban_nick_key = buf_lower2upper(kban_nick);

				// polecenie wyślij, gdy nick jest w pokoju
				if(ci[ga.current]->ni.find(kban_nick_key) != ci[ga.current]->ni.end())
				{
					// KBAN wpisuje polecenie BAN; KBANIP wpisuje polecenie BANIP
					irc_send(ga, ci, (kban_type == "KBAN" ? "CS BAN " : "CS BANIP ") + ci[ga.current]->channel + " ADD " + kban_nick);

					// zapisz nick, pokój i ewentualny powód, gdy serwer odpowie na CS BAN, to wyśle KICK
					ga.kb[kban_nick_key].nick = kban_nick;
					ga.kb[kban_nick_key].chan = ci[ga.current]->channel;
					ga.kb[kban_nick_key].reason = get_rest_args(kbd_buf, pos_arg_start);
				}

				// jeśli nicka nie ma w pokoju, pokaż ostrzeżenie
				else
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel,
							uINFOn xRED + kban_nick + " nie przebywa w pokoju " + ci[ga.current]->channel, false);
				}
			}

			// jeśli nie wpisano nicka, pokaż ostrzeżenie
			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do zbanowania i wyrzucenia.", false);
			}
		}

		// jeśli nie przebywamy w aktywnym pokoju czata, nie można wyrzucić użytkownika, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, ci);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_kick(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// w aktywnym pokoju czata można wyrzucić użytkownika
		if(ga.current < CHAN_CHAT)
		{
			// pobierz wpisany nick do wyrzucenia
			std::string kick_nick = get_arg(kbd_buf, pos_arg_start);

			// jeśli wpisano nick do wyrzucenia, wyślij polecenie do IRC (w aktualnie otwartym pokoju), opcjonalnie można podać powód wyrzucenia
			if(kick_nick.size() > 0)
			{
				irc_send(ga, ci, "KICK " + ci[ga.current]->channel + " " + kick_nick + " :" + get_rest_args(kbd_buf, pos_arg_start));

				ga.cf.kick = true;
			}

			// jeśli nie wpisano nicka, pokaż ostrzeżenie
			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do wyrzucenia.", false);
			}
		}

		// jeśli nie przebywamy w aktywnym pokoju czata, nie można wyrzucić użytkownika, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, ci);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_list(struct global_args &ga, struct channel_irc *ci[])
{
	std::string chan_nr;

	win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xGREEN "Aktualnie otwarte pokoje:", false);

	for(int i = 0; i < CHAN_MAX; ++i)
	{
		if(ci[i])
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

			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					uINFOb + chan_nr + (i >= 9 && i < CHAN_CHAT ? xNORMAL " " : xNORMAL "  ") + ci[i]->channel, false);
		}
	}
}


void command_lusers(struct global_args &ga, struct channel_irc *ci[])
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// polecenie do IRC
		irc_send(ga, ci, "LUSERS");

		ga.cf.lusers = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_me(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// w aktywnym pokoju czata można wysłać wiadomość
		if(ga.current < CHAN_CHAT)
		{
			// pobierz wpisany komunikat dla /me (nie jest niezbędny)
			std::string me_reason = get_rest_args(kbd_buf, pos_arg_start);

			// komunikat do wyświetlenia w oknie terminala
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					xBOLD_ON xMAGENTA "* " + ga.zuousername + xNORMAL " " + form_from_chat(me_reason), true, 2);

			// wyślij też komunikat do serwera IRC
			irc_send(ga, ci, "PRIVMSG " + ci[ga.current]->channel + " :\x01" "ACTION " + me_reason + "\x01");
		}

		// jeśli nie przebywamy w aktywnym pokoju czata, wiadomości nie można wysłać, więc pokaż ostrzeżenie
		else
		{
			msg_err_not_active_chan(ga, ci);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_names(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// /names może działać z parametrem lub bez (wtedy dotyczy aktywnego pokoju)
		std::string names_chan = get_arg(kbd_buf, pos_arg_start);

		// gdy podano pokój do sprawdzenia
		if(names_chan.size() > 0)
		{
			// jeśli nie podano # przed nazwą pokoju, dodaj #, ale tylko wtedy, gdy nie podano ^, aby można było sprawdzić priv
			if(names_chan[0] != '#' && names_chan[0] != '^')
			{
				names_chan.insert(0, "#");
			}

			// wyślij komunikat do serwera IRC
			irc_send(ga, ci, "NAMES " + names_chan);

			ga.cf.names = true;
		}

		// gdy nie podano pokoju sprawdź, czy pokój jest aktywny (bo /names dla "Status", "DebugIRC" lub "RawUnknown" nie ma sensu)
		else
		{
			if(ga.current < CHAN_CHAT)
			{
				irc_send(ga, ci, "NAMES " + ci[ga.current]->channel);

				ga.cf.names = true;
				ga.cf.names_empty = true;
			}

			else
			{
				msg_err_not_active_chan(ga, ci);
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_nick(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// po połączeniu z IRC nie można zmienić nicka
	if(ga.irc_ok)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Po zalogowaniu się nie można zmienić nicka.", false);
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
				msg_err_not_specified_nick(ga, ci);
			}

			// wpisanie /nick bez parametrów, gdy wcześniej był już podany, powoduje wypisanie aktualnego nicka
			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, (ga.my_password.size() == 0
						? uINFOn xGREEN "Aktualny nick tymczasowy: "
						: uINFOn xGREEN "Aktualny nick stały: ")
						+ ga.my_nick, false);
			}
		}

		else
		{
			ga.my_password = get_arg(kbd_buf, pos_arg_start);

			if(buf_chars(nick) < 3)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Wpisany nick jest za krótki (minimalnie 3 znaki).", false);
			}

			// dla nicka tymczasowego trzeba uwzględnić znak tyldy po zalogowaniu, dlatego maksymalna długość jest mniejsza niż dla stałego
			else if(ga.my_password.size() == 0 && buf_chars(nick) > 31)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel,
						uINFOn xRED "Wpisany nick jest za długi (maksymalnie 31 znaków dla nicka tymczasowego).", false);
			}

			else if(buf_chars(nick) > 32)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel,
						uINFOn xRED "Wpisany nick jest za długi (maksymalnie 32 znaki dla nicka stałego).", false);
			}

			// gdy wpisano nick mieszczący się w zakresie odpowiedniej liczby znaków
			else
			{
				// przepisz nick
				ga.my_nick = nick;

				// wyświetl wpisany nick
				win_buf_add_str(ga, ci, ci[ga.current]->channel, (ga.my_password.size() == 0
						? uINFOn xGREEN "Wpisany nick tymczasowy: "
						: uINFOn xGREEN "Wpisany nick stały: ")
						+ ga.my_nick, false);
			}
		}
	}
}


void command_op_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &op_type)
{
	// op_type może przyjąć następujące wartości: OP, SOP, DOP, DSOP
	if(op_type != "OP" && op_type != "SOP" && op_type != "DOP" && op_type != "DSOP")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nieprawidłowa opcja dla command_op_common()");
	}

	// jeśli połączono z IRC
	else if(ga.irc_ok)
	{
		// pobierz wpisany nick
		std::string op_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(op_nick.size() == 0)
		{
			if(op_type == "OP")
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do nadania uprawnień operatora.", false);
			}

			else if(op_type == "SOP")
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do nadania uprawnień superoperatora.", false);
			}

			else if(op_type == "DOP")
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka do odebrania uprawnień operatora.", false);
			}

			else
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel,
						uINFOn xRED "Nie podano nicka do odebrania uprawnień superoperatora.", false);
			}
		}

		else
		{
			// pobierz opcjonalny pokój
			std::string op_chan = get_arg(kbd_buf, pos_arg_start);

			// jeśli nie podano opcjonalnego pokoju
			if(op_chan.size() == 0)
			{
				// bez podania pokoju dać/zabrać opa/sopa można tylko w aktywnym pokoju czata, bo ten pokój jest wtedy pokojem,
				// w którym to robimy
				if(ga.current < CHAN_CHAT)
				{
					if(op_type == "OP")
					{
						irc_send(ga, ci, "CS HALFOP " + ci[ga.current]->channel + " ADD " + op_nick);
					}

					else if(op_type == "SOP")
					{
						irc_send(ga, ci, "CS OP " + ci[ga.current]->channel + " ADD " + op_nick);
					}

					else if(op_type == "DOP")
					{
						irc_send(ga, ci, "CS HALFOP " + ci[ga.current]->channel + " DEL " + op_nick);
					}

					else
					{
						irc_send(ga, ci, "CS OP " + ci[ga.current]->channel + " DEL " + op_nick);
					}
				}

				else
				{
					if(op_type == "OP")
					{
						win_buf_add_str(ga, ci, ci[ga.current]->channel,
								uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Nadać uprawnienia operatora w takim "
								"pokoju możesz wtedy, gdy po nicku podasz pokój, w którym chcesz nadać uprawnienia.", false);
					}

					else if(op_type == "SOP")
					{
						win_buf_add_str(ga, ci, ci[ga.current]->channel,
								uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Nadać uprawnienia superoperatora w takim "
								"pokoju możesz wtedy, gdy po nicku podasz pokój, w którym chcesz nadać uprawnienia.", false);
					}

					else if(op_type == "DOP")
					{
						win_buf_add_str(ga, ci, ci[ga.current]->channel,
								uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Odebrać uprawnienia operatora w takim "
								"pokoju możesz wtedy, gdy po nicku podasz pokój, w którym chcesz zabrać uprawnienia.", false);
					}

					else
					{
						win_buf_add_str(ga, ci, ci[ga.current]->channel,
								uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Odebrać uprawnienia superoperatora w takim "
								"pokoju możesz wtedy, gdy po nicku podasz pokój, w którym chcesz zabrać uprawnienia.", false);
					}
				}
			}

			// jeśli podano opcjonalny pokój
			else
			{
				// jeśli nie podano # przed nazwą pokoju, dodaj #
				if(op_chan[0] != '#')
				{
					op_chan.insert(0, "#");
				}

				if(op_type == "OP")
				{
					irc_send(ga, ci, "CS HALFOP " + op_chan + " ADD " + op_nick);
				}

				else if(op_type == "SOP")
				{
					irc_send(ga, ci, "CS OP " + op_chan + " ADD " + op_nick);
				}

				else if(op_type == "DOP")
				{
					irc_send(ga, ci, "CS HALFOP " + op_chan + " DEL " + op_nick);
				}

				else
				{
					irc_send(ga, ci, "CS OP " + op_chan + " DEL " + op_nick);
				}
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_part(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// w oknie "Status" i "DebugIRC" pokaż ostrzeżenie, że tych okien nie można zamknąć
	if(ga.current == CHAN_STATUS || ga.current == CHAN_DEBUG_IRC)
	{
		msg_err_not_active_chan(ga, ci);
	}

	// jako wyjątek /part w oknie "RawUnknown" zamyka je
	else if(ga.current == CHAN_RAW_UNKNOWN)
	{
		// zamknij log
		if(ci[CHAN_RAW_UNKNOWN]->chan_log.is_open())
		{
			ci[CHAN_RAW_UNKNOWN]->chan_log << LOG_STOPPED;

			ci[CHAN_RAW_UNKNOWN]->chan_log.close();
		}

		// tymczasowo przełącz na "Status", potem przerobić, aby przechodziło do poprzedniego, który był otwarty
		ga.current = CHAN_STATUS;
		ga.win_chat_refresh = true;

		// usuń kanał "RawUnknown"
		delete ci[CHAN_RAW_UNKNOWN];

		// wyzeruj go w tablicy, w ten sposób wiadomo, że już nie istnieje
		ci[CHAN_RAW_UNKNOWN] = 0;
	}

	// ta opcja dotyczy /part po zalogowaniu się na czat i dotyczy wyłącznie pokoi czata, ale nie trzeba już ich sprawdzać w warunku, bo wyżej zostało
	// wykluczone, że /part zadziała w niewłaściwym pokoju/oknie
	else if(ga.irc_ok)
	{
		std::string part_reason = get_rest_args(kbd_buf, pos_arg_start);

		// jeśli podano powód wyjścia, wyślij go, w przeciwnym razie wyślij samo polecenie
		irc_send(ga, ci, "PART " + ci[ga.current]->channel + (part_reason.size() ? " :" + part_reason : ""));
	}

	// gdy jesteśmy rozłączeni, daj możliwość zamknięcia pokoi czata (wtedy nie dostaniemy odpowiedzi PART z serwera), nie trzeba sprawdzać warunku,
	// bo wyżej zostało wykluczone, że /part zadziała w niewłaściwym pokoju/oknie
	else
	{
		del_chan_chat(ga, ci, ci[ga.current]->channel);
	}
}


void command_priv(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// pobierz nick, który zapraszamy do rozmowy prywatnej
		std::string priv_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(priv_nick.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka zapraszanego do rozmowy prywatnej.", false);
		}

		else
		{
			// wyślij polecenie do IRC
			irc_send(ga, ci, "PRIV " + priv_nick);

			ga.cf.join_priv = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_quit(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string quit_reason = get_rest_args(kbd_buf, pos_arg_start);

		// komunikat o rozłączaniu we wszystkich otwartych pokojach (z wyjątkiem "DebugIRC" i "RawUnknown")
		win_buf_all_chan_msg(ga, ci, uINFOb xYELLOW_BLACK "Rozłączanie z czatem...");

		// jeśli podano powód wyjścia, wyślij go, jeśli nie podano argumentu, wyślij samo polecenie
		irc_send(ga, ci, (quit_reason.size() > 0 ? "QUIT :" + quit_reason : "QUIT"));

		// zamknięcie programu z czekaniem na odpowiedź serwera (lub przy braku odpowiedzi zamknięcie po wyznaczonym czasie)
		ga.ucc_quit_time = true;
	}

	// jeśli nie połączono z IRC
	else
	{
		// zamknięcie programu od razu
		ga.ucc_quit = true;
	}


}


void command_raw(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string raw_args = get_rest_args(kbd_buf, pos_arg_start);

		raw_args.size() > 0
		// polecenie do IRC
		? irc_send(ga, ci, raw_args)
		// jeśli nie podano parametrów, pokaż ostrzeżenie
		: win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano argumentu lub argumentów.", false);

		// wykryj wysłanie wybranych poleceń przez /raw i na tej podstawie ustaw stosowne flagi
		pos_arg_start = 0;	// początek pobranej reszty z bufora

		std::string raw_command = buf_lower2upper(get_arg(raw_args, pos_arg_start));

		if(raw_command == "NS")
		{
			std::string raw_arg1 = buf_lower2upper(get_arg(raw_args, pos_arg_start));
			std::string raw_arg2 = buf_lower2upper(get_arg(raw_args, pos_arg_start));

			if(raw_arg1 == "INFO")
			{
				// odpowiada poleceniu /card
				ga.cf.card = true;
			}

			else if(raw_arg1 == "FAVOURITES")
			{
				// uproszczono założenie, że zawsze wpisano wersję bez nicka, bo w parserze IRC to i tak nie ma znaczenia
				ga.cf.favourites_empty = true;
			}

			else if(raw_arg1 == "FRIENDS")
			{
				if(raw_arg2 == "ADD" || raw_arg2 == "DEL")
				{
					// odpowiada poleceniu /(d)friends nick
					ga.cf.friends = true;
				}

				// założono, że inny argument oznacza wersję bez nicka
				else
				{
					// odpowiada poleceniu /(d)friends (bez parametrów)
					ga.cf.friends_empty = true;
				}
			}

			else if(raw_arg1 == "IGNORE")
			{
				if(raw_arg2 == "ADD" || raw_arg2 == "DEL")
				{
					// odpowiada poleceniu /(d)ignore nick
					ga.cf.ignore = true;
				}

				// założono, że inny argument oznacza wersję bez nicka
				else
				{
					// odpowiada poleceniu /(d)ignore (bez parametrów)
					ga.cf.ignore_empty = true;
				}
			}
		}

		else if(raw_command == "INVITE")
		{
			ga.cf.invite = true;
		}

		else if(raw_command == "JOIN" || raw_command == "PRIV")
		{
			ga.cf.join_priv = true;
		}

		else if(raw_command == "KICK")
		{
			ga.cf.kick = true;
		}

		else if(raw_command == "LUSERS")
		{
			ga.cf.lusers = true;
		}

		else if(raw_command == "NAMES")
		{
			ga.cf.names = true;
		}

		else if(raw_command == "VHOST")
		{
			ga.cf.vhost = true;
		}

		else if(raw_command == "WHOIS")
		{
			ga.cf.whois = true;
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_set(struct global_args &ga, std::string &kbd_buf, size_t pos_arg_start)
{
	std::string set_parm = buf_lower2upper(get_arg(kbd_buf, pos_arg_start));

	if(set_parm == "STAT")
	{
		std::string set_arg = get_arg(kbd_buf, pos_arg_start);

		if(set_arg == "0")
		{
			ga.show_stat_in_win_chat = false;
		}

		else if(set_arg == "1")
		{
			ga.show_stat_in_win_chat = true;
		}
	}

	else if(set_parm == "SSL")
	{
		std::string set_arg = get_arg(kbd_buf, pos_arg_start);

		if(set_arg == "0")
		{
			ga.all_auth_https = false;
		}

		else if(set_arg == "1")
		{
			ga.all_auth_https = true;
		}
	}
}


void command_time(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string time_serv = get_arg(kbd_buf, pos_arg_start);

		// opcjonalnie można podać serwer
		irc_send(ga, ci, (time_serv.size() > 0 ? "TIME " + time_serv : "TIME"));
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_topic(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		// temat można zmienić tylko w aktywnym oknie czata (nie w "Status", "DebugIRC" i "RawUnknown")
		if(ga.current < CHAN_CHAT)
		{
			std::string topic = get_rest_args(kbd_buf, pos_arg_start);

			// serwer ogranicza tekst do 200 znaków
			if(topic.size() > 200)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Temat za długi, maksymalnie może zawierać 200 znaków.", false);
			}

			// ustaw temat, jeśli za /topic wpisano jakiś tekst
			else if(topic.size() > 0)
			{
				irc_send(ga, ci, "CS SET " + ci[ga.current]->channel + " TOPIC " + topic);
			}

			// wyczyść temat, jeśli za /topic wpisano dwie lub więcej spacji
			else if(topic.size() == 0 && kbd_buf.size() > 7)	// > 7, czyli więcej od ciągu /topic + przynajmniej dwie spacje
			{
				irc_send(ga, ci, "CS SET " + ci[ga.current]->channel + " TOPIC");
			}

			// nie zmieniaj tematu (tylko go pokaż), jeśli za /topic nic nie wpisano lub wpisano jedną spację
			else
			{
				irc_send(ga, ci, "TOPIC " + ci[ga.current]->channel);
			}
		}

		else
		{
			msg_err_not_active_chan(ga, ci);
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_vhost(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string vhost_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie podano parametrów, pokaż ostrzeżenie
		if(vhost_nick.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano nicka i hasła dla VHost.", false);
		}

		// jeśli podano nick, sprawdź, czy podano hasło
		else
		{
			std::string vhost_password = get_arg(kbd_buf, pos_arg_start);

			// jeśli nie podano hasła, pokaż ostrzeżenie
			if(vhost_password.size() == 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nie podano hasła dla VHost.", false);
			}

			// jeśli podano hasło (i wcześniej nick), wyślij je na serwer
			else
			{
				// wyślij nick i hasło dla VHost
				irc_send(ga, ci, "VHOST " + vhost_nick + " " + vhost_password);

				ga.cf.vhost = true;
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_vip_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &vip_type)
{
	// vip_type może przyjąć następujące wartości: VIP, DVIP
	if(vip_type != "VIP" && vip_type != "DVIP")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Nieprawidłowa opcja dla command_vip_common()");
	}

	// jeśli połączono z IRC
	else if(ga.irc_ok)
	{
		// pobierz wpisany nick
		std::string vip_nick = get_arg(kbd_buf, pos_arg_start);

		// jeśli nie wpisano nicka, pokaż ostrzeżenie
		if(vip_nick.size() == 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, (vip_type == "VIP"
					? uINFOn xRED "Nie podano nicka do nadania uprawnień vipa."
					: uINFOn xRED "Nie podano nicka do odebrania uprawnień vipa."), false);
		}

		else
		{
			// pobierz opcjonalny pokój
			std::string vip_chan = get_arg(kbd_buf, pos_arg_start);

			// jeśli nie podano opcjonalnego pokoju
			if(vip_chan.size() == 0)
			{
				// bez podania pokoju nadać/odebrać uprawnienia vipa można tylko w aktywnym pokoju czata, bo ten pokój jest wtedy pokojem,
				// w którym to robimy
				if(ga.current < CHAN_CHAT)
				{
					// VIP wpisuje opcję ADD; DVIP wpisuje opcję DEL
					irc_send(ga, ci, "CS VOICE " + ci[ga.current]->channel + (vip_type == "VIP" ? " ADD " : " DEL ") + vip_nick);
				}

				else
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel, (vip_type == "VIP"
								? uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Nadać uprawnienia vipa w takim pokoju "
								"możesz wtedy, gdy po nicku podasz pokój, w którym chcesz zbanować osobę."
								: uINFOn xRED "Nie jesteś w aktywnym pokoju czata. Odebrać uprawnienia vipa w takim pokoju "
								"możesz wtedy, gdy po nicku podasz pokój, w którym chcesz odbanować osobę."), false);
				}
			}

			// jeśli podano opcjonalny pokój
			else
			{
				// jeśli nie podano # przed nazwą pokoju, dodaj #
				if(vip_chan[0] != '#')
				{
					vip_chan.insert(0, "#");
				}

				// VIP wpisuje opcję ADD; DVIP wpisuje opcję DEL
				irc_send(ga, ci, "CS VOICE " + vip_chan + (vip_type == "VIP" ? " ADD " : " DEL ") + vip_nick);
			}
		}
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_whois(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string whois_nick = get_arg(kbd_buf, pos_arg_start);

		// polecenie do IRC (jeśli nie podano nicka, użyj własnego, podano 2x nick, aby zawsze pokazało idle)
		irc_send(ga, ci, "WHOIS " + (whois_nick.size() > 0 ? whois_nick + " " + whois_nick : ga.zuousername + " " + ga.zuousername));

		// wpisanie 's' za nickiem wyświetli krótką formę (tylko nick, ZUO i host)
		if(get_arg(kbd_buf, pos_arg_start, true) == "S")
		{
                        ga.whois_short = buf_lower2upper(whois_nick);
		}

		ga.cf.whois = true;
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}


void command_whowas(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start)
{
	// jeśli połączono z IRC
	if(ga.irc_ok)
	{
		std::string whowas_nick = get_arg(kbd_buf, pos_arg_start);

		// polecenie do IRC (jeśli nie podano nicka, użyj własnego)
		irc_send(ga, ci, "WHOWAS " + (whowas_nick.size() > 0 ? whowas_nick : ga.zuousername));
	}

	// jeśli nie połączono z IRC, pokaż ostrzeżenie
	else
	{
		msg_err_first_login(ga, ci);
	}
}
