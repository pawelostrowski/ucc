/*
	Ucieszony Chat Client
	Copyright (C) 2013, 2014 Paweł Ostrowski

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

#include "chat_utils.hpp"
#include "window_utils.hpp"
#include "ucc_global.hpp"


void new_chan_status(struct global_args &ga, struct channel_irc *ci[])
{
	if(ci[CHAN_STATUS] == 0)
	{
		ci[CHAN_STATUS] = new channel_irc;

		ci[CHAN_STATUS]->channel = "Status";
		ci[CHAN_STATUS]->topic = UCC_NAME " " UCC_VER;	// napis wyświetlany na górnym pasku

		ci[CHAN_STATUS]->chan_act = 0;			// zacznij od braku aktywności kanału
		ci[CHAN_STATUS]->win_scroll_lock = false;	// ciągłe przesuwanie aktualnego tekstu
		ci[CHAN_STATUS]->lock_act = 0;
		ci[CHAN_STATUS]->win_pos_first = 0;
		ci[CHAN_STATUS]->win_skip_lead_first = 0;
		ci[CHAN_STATUS]->win_pos_last = 0;
		ci[CHAN_STATUS]->win_skip_lead_last = 0;

		// ustaw nowoutworzony kanał jako aktywny
		ga.current = CHAN_STATUS;
	}
}


void new_chan_debug_irc(struct global_args &ga, struct channel_irc *ci[])
{
	if(ci[CHAN_DEBUG_IRC] == 0)
	{
		ci[CHAN_DEBUG_IRC] = new channel_irc;

		ci[CHAN_DEBUG_IRC]->channel = "DebugIRC";
		ci[CHAN_DEBUG_IRC]->topic = "Surowe dane przesyłane między programem a serwerem (tylko IRC).";

		ci[CHAN_DEBUG_IRC]->chan_act = 0;		// zacznij od braku aktywności kanału
		ci[CHAN_DEBUG_IRC]->win_scroll_lock = false;	// ciągłe przesuwanie aktualnego tekstu
		ci[CHAN_DEBUG_IRC]->lock_act = 0;
		ci[CHAN_DEBUG_IRC]->win_pos_first = 0;
		ci[CHAN_DEBUG_IRC]->win_skip_lead_first = 0;
		ci[CHAN_DEBUG_IRC]->win_pos_last = 0;
		ci[CHAN_DEBUG_IRC]->win_skip_lead_last = 0;
	}
}


void new_chan_raw_unknown(struct global_args &ga, struct channel_irc *ci[])
{
	if(ci[CHAN_RAW_UNKNOWN] == 0)
	{
		ci[CHAN_RAW_UNKNOWN] = new channel_irc;

		ci[CHAN_RAW_UNKNOWN]->channel = "RawUnknown";
		ci[CHAN_RAW_UNKNOWN]->topic = "Nieznane lub niezaimplementowane komunikaty pobrane z serwera.";

		ci[CHAN_RAW_UNKNOWN]->chan_act = 0;		// zacznij od braku aktywności kanału
		ci[CHAN_RAW_UNKNOWN]->win_scroll_lock = false;	// ciągłe przesuwanie aktualnego tekstu
		ci[CHAN_RAW_UNKNOWN]->lock_act = 0;
		ci[CHAN_RAW_UNKNOWN]->win_pos_first = 0;
		ci[CHAN_RAW_UNKNOWN]->win_skip_lead_first = 0;
		ci[CHAN_RAW_UNKNOWN]->win_pos_last = 0;
		ci[CHAN_RAW_UNKNOWN]->win_skip_lead_last = 0;

		ci[CHAN_RAW_UNKNOWN]->chan_log.open(ga.user_dir + "/" + ci[CHAN_RAW_UNKNOWN]->channel + ".txt", std::ios::out | std::ios::app);

		if(ci[CHAN_RAW_UNKNOWN]->chan_log.good())
		{
			ci[CHAN_RAW_UNKNOWN]->chan_log << LOG_STARTED;
			ci[CHAN_RAW_UNKNOWN]->chan_log.flush();
		}
	}
}


bool new_chan_chat(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, bool active)
{
/*
	Utwórz nowy kanał czata w programie. Poniższa pętla wyszukuje pierwsze wolne miejsce w tablicy i wtedy tworzy kanał.
*/

	// pierwsza pętla wyszukuje, czy kanał o podanej nazwie istnieje, jeśli tak, nie będzie tworzony drugi o takiej samej nazwie
	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// nie twórz dwóch kanałów o takiej samej nazwie
		if(ci[i] && ci[i]->channel == chan_name)
		{
			// co prawda nie utworzono nowego kanału, ale nie jest to błąd, bo kanał już istnieje, dlatego zakończ z kodem sukcesu
			return true;
		}
	}

	// druga pętla szuka pierwszego wolnego kanału w tablicy pokoi i jeśli są wolne miejsca, to go tworzy
	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		if(ci[i] == 0)
		{
			ci[i] = new channel_irc;

			ci[i]->channel = chan_name;		// nazwa pokoju czata

			ci[i]->chan_act = 0;			// zacznij od braku aktywności kanału
			ci[i]->win_scroll_lock = false;		// ciągłe przesuwanie aktualnego tekstu
			ci[i]->lock_act = 0;
			ci[i]->win_pos_first = 0;
			ci[i]->win_skip_lead_first = 0;
			ci[i]->win_pos_last = 0;
			ci[i]->win_skip_lead_last = 0;

			ci[i]->chan_log.open(ga.user_dir + "/" + ci[i]->channel + ".txt", std::ios::out | std::ios::app);

			if(ci[i]->chan_log.good())
			{
				ci[i]->chan_log << LOG_STARTED;
				ci[i]->chan_log.flush();
			}

			// jeśli trzeba, kanał oznacz jako aktywny (przełącz na to okno), czyli tylko po wpisaniu /join lub /priv
			if(active)
			{
				ga.current = i;

				// wyczyść okno (by nie było zawartości poprzedniego okna na ekranie)
				wclear(ga.win_chat);

				// ustaw w nim kursor na pozycji początkowej
				wmove(ga.win_chat, 0, 0);
			}

			// gdy utworzono kanał, przerwij szukanie wolnego kanału z kodem sukcesu
			return true;
		}
	}

	// zakończ z błędem, gdy nie znaleziono wolnych miejsc w tablicy
	return false;
}


