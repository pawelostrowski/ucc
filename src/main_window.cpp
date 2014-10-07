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


#include <string>		// std::string, std::setlocale()
#include <csignal>		// sig_atomic_t, sigaction(), sigfillset()
#include <sys/select.h>		// select()
#include <sys/time.h>		// gettimeofday()
#include <unistd.h>		// close() - socket

// -std=c++11 - errno

#include "main_window.hpp"
#include "window_utils.hpp"
#include "chat_utils.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "auth.hpp"
#include "network.hpp"
#include "ucc_global.hpp"


volatile sig_atomic_t sig_quit;


void catch_signals(int signum)
{
	sig_quit = 1;
}


int main_window(bool _use_colors, bool _debug_irc)
{
	// przechwycenie sygnałów zamykających program
	sig_quit = 0;

	struct sigaction sa;

	sa.sa_handler = catch_signals;
	sigfillset(&(sa.sa_mask));
	sa.sa_flags = 0;

	sigaction(SIGHUP, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	// zapobiega zapętleniu się programu po wpisaniu w terminalu czegoś w stylu 'echo text | ucc'
	if(freopen("/dev/tty", "r", stdin) == NULL)
	{
		return 1;
	}

	// aby polskie znaki w UTF-8 wyświetlały się prawidłowo, przy okazji pl_PL ustawi polskie dni i miesiące przy pobieraniu daty z unixtimestamp
	std::setlocale(LC_ALL, "pl_PL.UTF-8");

	// inicjalizacja ncurses
	if(! initscr())
	{
		return 2;
	}

/*
	Zmienne.
*/
	int term_y, term_x;		// wymiary terminala

	bool win_first = true;		// pierwsze uruchomienie wymaga utworzenia okien "wirtualnych"

	int key_code;			// kod odczytany z bufora wejściowego (przy wciskaniu klawiszy i wklejaniu tekstu ze schowka)
	std::string key_code_tmp;	// bufor tymczasowy do konwersji int -> std::string
	std::string kbd_buf;		// bufor odczytanych znaków z klawiatury
	unsigned int kbd_buf_pos = 0;	// początkowa pozycja bufora klawiatury (istotne podczas używania strzałek, Home, End, Delete itd.)
	int kbd_cur_pos = 0;		// pozycja kursora na pasku wpisywania
	int kbd_cur_pos_offset = 0;	// offset kursora używany, gdy podczas pisania "lewa" część bufora nie jest wyświetlana
	bool kbd_buf_refresh = false;	// gdy była zmiana zawartości bufora klawiatury lub zmiana rozmiaru terminala, należy odświeżyć go na ekranie
	bool paste_tab;

	std::string command_tmp;	// tymczasowy bufor na odczytanie wpisanego polecenia, używany do detekcji wpisania /nick lub /vhost

	unsigned int hist_buf_max_items = HIST_BUF_MAX_ITEMS;	// maksymalna liczba pozycji w buforze historii (później zmienić jako opcja do wyboru)
	std::vector<std::string> hist_buf;	// bufor historii wpisywanych komunikatów i poleceń
	unsigned int hist_buf_index = 0;	// indeks (pozycja) w buforze historii
	bool hist_mod = false;		// czy dokonywano zmian w elementach historii

	int utf_i;			// zmienna pomocnicza używana przy wykrywaniu kodowania znaków w UTF-8

	int topic_len;
	char c;

	bool was_act;
	short int act_color;

	bool win_info_active = false;	// stan włączenia okna informacyjnego (nie mylić z włączeniem przez F2, chodzi o faktyczne wyświetlenie okna)

	struct timeval tv_sel;		// struktura dla select(), aby ustawić czas wychodzenia z oczekiwania na klawiaturę lub socket, by pokazać czas

	struct timeval tv_ping_pong;
	int64_t ping_pong_sec, ping_pong_usec;
	int ping_counter = 0;

	bool was_ucc_quit_time = false;

	struct channel_irc *ci[CHAN_MAX] = {};	// tablica kanałów (początkowo wyzerowana)
/*
	Koniec zmiennych.
*/

/*
	Ustalenie wartości początkowych wybranych globalnych zmiennych.
*/
	struct global_args ga;		// struktura globalnych zmiennych

	ga.use_colors = _use_colors;
	ga.debug_irc = _debug_irc;

	ga.ucc_quit = false;		// aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
	ga.ucc_quit_time = false;

	ga.win_info_state = true;
	ga.win_info_refresh = true;

	ga.socketfd_irc = 0;		// gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne

	ga.captcha_ready = false;	// stan wczytania CAPTCHA (jego pobranie z serwera)
	ga.irc_ready = false;		// gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
	ga.irc_ok = false;		// stan połączenia z czatem

	ga.is_irc_recv_buf_incomplete = false;

	ga.debug_http_f.open(DEBUG_HTTP_FILE, std::ios::out);
	ga.debug_irc_f.open(DEBUG_IRC_FILE, std::ios::out);
/*
	Koniec ustalania globalnych zmiennych.
*/

	// sprawdź, czy terminal obsługuje kolory, jeśli tak, włącz kolory oraz zainicjalizuj podstawową parę kolorów, ale tylko wtedy,
	// gdy uruchomiliśmy main_window() z _use_colors = true, gdy terminal nie obsługuje kolorów, check_colors() zwróci false
	if(ga.use_colors)
	{
		ga.use_colors = check_colors();
	}

	raw();				// zablokuj Ctrl-C i Ctrl-Z
	keypad(stdscr, TRUE);		// klawisze funkcyjne będą obsługiwane
	noecho();			// nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)
	nodelay(stdscr, TRUE);		// nie blokuj getch() (zwróć ERR, gdy nie ma żadnego znaku do odczytania)

	// bez zaktualizowania okna głównego pierwszy zapis do okna "wirtualnego" jest niewidoczny
	wnoutrefresh(stdscr);

	// pobierz wymiary terminala (okna głównego)
	getmaxyx(stdscr, term_y, term_x);

	// utwórz okno, w którym będą wyświetlane wszystkie kanały, privy, "Status" i "DebugIRC" (jeśli włączony jest tryb debugowania IRC)
//	ga.win_chat = newwin(term_y - 3, term_x, 1, 0);

	// nie zostawiaj kursora w oknie "wirtualnym"
//	leaveok(ga.win_chat, TRUE);

	// zapisz wymiary okna "wirtualnego" do wykorzystania później
//	getmaxyx(ga.win_chat, ga.wterm_y, ga.wterm_x);

	// utwórz kanał "Status"
	new_chan_status(ga, ci);

	// jeśli trzeba, utwórz kanał "DebugIRC"
	if(ga.debug_irc)
	{
		new_chan_debug_irc(ga, ci);
	}

	fd_set readfds;			// deskryptor dla select()
	fd_set readfds_tmp;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);		// klawiatura (stdin)

	// wpisz do bufora "Status" komunikat startowy w kolorze zielonym oraz cyjan (kolor będzie wtedy, gdy terminal obsługuje kolory) i go wyświetl
	// (ze względu na przyjętą budowę bufora na końcu tekstu nie ma kodu "\n")
	win_buf_add_str(ga, ci, "Status",
			xGREEN "# Aby zalogować się na nick tymczasowy, wpisz:\n"
			xCYAN  "/nick nazwa_nicka\n"
			xCYAN  "/connect " xGREEN "lub " xCYAN "/c\n"
			xGREEN "# Następnie przepisz kod z obrazka, w tym celu wpisz:\n"
			xCYAN  "/captcha kod_z_obrazka " xGREEN "lub " xCYAN "/cap kod_z_obrazka\n"
			xGREEN "# Aby zalogować się na nick stały (zarejestrowany), wpisz:\n"
			xCYAN  "/nick nazwa_nicka hasło_do_nicka\n"
			xCYAN  "/connect " xGREEN "lub " xCYAN "/c\n"
			xGREEN "# Aby zobaczyć dostępne polecenia, wpisz:\n"
			xCYAN  "/help " xGREEN "lub " xCYAN "/h\n"
			xGREEN "# Aby zakończyć działanie programu, wpisz:\n"
			xCYAN  "/quit " xGREEN "lub " xCYAN "/q");

	// licznik dla select(), aby pokazać aktualny czas oraz obsłużyć PING
	tv_sel.tv_sec = 0;
	tv_sel.tv_usec = 250000;

/*
	Pętla główna programu.
*/
	while(! ga.ucc_quit)
	{
		// w przypadku wykrycia zamknięcia programu przy połączeniu do IRC ustaw 0.5s zwłoki, zanim serwer nie da odpowiedzi, jeśli serwer nie
		// odpowie po tym czasie, zakończ (lub zakończ wcześniej, gdy serwer odpowie)
		if(ga.ucc_quit_time && ! was_ucc_quit_time)
		{
			tv_sel.tv_sec = 0;
			tv_sel.tv_usec = 500000;

			// nie aktualizuj ponownie licznika
			was_ucc_quit_time = true;
		}

		// wykrycie któregoś sygnału zamykającego program
		if(sig_quit)
		{
			// jeśli było połączenie z IRC, wyślij do serwera komunikat o rozłączeniu (tylko raz) i poczekaj maksymalnie 0.5s na odpowiedź
			if(ga.irc_ok && ! ga.ucc_quit_time)
			{
				irc_send(ga, ci, "QUIT");

				ga.ucc_quit_time = true;
			}

			// gdy nie było połączenia z IRC, można od razu przerwać pętlę główną
			else if(! ga.irc_ok)
			{
				break;
			}
		}

		// gdy nie było sygnału zamykającego program
		else
		{
			// wykryj zmianę rozmiaru okna terminala
			if(is_term_resized(term_y, term_x) && ! win_first)
			{
				getmaxyx(stdscr, term_y, term_x);		// pobierz nowe wymiary terminala (okna głównego) po jego zmianie
				wresize(stdscr, term_y, term_x);		// zmień rozmiar okna głównego po zmianie rozmiaru okna terminala

				// ustaw nowe wymiary okien "wirtualnych" zależnie od stanu włączenia okna informacyjnego
				if(win_info_active)
				{
					wresize(ga.win_chat, term_y - 3, term_x - NICKLIST_WIDTH);

					delwin(ga.win_info);

					ga.win_info = newwin(term_y - 3, NICKLIST_WIDTH, 1, term_x - NICKLIST_WIDTH);

					leaveok(ga.win_info, TRUE);

					ga.win_info_refresh = true;
				}

				else
				{
					wresize(ga.win_chat, term_y - 3, term_x);
				}

				getmaxyx(ga.win_chat, ga.wterm_y, ga.wterm_x);	// zapisz wymiary okna "wirtualnego"

				kbd_buf_refresh = true;		// odśwież na ekranie zawartość bufora klawiatury
				ga.win_chat_refresh = true;	// odśwież na ekranie zawartość okna "wirtualnego"

				// aby przy zmianie rozmiaru terminala w oknie "wirtualnym" (tak, to nie pomyłka) nie było śmieci, należy wyczyścić okno
				// główne
				erase();

				// brak zaktualizowania okna głównego powoduje, że po zmianie rozmiaru terminala znika zawartość okna "wirtualnego"
				wnoutrefresh(stdscr);
			}

			// jeśli terminal obsługuje kolory (lub nie zostały wyłączone), paski będą w niebieskie z białym fontem, jeśli nie, paski będą w
			// kolorze odwróconym od domyślnych
			ga.use_colors ? attrset(COLOR_PAIR(pWHITE_BLUE)) : attrset(A_REVERSE);

/*
	Informacje na pasku górnym.
*/
			move(0, 0);

			// wyświetl temat pokoju (tyle znaków, ile zmieści się na szerokość górnego paska)
			topic_len = ci[ga.current]->topic.size();
			utf_i = 0;

			for(int i = 0; i < topic_len && i < term_x + utf_i; ++i)
			{
				c = ci[ga.current]->topic[i];

				// uwzględnij kodowanie UTF-8 (wtedy trzeba dodać do szerokości nadmiarowe cykle, aby cały znak wyświetlił się)
				if((c & 0xE0) == 0xC0)
				{
					utf_i += 1;
				}

				else if((c & 0xF0) == 0xE0)
				{
					utf_i += 2;
				}

				else if((c & 0xF8) == 0xF0)
				{
					utf_i += 3;
				}

				else if((c & 0xFC) == 0xF8)
				{
					utf_i += 4;
				}

				else if((c & 0xFE) == 0xFC)
				{
					utf_i += 5;
				}

				printw("%c", c);
			}

			// pozostałą część paska wypełnij kolorem (uwzględnij, czy kursor jest jeszcze na pasku, aby nie tworzyć paska poniżej)
			for(int i = getcurx(stdscr); i < term_x && getcury(stdscr) == 0; ++i)
			{
				addch(' ');
			}
/*
	Koniec informacji na pasku górnym.
*/

/*
	Informacje na pasku dolnym.
*/
			// pokaż aktualny czas
			mvaddstr(term_y - 2, 0, get_time().c_str());

			// pokaż nick zależny od stanu zalogowania do IRC
			addch('[');

			if(! ga.irc_ok)
			{
				addstr(NICK_NOT_LOGGED);
			}

			else
			{
				attron(A_BOLD);
				addstr(ga.zuousername.c_str());
				attroff(A_BOLD);

				// jeśli away lub busy jest włączony, pokaż to za nickiem
				if(ga.my_away || ga.my_busy)
				{
					addstr(": ");
					attron(A_BOLD);

					if(ga.my_away)
					{
						addstr("away");
					}

					if(ga.my_busy)
					{
						// jeśli był away, pokaż przecinek za nim
						if(ga.my_away)
						{
							attroff(A_BOLD);
							addstr(", ");
							attron(A_BOLD);
						}

						addstr("busy");
					}

					attroff(A_BOLD);
				}
			}

			addch(']');

			// skasuj flagi aktywności w aktualnie otwartym pokoju
			ci[ga.current]->chan_act = 0;

			// wyświetl aktywność pokoi
			was_act = false;

			addstr(" [Pokoje: ");

			for(int i = 0; i < CHAN_MAX; ++i)
			{
				// wyświetl aktywność danego pokoju o ile istnieje
				if(ci[i])
				{
					// jeśli był już wyświetlony pokój, dopisz po nim przecinek na liście
					if(was_act)
					{
						addch(',');
					}

					was_act = true;

					// wykryj aktywność typu 3
					if(ci[i]->chan_act == 3)
					{
						attron(A_BOLD);		// włącz bold dla tej aktywności
						act_color = pMAGENTA_BLUE;

						// przy braku kolorów wykrycie tej aktywności spowoduje miganie numeru pokoju oraz jego podkreślenie
						if(! ga.use_colors)
						{
							attron(A_BLINK | A_UNDERLINE);
						}
					}

					// wykryj aktywność typu 2
					else if(ci[i]->chan_act == 2)
					{
						attron(A_BOLD);		// włącz bold dla tej aktywności
						act_color = pWHITE_BLUE;

						// przy braku kolorów wykrycie tej aktywności spowoduje podkreślenie numeru pokoju
						if(! ga.use_colors)
						{
							attron(A_UNDERLINE);
						}
					}

					// wykryj aktywność typu 1
					else if(ci[i]->chan_act == 1)
					{
						act_color = pCYAN_BLUE;

						// przy braku kolorów wykrycie tej aktywności spowoduje podkreślenie numeru pokoju
						if(! ga.use_colors)
						{
							attron(A_UNDERLINE);
						}
					}

					// brak aktywności
					else
						{
						// numer aktualnie otwartego pokoju wyświetl na białym tle (przy włączonych kolorach) lub kontrastowym tle
						// (przy braku kolorów)
						if(i == ga.current)
						{
							ga.use_colors ? attron(A_REVERSE) : attroff(A_REVERSE);
							act_color = pWHITE_BLUE;
						}

						else
						{
							act_color = pBLACK_BLUE;
						}
					}

					// ustaw kolor dla danego pokoju (jeśli kolory są obsługiwane)
					if(ga.use_colors)
					{
						attron(COLOR_PAIR(act_color));
					}

					// pokój czata jako cyfra
					if(i < CHAN_CHAT)
					{
						printw("%d", i + 1);	// + 1, bo pokoje na pasku są od 1 a nie od 0 (jak w tablicy pokoi)
					}

					// okno "Status" jako literka "s"
					else if(i == CHAN_STATUS)
					{
						addch('s');
					}

					// okno "DebugIRC" jako literka "d"
					else if(i == CHAN_DEBUG_IRC)
					{
						addch('d');
					}

					// okno "RawUnknown" jako literka "x" (bez else if, bo to i tak ostatni kanał na liście)
					else
					{
						addch('x');
					}

					// przywróć domyślne atrybuty paska
					ga.use_colors ? attrset(COLOR_PAIR(pWHITE_BLUE)) : attrset(A_REVERSE);
				}
			}

			// nawias zamykający
			addch(']');

			// wyświetl nazwę pokoju
			addstr(" [");
			attron(A_BOLD);
			addstr(ci[ga.current]->channel.c_str());
			attroff(A_BOLD);
			addch(']');

			// wyświetl lag (tylko przy połączeniu z IRC i gdy już obliczono lag)
			if(ga.irc_ok && ga.lag > 0)
			{
				addstr(" [Lag: ");

				// lag poniżej sekundy (w ms)
				if(ga.lag < 1000)
				{
					printw("%lldm", ga.lag);
				}

				// lag powyżej sekundy (w s)
				else
				{
					printw("%.2f", ga.lag / 1000.00);
				}

				addstr("s]");
			}

/*
	TEST START - usunąć po testach
*/
//			printw(" [%d, %d, %d]", ci[ga.current]->win_scroll_lock, ci[ga.current]->win_scroll_first, ci[ga.current]->win_scroll_last);

//			printw(" sock: %d", ga.socketfd_irc);

//			printw(" win_info -> on: %d, active: %d", ga.win_info_state, win_info_active);
/*
	TEST END
*/

			// jeśli pasek dolny "wszedł" na pasek pisania, odśwież zawartość bufora klawiatury
			if(getcury(stdscr) == term_y - 1 && getcurx(stdscr) > 0)
			{
				kbd_buf_refresh = true;
			}

			// pozostałą część paska wypełnij kolorem (uwzględnij, czy kursor jest jeszcze na pasku)
			for(int i = getcurx(stdscr); i < term_x && getcury(stdscr) == term_y - 2; ++i)
			{
				addch(' ');
			}
/*
	Koniec informacji na pasku dolnym.
*/

			// sprawdź gotowość do połączenia z IRC
			if(ga.irc_ready)
			{
				// zaloguj się do czata
				auth_irc_all(ga, ci);

				// jeśli udało się połączyć, dodaj socketfd_irc do zestawu select()
				if(ga.irc_ok)
				{
					FD_SET(ga.socketfd_irc, &readfds);

					// wyzeruj lag
					ping_counter = 0;
					ga.lag = 0;
					ga.lag_timeout = false;
				}

				// w przeciwnym razie wyzeruj socket
				else
				{
					if(ga.socketfd_irc > 0)
					{
						close(ga.socketfd_irc);
						ga.socketfd_irc = 0;
					}

					// pokaż komunikat o rozłączeniu we wszystkich otwartych kanałach, z wyjątkiem "DebugIRC" oraz "RawUnknown"
					msg_err_disconnect(ga, ci);
				}
			}

			// jeśli włączono okno informacyjne, a było wyłączone i aktualnie otwarte okno jest oknem czata, wyświetl to okno
			if(ga.win_info_state && ! win_info_active && ga.current < CHAN_NORMAL)
			{
				win_info_active = true;

				ga.win_chat_refresh = true;

				if(win_first)
				{
					// utwórz okno, w którym będą wyświetlane wszystkie kanały, privy, "Status" i "DebugIRC" (jeśli włączony jest tryb
					// debugowania IRC)
					ga.win_chat = newwin(term_y - 3, term_x - NICKLIST_WIDTH, 1, 0);

					// nie zostawiaj kursora w oknie "wirtualnym"
					leaveok(ga.win_chat, TRUE);

					win_first = false;
				}

				else
				{
					wresize(ga.win_chat, term_y - 3, term_x - NICKLIST_WIDTH);
				}

				// zapisz wymiary okna "wirtualnego"
				getmaxyx(ga.win_chat, ga.wterm_y, ga.wterm_x);

				// utwórz okno informacyjne
				ga.win_info = newwin(term_y - 3, NICKLIST_WIDTH, 1, term_x - NICKLIST_WIDTH);

				// nie zostawiaj kursora w oknie informacyjnym
				leaveok(ga.win_info, TRUE);
			}

			// jeśli wyłączono okno informacyjne, a było włączone lub jest włączone, ale aktualnie nie jest otwarte okno czata, wyłącz to okno
			else if((! ga.win_info_state && win_info_active) || (ga.win_info_state && win_info_active && ga.current >= CHAN_NORMAL))
			{
				win_info_active = false;

				ga.win_chat_refresh = true;

				if(win_first)
				{
					// utwórz okno, w którym będą wyświetlane wszystkie kanały, privy, "Status" i "DebugIRC" (jeśli włączony jest tryb
					// debugowania IRC)
					ga.win_chat = newwin(term_y - 3, term_x, 1, 0);

					// nie zostawiaj kursora w oknie "wirtualnym"
					leaveok(ga.win_chat, TRUE);

					win_first = false;
				}

				else
				{
					wresize(ga.win_chat, term_y - 3, term_x);
				}

				// zapisz wymiary okna "wirtualnego"
				getmaxyx(ga.win_chat, ga.wterm_y, ga.wterm_x);

				// usuń okno informacyjne
				delwin(ga.win_info);
			}

			// jeśli trzeba, odśwież na ekranie zawartość bufora klawiatury
			if(kbd_buf_refresh)
			{
				kbd_buf_show(kbd_buf, term_y, term_x, kbd_cur_pos, kbd_cur_pos_offset);
				kbd_buf_refresh = false;
			}

			// jeśli trzeba, odśwież na ekranie zawartość okna "wirtualnego"
			if(ga.win_chat_refresh)
			{
				win_buf_show(ga, ci);
			}

			// jeśli trzeba, odśwież okno informacyjne
			if(ga.win_info_refresh && win_info_active)
			{
				if(win_info_active)
				{
					nicklist_refresh(ga, ci);
				}

				// nie odświeżaj okna informacyjnego przy ponownym obiegu pętli
				ga.win_info_refresh = false;
			}

			// obecna pozycja kursora
			move(term_y - 1, kbd_cur_pos - kbd_cur_pos_offset);

			// zaktualizuj okno główne oraz pokaż zmiany na ekranie dla wszystkich okien
			wnoutrefresh(stdscr);
			doupdate();
		}

/*
	Obsługa select() - czekaj na aktywność klawiatury lub gniazda.
*/
		readfds_tmp = readfds;

		if(select(ga.socketfd_irc + 1, &readfds_tmp, NULL, NULL, &tv_sel) == -1)
		{
			// sygnał SIGWINCH (zmiana rozmiaru okna terminala) powoduje, że select() zwraca -1, trzeba to wykryć, aby nie wywalić programu
			if(errno == EINTR)	// Interrupted system call (wywołany np. przez SIGWINCH)
			{
				getch();	// ignoruj KEY_RESIZE
				continue;	// wróć do początku pętli głównej while()
			}

			// inny błąd select() powoduje zakończenie działania programu
			else
			{
				if(ga.socketfd_irc > 0)
				{
					close(ga.socketfd_irc);
				}

				del_all_chan(ci);

				ga.debug_irc_f.close();
				ga.debug_http_f.close();

				if(win_info_active)
				{
					delwin(ga.win_info);
				}

				delwin(ga.win_chat);
				endwin();	// zakończ tryb ncurses

				fclose(stdin);

				return 3;
			}
		}
/*
	Koniec obsługi select(), poniżej będzie obsługa przerwania timeout, klawiatury oraz gniazda.
*/

/*
	Obsługa timeout.
*/
		// co 0.25s aktualizuj licznik (trzeba pamiętać, że timeout to nie jedyny sposób wyjścia z select(), tak samo klawiatura i gniazdo przerywają)
		if(tv_sel.tv_sec == 0 && tv_sel.tv_usec == 0)
		{
			tv_sel.tv_sec = 0;
			tv_sel.tv_usec = 250000;

			// jeśli wpisano /quit przy połączeniu z IRC i nie otrzymano odpowiedzi serwera przez określony czas, zakończ program
			// (wtedy powyższa aktualizacja licznika nie będzie miała znaczenia, bo nastąpi przerwanie pętli głównej programu)
			if(ga.ucc_quit_time)
			{
				ga.ucc_quit = true;
			}

			// obsługa PING (tylko przy połączeniu z IRC oraz gdy nie jest to zamykanie programu)
			if(ga.irc_ok && ! ga.ucc_quit_time)
			{
				++ping_counter;

				// gdy serwer nie dał odpowiedzi PONG, dolicz nowy lag (gdy odpowie, lag_timeout nie jest ustawiony)
				if(ga.lag_timeout)
				{
					gettimeofday(&tv_ping_pong, NULL);
					ping_pong_sec = tv_ping_pong.tv_sec;
					ping_pong_usec = tv_ping_pong.tv_usec;
					ga.pong = (ping_pong_sec * 1000) + (ping_pong_usec / 1000);

					// daj 5s, zanim lag zacznie szybko narastać (czas w ms, dlatego 5000)
					if(ga.pong - ga.ping >= 5000)
					{
						ga.lag = ga.pong - ga.ping;

						// w przypadku narastania laga, ustaw chwilę oczekiwania, aby po ustabilizowaniu się dać czas na odczytanie
						ping_counter = PING_TIME * 2;
					}

					// jeśli serwer nie odpowie przez wystarczająco długi czas, wyzeruj socket (czyli brak połączenia z IRC)
					if(ga.lag >= PING_TIMEOUT * 1000)
					{
						// usuń socketfd_irc z zestawu select()
						FD_CLR(ga.socketfd_irc, &readfds);

						// zamknij oraz wyzeruj socket
						close(ga.socketfd_irc);
						ga.socketfd_irc = 0;

						// usuń wszystkie nicki ze wszystkich otwartych pokoi z listy oraz wyświetl komunikat we wszystkich otwartych
						// pokojach (poza "DebugIRC" i "RawUnknown")
						for(int i = 0; i < CHAN_NORMAL; ++i)
						{
							if(ci[i])
							{
								ci[i]->ni.clear();

								win_buf_add_str(ga, ci, ci[i]->channel,
										xBOLD_ON xRED "# Serwer nie odpowiadał przez ponad "
										+ std::to_string(PING_TIMEOUT) + "s, rozłączono.");

								// odśwież listę w aktualnie otwartym pokoju (o ile włączone jest okno informacyjne oraz
								// zmiana dotyczyła nicka, który też jest w tym pokoju)
								if(ga.win_info_state && ga.current == i)
								{
									ga.win_info_refresh = true;
								}
							}
						}

						ga.irc_ok = false;
					}
				}

				if(ping_counter == PING_TIME * 4)
				{
					ping_counter = 0;

					// wyślij PING, o ile nie doszło to PING timeout, czyli rozłączenia z IRC oraz serwer odpowiedział PONG
					if(ga.irc_ok && ! ga.lag_timeout)
					{
						gettimeofday(&tv_ping_pong, NULL);
						ping_pong_sec = tv_ping_pong.tv_sec;
						ping_pong_usec = tv_ping_pong.tv_usec;
						ga.ping = (ping_pong_sec * 1000) + (ping_pong_usec / 1000);

						irc_send(ga, ci, "PING :" + std::to_string(ga.ping));
					}

					// załóż brak PONG, zmienione zostanie, jeśli serwer odpowie PONG
					ga.lag_timeout = true;
				}
			}
		}
/*
	Koniec obsługi timeout.
*/

/*
	Obsługa klawiatury.
*/
		if(FD_ISSET(0, &readfds_tmp))
		{
			// wstępnie przyjmij, że nie wklejono kodu tabulatora (i czegoś za nim)
			paste_tab = false;

			// pobierz kod wciśniętego klawisza
			key_code = getch();

			// Tab
			if(key_code == '\t')
			{
				// pobierz kolejny kod wciśniętego klawisza, aby sprawdzić, czy wciśnięto Tab, czy wklejono jego kod ze schowka (jeśli
				// to nie jedyny kod i dalej coś jeszcze jest w buforze schowka, natomiast jeśli to jedyny kod, mimo wklejenia ze schowka
				// będzie obsługiwany jak naciśnięcie tabulatora)
				key_code = getch();

				// jeśli wciśnięto Tab, wykonaj obsługę dopełniania nicków tabulatorem
				if(key_code == ERR)
				{
					// tu dopisać dopełnianie nicków tabem
				}

				// jeśli tekst wklejono ze schowka, poinformuj, żeby wkleić kod tabulatora
				else
				{
					paste_tab = true;
				}
			}

			// kody ASCII (oraz rozszerzone) wczytaj do bufora (te z zakresu 32...255), jednocześnie ogranicz pojemność bufora wejściowego,
			// dopuść też kod tabulatora (pojawia się w tym miejscu, gdy do bufora coś wklejono ze schowka, a nie wciśnięto Tab);
			// if, aby wklejenie tekstu z tabulatorem umożliwiło dalsze wczytanie schowka
			if(((key_code >= 32 && key_code <= 255) || key_code == '\t') && buf_chars(kbd_buf) < KBD_BUF_MAX_CHARS)
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				// jeśli wklejono kod tabulatora i coś za nim było, nie strać tego kodu
				if(paste_tab)
				{
					kbd_buf.insert(kbd_buf_pos, "\t");

					++kbd_buf_pos;
					++kbd_cur_pos;
				}

				// jeśli modyfikowano element historii, ustaw odpowiednią flagę informującą o tym
				if(hist_buf_max_items > 0 && hist_buf_index < hist_buf.size())
				{
					hist_mod = true;
				}

				int kbd_buf_chars = buf_chars(kbd_buf);
				int line_count = 0;

				while(key_code != ERR)
				{
					// jeśli nie koniec bufora, ale inne dopuszczone znaki (ASCII i \t), dopuść je do wczytania do bufora
					if(((key_code >= 32 && key_code <= 255) || key_code == '\t') && kbd_buf_chars < KBD_BUF_MAX_CHARS)
					{
						key_code_tmp = key_code;
						kbd_buf.insert(kbd_buf_pos, key_code_tmp);

						++kbd_buf_pos;
						++kbd_cur_pos;

						++kbd_buf_chars;

						// uwzględnij kodowanie UTF-8
						utf_i = 0;

						if((key_code & 0xE0) == 0xC0)
						{
							utf_i = 1;
						}

						else if((key_code & 0xF0) == 0xE0)
						{
							utf_i = 2;
						}

						else if((key_code & 0xF8) == 0xF0)
						{
							utf_i = 3;
						}

						else if((key_code & 0xFC) == 0xF8)
						{
							utf_i = 4;
						}

						else if((key_code & 0xFE) == 0xFC)
						{
							utf_i = 5;
						}

						// jeśli utf_i > 0, pobierz brakujące bajty znaku w UTF-8
						for(int i = 0; i < utf_i; ++i)
						{
							key_code_tmp = getch();
							kbd_buf.insert(kbd_buf_pos, key_code_tmp);

							++kbd_buf_pos;
						}
					}

					// jeśli we wklejonym z bufora tekście był kod \n lub przekroczono pojemność bufora, uruchom parser klawiatury,
					// (wszystkie inne znaki ignoruj)
					else if((key_code >= 32 && key_code <= 255) || key_code == '\t' || key_code == '\n')
					{
						// w przypadku wklejania ze schowka pomijaj puste wiersze (te, gdzie był sam kod \n, który nie jest wklejany)
						if(kbd_buf.size() > 0)
						{
							// jeśli historia wpisywanych tekstów i poleceń jest obsługiwana, wpisz do niej obecną zawartość
							if(hist_buf_max_items > 0)
							{
								// jeśli na początku wpisano "/", pobierz wpisane polecenie
								command_tmp = (kbd_buf[0] == '/' ? buf_lower_to_upper(get_raw_parm(kbd_buf, 0)) : "");

								// nie zapisuj w historii poleceń /nick oraz /vhost, aby nie było w niej haseł
								if(command_tmp != "/NICK" && command_tmp != "/VHOST")
								{
									// jeśli to pierwszy wpis do bufora historii, wpisz go zawsze
									if(hist_buf.size() == 0)
									{
										// wpisz do bufora historii wpisany tekst
										hist_buf.push_back(kbd_buf);
									}

									// podczas obsługi bufora historii pomijaj wpisanie tego samego co ostatnio
									else if(hist_buf[hist_buf.size() - 1] != kbd_buf)
									{
										// wpisz do bufora historii wpisany tekst
										hist_buf.push_back(kbd_buf);

										// ogranicz maksymalną liczbę pozycji w buforze historii
										if(hist_buf.size() > hist_buf_max_items)
										{
											// usuń pierwszy element historii (nie trzeba liczyć nadmiaru,
											// bo wszędzie pilnowane jest, aby zawsze usuwać nadmiar na czas)
											hist_buf.erase(hist_buf.begin());
										}
									}
								}
							}

							// wykonaj obsługę bufora klawiatury (zidentyfikuj polecenie i wykonaj odpowiednie działanie)
							kbd_parser(ga, ci, kbd_buf);

							// po obsłudze bufora klawiatury wyczyść go
							kbd_buf.clear();
							kbd_buf_pos = 0;
							kbd_cur_pos = 0;
						}

						// ustaw wskaźnik bufora historii na koniec
						hist_buf_index = hist_buf.size();

						// wyzeruj flagę dokonanych zmian w elemencie bufora historii
						hist_mod = false;

						// jeśli podczas wklejania ze schowka doszło do limitu bufora, nie strać ostatniego znaku
						if(key_code != '\n')
						{
							ungetch(key_code);
						}

						kbd_buf_chars = 0;
						++line_count;

						// zabezpieczenie przed wklejeniem gigantycznego tekstu do 100 wierszy
						if(line_count > 100)
						{
							flushinp();
						}
					}

					// pobierz kolejny kod, jeśli to ERR (nic nie ma w buforze), pętla zostanie zakończona
					key_code = getch();
				}
			}

			// Enter (0x0A)
			else if(key_code == '\n')
			{
				// wykonaj obsługę bufora klawiatury, ale tylko wtedy, gdy coś w nim jest
				if(kbd_buf.size() > 0)
				{
					// jeśli historia wpisywanych tekstów i poleceń jest obsługiwana, wpisz do niej obecną zawartość;
					if(hist_buf_max_items > 0)
					{
						// jeśli na początku wpisano "/", pobierz wpisane polecenie
						command_tmp = (kbd_buf[0] == '/' ? buf_lower_to_upper(get_raw_parm(kbd_buf, 0)) : "");

						// nie zapisuj w historii poleceń /nick oraz /vhost, aby nie było w niej haseł
						if(command_tmp != "/NICK" && command_tmp != "/VHOST")
						{
							// jeśli to pierwszy wpis do bufora historii, wpisz go zawsze
							if(hist_buf.size() == 0)
							{
								// wpisz do bufora historii wpisany tekst
								hist_buf.push_back(kbd_buf);
							}

							// podczas obsługi bufora historii pomijaj wpisanie tego samego co ostatnio
							else if(hist_buf[hist_buf.size() - 1] != kbd_buf)
							{
								// wpisz do bufora historii wpisany tekst
								hist_buf.push_back(kbd_buf);

								// ogranicz maksymalną liczbę pozycji w buforze historii
								if(hist_buf.size() > hist_buf_max_items)
								{
									// usuń pierwszy element historii (nie trzeba liczyć nadmiaru,
									// bo wszędzie pilnowane jest, aby zawsze usuwać nadmiar na czas)
									hist_buf.erase(hist_buf.begin());
								}
							}
						}
					}

					// przenieś kursor na początek paska wpisywania tekstu oraz wyczyść pasek
					move(term_y - 1, 0);
					clrtoeol();

					// wykonaj obsługę bufora klawiatury (zidentyfikuj polecenie i wykonaj odpowiednie działanie)
					kbd_parser(ga, ci, kbd_buf);

					// po obsłudze bufora klawiatury wyczyść go
					kbd_buf.clear();
					kbd_buf_pos = 0;
					kbd_cur_pos = 0;
				}

				// ustaw wskaźnik bufora historii na koniec
				hist_buf_index = hist_buf.size();

				// wyzeruj flagę dokonanych zmian w elemencie bufora historii
				hist_mod = false;
			}

			// Left Arrow
			else if(key_code == KEY_LEFT && kbd_buf_pos > 0)
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				--kbd_buf_pos;
				--kbd_cur_pos;

				// uwzględnij kodowanie UTF-8
				if(kbd_buf_pos >= 1 && (kbd_buf[kbd_buf_pos - 1] & 0xE0) == 0xC0)
				{
					kbd_buf_pos -= 1;
				}

				else if(kbd_buf_pos >= 2 && (kbd_buf[kbd_buf_pos - 2] & 0xF0) == 0xE0)
				{
					kbd_buf_pos -= 2;
				}

				else if(kbd_buf_pos >= 3 && (kbd_buf[kbd_buf_pos - 3] & 0xF8) == 0xF0)
				{
					kbd_buf_pos -= 3;
				}

				else if(kbd_buf_pos >= 4 && (kbd_buf[kbd_buf_pos - 4] & 0xFC) == 0xF8)
				{
					kbd_buf_pos -= 4;
				}

				else if(kbd_buf_pos >= 5 && (kbd_buf[kbd_buf_pos - 5] & 0xFE) == 0xFC)
				{
					kbd_buf_pos -= 5;
				}
			}

			// Right Arrow
			else if(key_code == KEY_RIGHT && kbd_buf_pos < kbd_buf.size())
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				// uwzględnij kodowanie UTF-8
				if(kbd_buf_pos + 1 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xE0) == 0xC0)
				{
					kbd_buf_pos += 1;
				}

				else if(kbd_buf_pos + 2 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xF0) == 0xE0)
				{
					kbd_buf_pos += 2;
				}

				else if(kbd_buf_pos + 3 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xF8) == 0xF0)
				{
					kbd_buf_pos += 3;
				}

				else if(kbd_buf_pos + 4 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xFC) == 0xF8)
				{
					kbd_buf_pos += 4;
				}

				else if(kbd_buf_pos + 5 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xFE) == 0xFC)
				{
					kbd_buf_pos += 5;
				}

				++kbd_buf_pos;
				++kbd_cur_pos;
			}

			// Up Arrow
			else if(key_code == KEY_UP && hist_buf_max_items > 0 && hist_buf.size() > 0 && hist_buf_index > 0)
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				// jeśli to nie jest element historii, tylko obecnie wpisywany tekst i wciśnięto strzałkę w górę, wpisz go do historii,
				// jednocześnie podczas obsługi bufora historii pomijaj wpisanie tego samego co ostatnio
				if(hist_buf_index == hist_buf.size() && kbd_buf.size() > 0 && hist_buf[hist_buf.size() - 1] != kbd_buf)
				{
					// jeśli na początku wpisano "/", pobierz wpisane polecenie
					command_tmp = (kbd_buf[0] == '/' ? buf_lower_to_upper(get_raw_parm(kbd_buf, 0)) : "");

					// nie zapisuj w historii poleceń /nick oraz /vhost, aby nie było w niej haseł
					if(command_tmp != "/NICK" && command_tmp != "/VHOST")
					{
						// wpisz do bufora historii wpisany tekst
						hist_buf.push_back(kbd_buf);

						// ogranicz maksymalną liczbę pozycji w buforze historii
						if(hist_buf.size() > hist_buf_max_items)
						{
							// usuń pierwszy element historii (nie trzeba liczyć nadmiaru, bo wszędzie pilnowane
							// jest, aby zawsze usuwać nadmiar na czas)
							hist_buf.erase(hist_buf.begin());

							// zachowaj równowagę bufora historii
							--hist_buf_index;
						}
					}
				}

				// jeśli modyfikowano ostatni element historii poprzez dopisanie lub usunięcie czegoś, zmodyfikuj go w historii
				if(hist_mod)
				{
					// do modyfikowanego elementu historii wpisz zmodyfikowany bufor klawiatury
					hist_buf[hist_buf_index] = kbd_buf;

					// wyzeruj flagę dokonanych zmian w elemencie bufora historii
					hist_mod = false;
				}

				// zmniejsz wskaźnik bufora historii (jeśli ustawiono limit na jedną pozycję, co tu daje już wartość zero, nie dopuść
				// do dalszego zmniejszania)
				if(hist_buf_index > 0)
				{
					--hist_buf_index;
				}

				// wpisz zawartość bufora historii do bufora klawiatury
				kbd_buf = hist_buf[hist_buf_index];

				// wskaźnik i kursor na koniec
				kbd_buf_pos = kbd_buf.size();
				kbd_cur_pos = buf_chars(kbd_buf);
			}

			// Down Arrow
			else if(key_code == KEY_DOWN && hist_buf_max_items > 0 && hist_buf_index <= hist_buf.size())
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				// jeśli to nie jest element historii, tylko obecnie wpisywany tekst i wciśnięto strzałkę w dół, wpisz go do historii
				if(hist_buf_index == hist_buf.size() && kbd_buf.size() > 0)
				{
					// jeśli na początku wpisano "/", pobierz wpisane polecenie
					command_tmp = (kbd_buf[0] == '/' ? buf_lower_to_upper(get_raw_parm(kbd_buf, 0)) : "");

					// nie zapisuj w historii poleceń /nick oraz /vhost, aby nie było w niej haseł
					if(command_tmp != "/NICK" && command_tmp != "/VHOST")
					{
						// jeśli to pierwszy wpis do bufora historii, wpisz go zawsze
						if(hist_buf.size() == 0)
						{
							// wpisz do bufora historii wpisany tekst
							hist_buf.push_back(kbd_buf);
						}

						// podczas obsługi bufora historii pomijaj wpisanie tego samego co ostatnio
						else if(hist_buf[hist_buf.size() - 1] != kbd_buf)
						{
							// wpisz do bufora historii wpisany tekst
							hist_buf.push_back(kbd_buf);

							// ogranicz maksymalną liczbę pozycji w buforze historii
							if(hist_buf.size() > hist_buf_max_items)
							{
								// usuń pierwszy element historii (nie trzeba liczyć nadmiaru, bo wszędzie pilnowane
								// jest, aby zawsze usuwać nadmiar na czas)
								hist_buf.erase(hist_buf.begin());
							}
						}
					}

					// ustaw wskaźnik bufora historii na koniec
					hist_buf_index = hist_buf.size();

					// wyczyść bufor klawiatury
					kbd_buf.clear();
					kbd_buf_pos = 0;
					kbd_cur_pos = 0;
				}

				// jeśli to element z bufora historii
				else if(hist_buf_index < hist_buf.size())
				{
					// jeśli modyfikowano ostatni element historii poprzez dopisanie lub usunięcie czegoś, zmodyfikuj go w historii
					if(hist_mod)
					{
						// do modyfikowanego elementu historii wpisz zmodyfikowany bufor klawiatury
						hist_buf[hist_buf_index] = kbd_buf;

						// wyzeruj flagę dokonanych zmian w elemencie bufora historii
						hist_mod = false;
					}

					// zwiększ wskaźnik bufora historii
					++hist_buf_index;

					// jeśli nie koniec bufora historii, przepisz pozycję do bufora klawiatury
					if(hist_buf_index < hist_buf.size())
					{
						// wpisz zawartość bufora historii do bufora klawiatury
						kbd_buf = hist_buf[hist_buf_index];

						// wskaźnik i kursor na koniec
						kbd_buf_pos = kbd_buf.size();
						kbd_cur_pos = buf_chars(kbd_buf);
					}

					// jeśli to ostatni element historii, wyczyść bufor klawiatury
					else
					{
						kbd_buf.clear();
						kbd_buf_pos = 0;
						kbd_cur_pos = 0;
					}
				}
			}

			// Backspace
			else if(key_code == KEY_BACKSPACE && kbd_buf_pos > 0)
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				// jeśli modyfikowano element historii, ustaw odpowiednią flagę informującą o tym
				if(hist_buf_max_items > 0 && hist_buf_index < hist_buf.size())
				{
					hist_mod = true;
				}

				--kbd_buf_pos;
				--kbd_cur_pos;

				// uwzględnij kodowanie UTF-8
				utf_i = 0;

				if(kbd_buf_pos >= 1 && (kbd_buf[kbd_buf_pos - 1] & 0xE0) == 0xC0)
				{
					utf_i = 1;
					kbd_buf_pos -= 1;
				}

				else if(kbd_buf_pos >= 2 && (kbd_buf[kbd_buf_pos - 2] & 0xF0) == 0xE0)
				{
					utf_i = 2;
					kbd_buf_pos -= 2;
				}

				else if(kbd_buf_pos >= 3 && (kbd_buf[kbd_buf_pos - 3] & 0xF8) == 0xF0)
				{
					utf_i = 3;
					kbd_buf_pos -= 3;
				}

				else if(kbd_buf_pos >= 4 && (kbd_buf[kbd_buf_pos - 4] & 0xFC) == 0xF8)
				{
					utf_i = 4;
					kbd_buf_pos -= 4;
				}

				else if(kbd_buf_pos >= 5 && (kbd_buf[kbd_buf_pos - 5] & 0xFE) == 0xFC)
				{
					utf_i = 5;
					kbd_buf_pos -= 5;
				}

				kbd_buf.erase(kbd_buf_pos, utf_i + 1);
			}

			// Delete
			else if(key_code == KEY_DC && kbd_buf_pos < kbd_buf.size())
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				// jeśli modyfikowano element historii, ustaw odpowiednią flagę informującą o tym
				if(hist_buf_max_items > 0 && hist_buf_index < hist_buf.size())
				{
					hist_mod = true;
				}

				// uwzględnij kodowanie UTF-8
				utf_i = 0;

				if(kbd_buf_pos + 1 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xE0) == 0xC0)
				{
					utf_i = 1;
				}

				else if(kbd_buf_pos + 2 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xF0) == 0xE0)
				{
					utf_i = 2;
				}

				else if(kbd_buf_pos + 3 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xF8) == 0xF0)
				{
					utf_i = 3;
				}

				else if(kbd_buf_pos + 4 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xFC) == 0xF8)
				{
					utf_i = 4;
				}

				else if(kbd_buf_pos + 5 < kbd_buf.size() && (kbd_buf[kbd_buf_pos] & 0xFE) == 0xFC)
				{
					utf_i = 5;
				}

				kbd_buf.erase(kbd_buf_pos, utf_i + 1);
			}

			// Home
			else if(key_code == KEY_HOME)
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				kbd_buf_pos = 0;
				kbd_cur_pos = 0;
			}

			// End
			else if(key_code == KEY_END)
			{
				// po kolejnym przejściu pętli głównej odśwież na ekranie zawartość bufora klawiatury
				kbd_buf_refresh = true;

				kbd_buf_pos = kbd_buf.size();
				kbd_cur_pos = buf_chars(kbd_buf);
			}

			// Page Up
			else if(key_code == KEY_PPAGE)
			{
				ci[ga.current]->win_scroll_lock = true;
				ga.win_chat_refresh = true;
			}

			// Page Down
			else if(key_code == KEY_NPAGE)
			{
				ci[ga.current]->win_scroll_lock = false;
				ga.win_chat_refresh = true;
			}

			// F2
			else if(key_code == KEY_F(2) && ga.current < CHAN_NORMAL)
			{
				if(ga.win_info_state)
				{
					ga.win_info_state = false;
				}

				else
				{
					ga.win_info_state = true;
					ga.win_info_refresh = true;
				}
			}

			// Alt Left + (...)
			else if(key_code == 0x1B || key_code == 0x21E || key_code == 0x22D)
			{
				// zapamiętaj obecny kod (trzeba go sprawdzić na terminalu Screen z Alt Left + Arrow Left i Alt Left + Arrow Right
				int prev_key_code = key_code;

				// sekwencja 0x1B 0x5B 0x31 0x3B 0x33 dla Alt Left + Arrow Left i Alt Left + Arrow Right na terminalu Screen (domyślnie brak)
				bool alt_arrow_seq = false;

				// lewy Alt generuje też kod klawisza, z którym został wciśnięty (dla poniższych sprawdzeń), dlatego pobierz ten kod
				key_code = getch();

				// "okna" od 1 do 9 (jeśli kanał istnieje, wybierz go (- '1', aby zamienić na cyfry 0x00...0x08))
				if(key_code >= '1' && key_code <= '9' && key_code - '1' < CHAN_CHAT && ci[key_code - '1'])
				{
					ga.current = key_code - '1';
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 10 (0 jest traktowane jak 10) (to nie pomyłka, że 9, bo numery są od 0)
				else if(key_code == '0' && 9 < CHAN_CHAT && ci[9])
				{
					ga.current = 9;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 11
				else if(key_code == 'q' && 10 < CHAN_CHAT && ci[10])
				{
					ga.current = 10;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 12
				else if(key_code == 'w' && 11 < CHAN_CHAT && ci[11])
				{
					ga.current = 11;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 13
				else if(key_code == 'e' && 12 < CHAN_CHAT && ci[12])
				{
					ga.current = 12;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 14
				else if(key_code == 'r' && 13 < CHAN_CHAT && ci[13])
				{
					ga.current = 13;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 15
				else if(key_code == 't' && 14 < CHAN_CHAT && ci[14])
				{
					ga.current = 14;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 16
				else if(key_code == 'y' && 15 < CHAN_CHAT && ci[15])
				{
					ga.current = 15;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 17
				else if(key_code == 'u' && 16 < CHAN_CHAT && ci[16])
				{
					ga.current = 16;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 18
				else if(key_code == 'i' && 17 < CHAN_CHAT && ci[17])
				{
					ga.current = 17;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 19
				else if(key_code == 'o' && 18 < CHAN_CHAT && ci[18])
				{
					ga.current = 18;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "okno" 20
				else if(key_code == 'p' && 19 < CHAN_CHAT && ci[19])
				{
					ga.current = 19;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "Status"
				else if(key_code == 's' && ci[CHAN_STATUS])
				{
					ga.current = CHAN_STATUS;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "DebugIRC"
				else if(key_code == 'd' && ci[CHAN_DEBUG_IRC])
				{
					ga.current = CHAN_DEBUG_IRC;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// "RawUnknown"
				else if(key_code == 'x' && ci[CHAN_RAW_UNKNOWN])
				{
					ga.current = CHAN_RAW_UNKNOWN;
					ga.win_chat_refresh = true;
					ga.win_info_refresh = true;
				}

				// wykryj sekwencję 0x1B 0x5B 0x31 0x3B 0x33 dla Alt Left + Arrow Left i Alt Left + Arrow Right na terminalu Screen
				if(prev_key_code == 0x1B && key_code == 0x5B && getch() == 0x31 && getch() == 0x3B && getch() == 0x33)
				{
					// true oznacza, że była powyższa sekwencja
					alt_arrow_seq = true;

					// pobierz ostatni kod, odróżniający Alt Left + Arrow Left od Alt Left + Arrow Right
					key_code = getch();
				}

				// Alt Left + Arrow Left
				// 0x21E - zwykły terminal
				// 0x1B 0x5B 0x31 0x3B 0x33 0x44 - terminal Screen
				if(prev_key_code == 0x21E || (alt_arrow_seq && key_code == 0x44))
				{
					for(int i = 0; i < CHAN_MAX; ++i)
					{
						--ga.current;

						if(ga.current < 0)
						{
							ga.current = CHAN_MAX - 1;
						}

						if(ci[ga.current])
						{
							ga.win_chat_refresh = true;
							ga.win_info_refresh = true;
							break;
						}
					}
				}

				// Alt Left + Arrow Right
				// 0x22D - zwykły terminal
				// 0x1B 0x5B 0x31 0x3B 0x33 0x43 - terminal Screen
				else if(prev_key_code == 0x22D || (alt_arrow_seq && key_code == 0x43))
				{
					for(int i = 0; i < CHAN_MAX; ++i)
					{
						++ga.current;

						if(ga.current == CHAN_MAX)
						{
							ga.current = 0;
						}

						if(ci[ga.current])
						{
							ga.win_chat_refresh = true;
							ga.win_info_refresh = true;
							break;
						}
					}
				}
			}
		}
/*
	Koniec obsługi klawiatury.
*/

/*
	Obsługa gniazda sieciowego dla IRC (socket).
*/
		// gniazdo (socket), sprawdzaj tylko wtedy, gdy socket jest aktywny
		if(ga.socketfd_irc > 0 && FD_ISSET(ga.socketfd_irc, &readfds_tmp))
		{
			// pobierz dane z serwera oraz zinterpretuj odpowiedź (obsłuż otrzymane dane)
			irc_parser(ga, ci);

			// jeśli serwer zakończy połączenie, przywróć parametry sprzed połączenia
			if(! ga.irc_ok)
			{
				// usuń socketfd_irc z zestawu select()
				FD_CLR(ga.socketfd_irc, &readfds);

				// zamknij oraz wyzeruj socket
				close(ga.socketfd_irc);
				ga.socketfd_irc = 0;

				// usuń wszystkie nicki ze wszystkich otwartych pokoi z listy oraz wyświetl komunikat (z wyjątkiem "DebugIRC" i "RawUnknown")
				for(int i = 0; i < CHAN_NORMAL; ++i)
				{
					if(ci[i])
					{
						ci[i]->ni.clear();

						win_buf_add_str(ga, ci, ci[i]->channel, xBOLD_ON xRED "# Rozłączono.");

						// odśwież listę w aktualnie otwartym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
						// nicka, który też jest w tym pokoju)
						if(ga.win_info_state && ga.current == i)
						{
							ga.win_info_refresh = true;
						}
					}
				}
			}
		}

/*
	Koniec obsługi gniazda sieciowego dla IRC (socket).
*/

	}
/*
	Koniec pętli głównej programu.
*/

/*
	Kończenie działania programu.
*/
	// jeśli podczas zamykania programu gniazdo IRC (socket) jest nadal otwarte, zamknij je
	if(ga.socketfd_irc > 0)
	{
		close(ga.socketfd_irc);
	}

	del_all_chan(ci);

	ga.debug_irc_f.close();
	ga.debug_http_f.close();

	if(win_info_active)
	{
		delwin(ga.win_info);
	}

	delwin(ga.win_chat);
	endwin();	// zakończ tryb ncurses

	fclose(stdin);

	return 0;
}
