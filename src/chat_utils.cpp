#include <string>		// std::string

#include "chat_utils.hpp"
#include "enc_str.hpp"
#include "window_utils.hpp"
#include "ucc_global.hpp"


void new_chan_status(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_STATUS] == 0)
	{
		chan_parm[CHAN_STATUS] = new channel_irc;
		chan_parm[CHAN_STATUS]->channel = "Status";
		chan_parm[CHAN_STATUS]->topic = UCC_NAME " " UCC_VER;	// napis wyświetlany na górnym pasku
		chan_parm[CHAN_STATUS]->chan_act = 0;		// zacznij od braku aktywności kanału

		chan_parm[CHAN_STATUS]->win_scroll = -1;	// ciągłe przesuwanie aktualnego tekstu

		// ustaw w nim kursor na pozycji początkowej (to pierwszy tworzony pokój, więc zawsze należy zacząć od pozycji początkowej)
		ga.wcur_y = 0;
		ga.wcur_x = 0;

		// ustaw nowoutworzony kanał jako aktywny
		ga.current = CHAN_STATUS;
	}
}


void new_chan_debug_irc(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_DEBUG_IRC] == 0)
	{
		chan_parm[CHAN_DEBUG_IRC] = new channel_irc;
		chan_parm[CHAN_DEBUG_IRC]->channel = "Debug";
		chan_parm[CHAN_DEBUG_IRC]->topic = "Surowe dane przesyłane między programem a serwerem (tylko IRC).";
		chan_parm[CHAN_DEBUG_IRC]->chan_act = 0;	// zacznij od braku aktywności kanału

		chan_parm[CHAN_DEBUG_IRC]->win_scroll = -1;	// ciągłe przesuwanie aktualnego tekstu
	}
}


void new_chan_raw_unknown(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_RAW_UNKNOWN] == 0)
	{
		chan_parm[CHAN_RAW_UNKNOWN] = new channel_irc;
		chan_parm[CHAN_RAW_UNKNOWN]->channel = "RawUnknown";
		chan_parm[CHAN_RAW_UNKNOWN]->topic = "Nieznane lub niezaimplementowane komunikaty pobrane z serwera.";
		chan_parm[CHAN_RAW_UNKNOWN]->chan_act = 0;	// zacznij od braku aktywności kanału

		chan_parm[CHAN_RAW_UNKNOWN]->win_scroll = -1;	// ciągłe przesuwanie aktualnego tekstu
	}
}


bool new_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, bool active)
{
/*
	Utwórz nowy kanał czata w programie. Poniższa pętla wyszukuje pierwsze wolne miejsce w tablicy i wtedy tworzy kanał.
*/

	// pierwsza pętla wyszukuje, czy kanał o podanej nazwie istnieje, jeśli tak, nie będzie tworzony drugi o takiej samej nazwie
	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// nie twórz dwóch kanałów o takiej samej nazwie
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			// co prawda nie utworzono nowego kanału, ale nie jest to błąd, bo kanał już istnieje, dlatego zakończ z kodem sukcesu
			return true;
		}
	}

	// druga pętla szuka pierwszego wolnego kanału w tablicy pokoi i jeśli są wolne miejsca, to go tworzy
	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		if(chan_parm[i] == 0)
		{
			chan_parm[i] = new channel_irc;
			chan_parm[i]->channel = chan_name;	// nazwa kanału czata
			chan_parm[i]->chan_act = 0;		// zacznij od braku aktywności kanału

			chan_parm[i]->win_scroll = -1;		// ciągłe przesuwanie aktualnego tekstu

			// jeśli trzeba, kanał oznacz jako aktywny (przełącz na to okno), czyli tylko po wpisaniu /join
			if(active)
			{
				ga.current = i;

				// wyczyść okno (by nie było zawartości poprzedniego okna na ekranie)
				wclear(ga.win_chat);

				// ustaw w nim kursor na pozycji początkowej
				ga.wcur_y = 0;
				ga.wcur_x = 0;
			}

			// gdy utworzono kanał, przerwij szukanie wolnego kanału z kodem sukcesu
			return true;
		}
	}

	return false;	// zakończ z błędem, gdy nie znaleziono wolnych miejsc w tablicy
}