void del_chan_chat(struct global_args &ga, struct channel_irc *ci[], std::string chan_name)
{
/*
	Usuń kanał czata w programie.
*/

	for(int i = 0; i < CHAN_CHAT; ++i)	// pokoje inne, niż pokoje czata nie mogą zostać usunięte (podczas normalnego działania programu)
	{
		// znajdź po nazwie kanału jego numer w tablicy
		if(ci[i] && ci[i]->channel == chan_name)
		{
			// zamknij log
			if(ci[i]->chan_log.is_open())
			{
				ci[i]->chan_log << LOG_STOPPED;

				ci[i]->chan_log.close();
			}

			// tymczasowo przełącz na "Status", potem przerobić, aby przechodziło do poprzedniego, który był otwarty
			ga.current = CHAN_STATUS;
			ga.win_chat_refresh = true;

			// usuń kanał, który był przed zmianą na "Status"
			delete ci[i];

			// wyzeruj go w tablicy, w ten sposób wiadomo, że już nie istnieje
			ci[i] = 0;

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}
}


void del_all_chan(struct channel_irc *ci[])
{
/*
	Usuń wszystkie aktywne kanały (zwolnij pamięć przez nie zajmowaną). Funkcja używana przed zakończeniem działania programu.
*/

	for(int i = 0; i < CHAN_MAX; ++i)
	{
		if(ci[i])
		{
			// zamknij log
			if(ci[i]->chan_log.is_open())
			{
				ci[i]->chan_log << LOG_STOPPED;

				ci[i]->chan_log.close();
			}

			delete ci[i];
		}
	}
}


void new_or_update_nick_chan(struct global_args &ga, struct channel_irc *ci[], std::string &channel, std::string &nick, std::string zuo_ip,
	struct nick_flags flags)
{
	// w kluczu trzymaj nick zapisany wielkimi literami (w celu poprawienia sortowania zapewnianego przez std::map)
	std::string nick_key = buf_lower_to_upper(nick);

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// znajdź kanał, którego dotyczy dodanie nicka
		if(ci[i] && ci[i]->channel == channel)
		{
			// wpisz nick
			ci[i]->ni[nick_key].nick = nick;

			// jeśli nie podano ZUO i IP (podano puste), nie nadpisuj aktualnego
			if(zuo_ip.size() > 0)
			{
				ci[i]->ni[nick_key].zuo_ip = zuo_ip;
			}

			// wpisz flagi nicka
			ci[i]->ni[nick_key].nf = flags;

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}
}


void del_nick_chan(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, std::string nick)
{
	std::string nick_key = buf_lower_to_upper(nick);

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// znajdź kanał, którego dotyczy usunięcie nicka
		if(ci[i] && ci[i]->channel == chan_name)
		{
			ci[i]->ni.erase(nick_key);

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}
}