void del_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name)
{
/*
	Usuń kanał czata w programie.
*/

	for(int i = 0; i < CHAN_CHAT; ++i)	// pokoje inne, niż pokoje czata nie mogą zostać usunięte (podczas normalnego działania programu)
	{
		// znajdź po nazwie kanału jego numer w tablicy
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			// tymczasowo przełącz na "Status", potem przerobić, aby przechodziło do poprzedniego, który był otwarty
			ga.current = CHAN_STATUS;
			win_buf_refresh(ga, chan_parm);

			// usuń kanał, który był przed zmianą na "Status"
			delete chan_parm[i];

			// wyzeruj go w tablicy, w ten sposób wiadomo, że już nie istnieje
			chan_parm[i] = 0;

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}
}


void del_all_chan(struct channel_irc *chan_parm[])
{
/*
	Usuń wszystkie aktywne kanały (zwolnij pamięć przez nie zajmowaną). Funkcja używana przed zakończeniem działania programu.
*/

	for(int i = 0; i < CHAN_MAX; ++i)
	{
		if(chan_parm[i])
		{
			delete chan_parm[i];
		}
	}
}


void new_or_update_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &channel,
				std::string &nick, std::string zuo_ip, struct nick_flags flags)
{
	// w kluczu trzymaj nick zapisany wielkimi literami (w celu poprawienia sortowania zapewnianego przez std::map)
	std::string nick_key = buf_lower2upper(nick);

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// znajdź kanał, którego dotyczy dodanie nicka
		if(chan_parm[i] && chan_parm[i]->channel == channel)
		{
			// wpisz nick
			chan_parm[i]->nick_parm[nick_key].nick = nick;

			// jeśli nie podano ZUO i IP (podano puste), nie nadpisuj aktualnego
			if(zuo_ip.size() > 0)
			{
				chan_parm[i]->nick_parm[nick_key].zuo_ip = zuo_ip;
			}

			// wpisz flagi nicka
			chan_parm[i]->nick_parm[nick_key].flags = flags;

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}
}


void del_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string nick)
{
	std::string nick_key = buf_lower2upper(nick);

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// znajdź kanał, którego dotyczy usunięcie nicka
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			chan_parm[i]->nick_parm.erase(nick_key);

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}
}


void hist_erase_password(std::string &kbd_buf, std::string &hist_buf, std::string &hist_ignore)
{
	// gdy wpisano nick z hasłem, w historii nie trzymaj hasła
	if(kbd_buf.find("/nick") == 0)	// reaguj tylko na wpisanie polecenia, dlatego 0
	{
		std::string hist_ignore_nick;

		// początkowo wpisz do bufora "/nick"
		hist_ignore_nick = "/nick";

		// tu będzie tymczasowa pozycja nicka za spacją lub spacjami
		int hist_nick = 5;

		// przepisz spację lub spacje (jeśli są)
		for(int i = 5; i < static_cast<int>(kbd_buf.size()); ++i)	// i = 5, bo pomijamy "/nick"
		{
			if(kbd_buf[i] == ' ')
			{
				hist_ignore_nick += " ";
			}

			else
			{
				hist_nick = i;

				break;
			}
		}

		// przepisz nick za spacją (lub spacjami), o ile go wpisano
		if(hist_nick > 5)
		{
			for(int i = hist_nick; i < static_cast<int>(kbd_buf.size()); ++i)
			{
				// pojawienie się spacji oznacza, że dalej jest hasło
				if(kbd_buf[i] == ' ')
				{
					// przepisz jedną spację za nick
					hist_ignore_nick += " ";

					break;
				}

				else
				{
					hist_ignore_nick += kbd_buf[i];
				}
			}
		}

		// jeśli wpisano w ten sam sposób nick (hasło nie jest sprawdzane), pomiń go w historii
		if(hist_ignore != hist_ignore_nick)
		{
			hist_ignore = hist_ignore_nick;
			hist_buf += hist_ignore_nick + "\n";
		}
	}

	// gdy nie wpisano nicka, przepisz cały bufor
	else
	{
		hist_ignore = kbd_buf;
		hist_buf += kbd_buf + "\n";
	}
}


void destroy_my_password(std::string &buf)
{
	for(int i = 0; i < static_cast<int>(buf.size()); ++i)
	{
		buf[i] = rand();
	}
}
