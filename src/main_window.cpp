#include <string>		// std::string, setlocale()
#include <sys/select.h>		// select()
#include <sys/time.h>		// gettimeofday()
#include <unistd.h>		// close() - socket

// -std=gnu++11 - errno

#include "main_window.hpp"
#include "window_utils.hpp"
#include "chat_utils.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


int main_window(bool use_colors_main, bool ucc_dbg_irc_main)
{
	// zapobiega zapętleniu się programu po wpisaniu w terminalu czegoś w stylu 'echo text | ucc'
	if(freopen("/dev/tty", "r", stdin) == NULL)
	{
		return 1;
	}

	// aby polskie znaki w UTF-8 wyświetlały się prawidłowo, przy okazji pl_PL ustawi polskie dni i miesiące przy pobieraniu daty z unixtimestamp
	setlocale(LC_ALL, "pl_PL.UTF-8");

	// inicjalizacja ncurses
	if(! initscr())
	{
		return 2;
	}

/*
	Zmienne.
*/
	int term_y, term_x;		// wymiary terminala

	int kbd_buf_pos = 0;		// początkowa pozycja bufora klawiatury (istotne podczas używania strzałek, Home, End, Delete itd.)
	int kbd_buf_end = 0;		// początkowy koniec (rozmiar) bufora klawiatury
	int key_code;			// kod ostatnio wciśniętego klawisza
	std::string kbd_buf;		// bufor odczytanych znaków z klawiatury

	long hist_end = 0;
	bool hist_mod = false;
	bool hist_up = false;
	bool hist_down = false;
	std::string hist_buf, hist_ignore;

//	int tab_index, tab_pos;
//	std::string tab_buf;

	int topic_utf8_excess;

	bool was_act;
	short act_color;

	int ping_counter = 0;
/*
	Koniec zmiennych.
*/

/*
	Struktura globalnych zmiennych (nazwa może być myląca, chodzi o zmienne używane w wielu miejscach) oraz ustalenie wartości początkowych.
*/
	struct global_args ga;

	ga.use_colors = use_colors_main;
	ga.ucc_dbg_irc = ucc_dbg_irc_main;

	ga.ucc_quit = false;		// aby zakończyć program, zmienna ta musi mieć wartość prawdziwą

	ga.socketfd_irc = 0;		// gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne

	ga.captcha_ready = false;	// stan wczytania captcha (jego pobranie z serwera)
	ga.irc_ready = false;		// gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
	ga.irc_ok = false;		// stan połączenia z czatem

	ga.zuousername = NICK_NOT_LOGGED;

	ga.nicklist = true;		// domyślnie włącz listę nicków
	ga.nicklist_refresh = true;

	ga.ping = 0;
	ga.pong = 0;
	ga.lag = 0;

	ga.f_dbg_http.open(FILE_DBG_HTTP, std::ios::out);
	ga.f_dbg_irc.open(FILE_DBG_IRC, std::ios::out);
/*
	Koniec ustalania globalnych zmiennych.
*/

	struct timeval t_ping, t_pong;

	struct timeval tv;		// struktura dla select(), aby ustawić czas wychodzenia z oczekiwania na klawiaturę lub socket, aby pokazać czas

	fd_set readfds;			// deskryptor dla select()
	fd_set readfds_tmp;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);		// klawiatura (stdin)

	raw();				// zablokuj Ctrl-C i Ctrl-Z
	keypad(stdscr, TRUE);		// klawisze funkcyjne będą obsługiwane
	noecho();			// nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)
	nodelay(stdscr, TRUE);		// nie blokuj getch() (zwróć ERR, gdy nie ma żadnego znaku do odczytania)

	// sprawdź, czy terminal obsługuje kolory, jeśli tak, włącz kolory oraz zainicjalizuj podstawową parę kolorów, ale tylko wtedy,
	// gdy uruchomiliśmy main_window() z use_colors_main = true, gdy terminal nie obsługuje kolorów, check_colors() zwróci false
	if(ga.use_colors)
	{
		ga.use_colors = check_colors();
	}

	// utwórz okno, w którym będą wyświetlane wszystkie kanały, privy, "Status" i "Debug" (jeśli włączony jest tryb debugowania IRC)
	getmaxyx(stdscr, term_y, term_x);	// pobierz wymiary terminala (okna głównego)

	if(ga.nicklist)
	{
		ga.win_chat = newwin(term_y - 3, term_x - NICKLIST_WIDTH, 1, 0);
	}

	else
	{
		ga.win_chat = newwin(term_y - 3, term_x, 1, 0);
	}

	scrollok(ga.win_chat, TRUE);		// włącz przewijanie w tym oknie

	// tablica kanałów
	struct channel_irc *chan_parm[CHAN_MAX] = {};		// wyzeruj od razu tablicę

	// kanał "Status" zawsze pod numerem 0 w tablicy (nie mylić z włączaniem go kombinacją Alt+1)
	new_chan_status(ga, chan_parm);

	// utwórz okno, w którym będzie wyświetlona lista nicków, a w "Status" informacje o użytkowniku i programie
	nicklist_on(ga);

	// wpisz do bufora "Status" komunikat startowy w kolorze zielonym oraz cyjan (kolor będzie wtedy, gdy terminal obsługuje kolory) i go wyświetl
	win_buf_add_str(ga, chan_parm, "Status",
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
			xCYAN  "/quit " xGREEN "lub " xCYAN "/q");		// ze względu na przyjętą budowę bufora na końcu nie ma \n

	// jeśli trzeba, utwórz kanał "Debug"
	if(ga.ucc_dbg_irc)
	{
		new_chan_debug_irc(ga, chan_parm);
	}

	// co 0.25s select() ma wychodzić z oczekiwania na klawiaturę lub socket (chodzi o pokazanie aktualnego czasu na dolnym pasku
	// oraz o aktualizację aktywności pokoi)
	tv.tv_sec = 0;
	tv.tv_usec = 250000;

/*
	Pętla główna programu.
*/
	while(! ga.ucc_quit)
	{
		// wykryj zmianę rozmiaru okna terminala
		if(is_term_resized(term_y, term_x))
		{
			getmaxyx(stdscr, term_y, term_x);	// pobierz nowe wymiary terminala (okna głównego) po jego zmianie
			wresize(stdscr, term_y, term_x);	// zmień rozmiar okna głównego po zmianie rozmiaru okna terminala

			if(ga.nicklist)
			{
				wresize(ga.win_chat, term_y - 3, term_x - NICKLIST_WIDTH);	// j/w, ale dla okna diagnostycznego
			}

			else
			{
				wresize(ga.win_chat, term_y - 3, term_x);	// j/w, ale dla okna diagnostycznego
			}

			// po zmianie wielkości okna terminala należy uaktualnić jego zawartość
			win_buf_refresh(ga, chan_parm);

			if(ga.nicklist)
			{
				nicklist_off(ga);
				nicklist_on(ga);
			}

			ga.nicklist_refresh = true;
		}

/*
	Narysuj paski.
*/
		// paski (jeśli terminal obsługuje kolory, paski będą niebieskie)
		if(ga.use_colors)
		{
			wattron_color(stdscr, ga.use_colors, pWHITE_BLUE);
		}

		else
		{
			attrset(A_NORMAL);
			attron(A_REVERSE);
		}

		// pasek górny
		move(0, 0);

		for(int i = 0; i < term_x; ++i)
		{
			printw(" ");
		}

		// pasek dolny
		move(term_y - 2, 0);

		for(int i = 0; i < term_x; ++i)
		{
			printw(" ");
		}
/*
	Koniec rysowania pasków.
*/

/*
	Informacje na pasku górnym.
*/
		move(0, 0);

		// temat pokoju
		// wyświetl, uwzględniając szerokość terminala (wyświetl tyle, ile się zmieści)
		topic_utf8_excess = 0;

		for(int i = 0; i < static_cast<int>(chan_parm[ga.current]->topic.size()) && i < term_x + topic_utf8_excess; ++i)
		{
			// wykryj znaki wielobajtowe w UTF-8 (konkretnie 2-bajtowe, wersja uproszczona, zakładająca, że nie będzie innych znaków),
			// aby zniwelować szerokość wyświetlania
			if((chan_parm[ga.current]->topic[i] & 0xE0) == 0xC0)
			{
				++topic_utf8_excess;
			}

			printw("%c", chan_parm[ga.current]->topic[i]);
		}
/*
	Koniec informacji na pasku górnym.
*/

/*
	Informacje na pasku dolnym.
*/
		move(term_y - 2, 0);

		//pokaż aktualny czas
		printw("%s", get_time().erase(0, 1).c_str());	// .erase(0, 1) - usuń kod \x17 (na paskach kody nie są obsługiwane)

		// skasuj flagi aktywności w aktualnie otwartym pokoju
		chan_parm[ga.current]->chan_act = 0;

		// wyświetl aktywność pokoi
		was_act = false;

		printw("[Pokoje: ");

		for(int i = 0; i < CHAN_MAX; ++i)
		{
			if(chan_parm[i])
			{
				// wykryj aktywność typu 3
				if(chan_parm[i]->chan_act == 3)
				{
					act_color = pMAGENTA_BLUE;
				}

				// wykryj aktywność typu 2
				else if(chan_parm[i]->chan_act == 2)
				{
					act_color = pWHITE_BLUE;
				}

				// wykryj aktywność typu 1
				else if(chan_parm[i]->chan_act == 1)
				{
					act_color = pCYAN_BLUE;
				}

				// brak aktywności
				else
				{
					// numer aktualnie otwartego pokoju wyświetl na białym tle
					if(i == ga.current)
					{
						act_color = pBLUE_WHITE;
					}

					else
					{
						act_color = pBLACK_BLUE;
					}
				}

				// jeśli był już wyświetlony pokój, dopisz po nim przecinek na liście
				if(was_act)
				{
					printw(",");
				}

				was_act = true;

				// numer pokoju w kolorze
				wattron_color(stdscr, ga.use_colors, act_color);

				if(act_color == pMAGENTA_BLUE || act_color == pWHITE_BLUE)
				{
					attron(A_BOLD);
				}

				// okno "Status" jako literka "s"
				if(i == CHAN_STATUS)
				{
					printw("s");
				}

				// pokój czata jako cyfra
				else if(i < CHAN_CHAT)
				{
					printw("%d", i + 1);	// + 1, bo pokoje na pasku są od 1 a nie od 0 (jak w tablicy pokoi)
				}

				// okno "Debug" jako literka "d"
				else if(i == CHAN_DEBUG_IRC)
				{
					printw("d");
				}

				// okno "RawUnknown" jako literka "x" (bez else if, bo to i tak ostatni kanał na liście)
				else
				{
					printw("x");
				}

				// przywróć domyślny kolor paska bez bolda
				wattron_color(stdscr, ga.use_colors, pWHITE_BLUE);
				attroff(A_BOLD);
			}
		}

		// nawias zamykający
		printw("]");

		// nazwa pokoju
		printw(" [%s]", chan_parm[ga.current]->channel.c_str());

		// pokaż lag
		if(ga.lag > 0)
		{
			if(ga.lag < 1000)
			{
				printw(" [Lag: %dms]", ga.lag);
			}

			else
			{
				printw(" [Lag: %.2fs]", ga.lag / 1000.00);
			}
		}
/*
	Koniec informacji na pasku dolnym.
*/

		// pokaż nicki
		if(ga.nicklist && ga.nicklist_refresh)
		{
			nicklist_refresh(ga, chan_parm);
		}

		// wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
		// oraz ustaw kursor w obecnie przetwarzany znak
		kbd_buf_show(kbd_buf, ga.zuousername, term_y, term_x, kbd_buf_pos);

/*
	Czekaj na aktywność klawiatury lub gniazda (socket).
*/
		readfds_tmp = readfds;

		if(select(ga.socketfd_irc + 1, &readfds_tmp, NULL, NULL, &tv) == -1)
		{
			// sygnał SIGWINCH (zmiana rozmiaru okna terminala) powoduje, że select() zwraca -1, więc trzeba to wykryć, aby nie wywalić programu
			// w kosmos
			if(errno == EINTR)	// Interrupted system call (wywołany np. przez SIGWINCH)
			{
				getch();	// ignoruj KEY_RESIZE
				continue;	// wróć do początku pętli while()
			}

			// inny błąd select() powoduje zakończenie działania programu
			else
			{
				ga.f_dbg_http.close();
				ga.f_dbg_irc.close();
				delwin(ga.win_chat);
				delwin(ga.win_info);
				endwin();	// zakończ tryb ncurses
				del_all_chan(chan_parm);
				destroy_my_password(ga.my_password);
				fclose(stdin);

				return 3;
			}
		}

		// co 0.25s aktualizuj licznik (trzeba pamiętać, że timeout to nie jedyny sposób wyjścia z select(), tak samo klawiatura i gniazdo przerywają)
		if(tv.tv_sec == 0 && tv.tv_usec == 0)
		{
			tv.tv_sec = 0;
			tv.tv_usec = 250000;

/*
	Obsługa PING.
*/
			// licznik dla PING
			if(ga.irc_ok)
			{
				++ping_counter;
			}

			else
			{
				ping_counter = 0;
				ga.lag = 0;
				ga.lag_timeout = false;
			}

			// PING do serwera co liczbę sekund w PING_TIME (gdy klient jest zalogowany)
			if(ga.irc_ok && ping_counter == PING_TIME * 4)
			{
				if(ga.lag_timeout)
				{
					gettimeofday(&t_pong, NULL);
					ga.pong = t_pong.tv_sec * 1000;
					ga.pong += t_pong.tv_usec / 1000;

					ga.lag += ga.pong - ga.ping;

					if(ga.lag >= PING_TIMEOUT * 1000)
					{
						ga.irc_ok = false;
						FD_CLR(ga.socketfd_irc, &readfds);
						ga.zuousername = NICK_NOT_LOGGED;

						if(ga.socketfd_irc > 0)
						{
							close(ga.socketfd_irc);
							ga.socketfd_irc = 0;
						}

						// usuń wszystkie nicki ze wszystkich otwartych pokoi z listy oraz wyświetl komunikat we wszystkich otwartych
						// pokojach (poza "Debug" i "RawUnknown")
						for(int i = 0; i < CHAN_NORMAL; ++i)
						{
							if(chan_parm[i])
							{
								chan_parm[i]->nick_parm.clear();

								win_buf_add_str(ga, chan_parm, chan_parm[i]->channel,
										xBOLD_ON xRED "# Serwer nie odpowiadał przez ponad "
										+ std::to_string(PING_TIMEOUT) + "s, rozłączono.");
							}
						}

						ga.nicklist_refresh = true;
					}
				}

				ga.lag_timeout = true;
				ping_counter = 0;
				gettimeofday(&t_ping, NULL);
				ga.ping = t_ping.tv_sec * 1000;
				ga.ping += t_ping.tv_usec / 1000;

				if(ga.irc_ok)
				{
					irc_send(ga, chan_parm, "PING :" + std::to_string(ga.ping));
				}
			}
/*
	Koniec obsługi PING.
*/

		}

		// klawiatura
		if(FD_ISSET(0, &readfds_tmp))
		{
			key_code = getch();	// pobierz kod wciśniętego klawisza

			// Left Arrow
			if(key_code == KEY_LEFT && kbd_buf_pos > 0)
			{
				--kbd_buf_pos;
			}

			// Right Arrow
			else if(key_code == KEY_RIGHT && kbd_buf_pos < kbd_buf_end)
			{
				++kbd_buf_pos;
			}

			// Up Arrow
			else if(key_code == KEY_UP)
			{
				if(hist_mod && kbd_buf.size() > 0 && hist_ignore != kbd_buf)
				{
					// jeśli wpisano polecenie /nick wraz z hasłem, to hasło wytnij z historii
					hist_erase_password(kbd_buf, hist_buf, hist_ignore);

					hist_mod = false;
					hist_up = true;
				}

				if(hist_end > 0)
				{
					size_t hist_prev = hist_buf.rfind("\n", hist_end - 1);

					// gdy poprzednio używano Down Arrow, trzeba 2x pominąć \n
					if(hist_down)
					{
						hist_end = hist_prev;
						hist_prev = hist_buf.rfind("\n", hist_end - 1);
						hist_down = false;
					}

					if(hist_prev == std::string::npos)
					{
						hist_prev = 0;
					}

					else
					{
						++hist_prev;
					}

					kbd_buf.clear();
					kbd_buf.insert(0, hist_buf, hist_prev, hist_end - hist_prev);

					kbd_buf_pos = kbd_buf.size();
					kbd_buf_end = kbd_buf_pos;

					hist_end = hist_prev - 1;
					hist_up = true;
				}
			}

			// Down Arrow
			else if(key_code == KEY_DOWN)
			{
				if(hist_mod && kbd_buf.size() > 0 && hist_ignore != kbd_buf)
				{
					// jeśli wpisano polecenie /nick wraz z hasłem, to hasło wytnij z historii
					hist_erase_password(kbd_buf, hist_buf, hist_ignore);

					hist_end = hist_buf.size() - 1;

					kbd_buf.clear();
					kbd_buf_pos = 0;
					kbd_buf_end = 0;

					hist_mod = false;
					hist_down = false;
				}

				else if(hist_end + 1 < static_cast<long>(hist_buf.size()))
				{
					size_t hist_next = hist_buf.find("\n", hist_end + 1);

					// gdy poprzednio używano Up Arrow, trzeba 2x pominąć \n
					if(hist_up)
					{
						hist_end = hist_next;
						hist_next = hist_buf.find("\n", hist_end + 1);
						hist_up = false;
					}

					if(hist_next != std::string::npos)
					{
						kbd_buf.clear();
						kbd_buf.insert(0, hist_buf, hist_end + 1, hist_next - hist_end - 1);

						kbd_buf_pos = kbd_buf.size();
						kbd_buf_end = kbd_buf_pos;

						hist_end = hist_next;
						hist_down = true;
					}

					else
					{
						kbd_buf.clear();
						kbd_buf_pos = 0;
						kbd_buf_end = 0;

						hist_down = false;
					}
				}

				// gdy to ostatni element historii, wyczyść bufor klawiatury
				else
				{
					kbd_buf.clear();
					kbd_buf_pos = 0;
					kbd_buf_end = 0;

					hist_down = false;
				}
			}

			// Backspace
			else if(key_code == KEY_BACKSPACE && kbd_buf_pos > 0)
			{
				--kbd_buf_pos;
				--kbd_buf_end;
				kbd_buf.erase(kbd_buf_pos, 1);
			}

			// Delete
			else if(key_code == KEY_DC && kbd_buf_pos < kbd_buf_end)
			{
				--kbd_buf_end;
				kbd_buf.erase(kbd_buf_pos, 1);
			}

			// Home
			else if(key_code == KEY_HOME)
			{
				kbd_buf_pos = 0;
			}

			// End
			else if(key_code == KEY_END)
			{
				kbd_buf_pos = kbd_buf_end;
			}

			// Page Up
			else if(key_code == KEY_PPAGE)
			{
			}

			// Page Down
			else if(key_code == KEY_NPAGE)
			{
//				win_buf_add_str(ga, chan_parm, "Status", "tab_buf: " + tab_buf);
			}

			// Tab
			else if(key_code == '\t')
			{
				//
			}

/*
			// kod tabulatora dodaj do bufora - dodać wklejanie tylko ze schowka
			else if(key_code == '\t')
			{
				kbd_buf.insert(kbd_buf_pos, "\t");
				++kbd_buf_pos;
				++kbd_buf_end;
			}
*/

			// Alt Left + (...)
			else if(key_code == 0x1b)
			{
				// lewy Alt generuje też kod klawisza, z którym został wciśnięty (dla poniższych sprawdzeń), dlatego pobierz ten kod
				key_code = getch();

				// "okna" od 1 do 9 (jeśli kanał istnieje, wybierz go (- '1', aby zamienić na cyfry 0x00...0x08))
				if(key_code >= '1' && key_code <= '9' && chan_parm[key_code - '1'])
				{
					ga.current = key_code - '1';
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 10 (0 jest traktowane jak 10) (to nie pomyłka, że 9, bo numery są od 0)
				else if(key_code == '0' && chan_parm[9])
				{
					ga.current = 9;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 11
				else if(key_code == 'q' && chan_parm[10])
				{
					ga.current = 10;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 12
				else if(key_code == 'w' && chan_parm[11])
				{
					ga.current = 11;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 13
				else if(key_code == 'e' && chan_parm[12])
				{
					ga.current = 12;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 14
				else if(key_code == 'r' && chan_parm[13])
				{
					ga.current = 13;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 15
				else if(key_code == 't' && chan_parm[14])
				{
					ga.current = 14;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 16
				else if(key_code == 'y' && chan_parm[15])
				{
					ga.current = 15;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 17
				else if(key_code == 'u' && chan_parm[16])
				{
					ga.current = 16;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 18
				else if(key_code == 'i' && chan_parm[17])
				{
					ga.current = 17;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 19
				else if(key_code == 'o' && chan_parm[18])
				{
					ga.current = 18;
					win_buf_refresh(ga, chan_parm);
				}

				// "okno" 20
				else if(key_code == 'p' && chan_parm[19])
				{
					ga.current = 19;
					win_buf_refresh(ga, chan_parm);
				}

				// "Status"
				else if(key_code == 's' && chan_parm[CHAN_STATUS])
				{
					ga.current = CHAN_STATUS;
					win_buf_refresh(ga, chan_parm);
				}

				// "Debug"
				else if(key_code == 'd' && chan_parm[CHAN_DEBUG_IRC])
				{
					ga.current = CHAN_DEBUG_IRC;
					win_buf_refresh(ga, chan_parm);
				}

				// "RawUnknown"
				else if(key_code == 'x' && chan_parm[CHAN_RAW_UNKNOWN])
				{
					ga.current = CHAN_RAW_UNKNOWN;
					win_buf_refresh(ga, chan_parm);
				}
			}

			// Alt Left + Arrow Left
			else if(key_code == 0x21e)
			{
				for(int i = 0; i < CHAN_MAX; ++i)
				{
					--ga.current;

					if(ga.current < 0)
					{
						ga.current = CHAN_MAX - 1;
					}

					if(chan_parm[ga.current])
					{
						win_buf_refresh(ga, chan_parm);
						break;
					}
				}
			}

			// Alt Left + Arrow Right
			else if(key_code == 0x22d)
			{
				for(int i = 0; i < CHAN_MAX; ++i)
				{
					++ga.current;

					if(ga.current == CHAN_MAX)
					{
						ga.current = 0;
					}

					if(chan_parm[ga.current])
					{
						win_buf_refresh(ga, chan_parm);
						break;
					}
				}
			}

			// F2
			else if(key_code == KEY_F(2))
			{
				getmaxyx(stdscr, term_y, term_x);

				if(ga.nicklist)
				{
					wresize(ga.win_chat, term_y - 3, term_x);
					nicklist_off(ga);
					ga.nicklist = false;
				}

				else
				{
					wresize(ga.win_chat, term_y - 3, term_x - NICKLIST_WIDTH);
					nicklist_on(ga);
					nicklist_refresh(ga, chan_parm);
					ga.nicklist = true;
				}

				win_buf_refresh(ga, chan_parm);
			}

			// Enter (0x0A) - wykonaj obsługę bufora klawiatury, ale tylko wtedy, gdy coś w nim jest
			else if(key_code == '\n' && kbd_buf.size() > 0)
			{
				// zapisz bufor klawiatury do bufora historii (końcowy znak \n mówi o końcu danego elementu historii), pomijaj te same
				// elementy wpisane poprzednio (dla jednego wpisu historii, a nie wszystkich)
				if(hist_ignore != kbd_buf)
				{
					// jeśli wpisano polecenie /nick wraz z hasłem, to hasło wytnij z historii
					hist_erase_password(kbd_buf, hist_buf, hist_ignore);
				}

				hist_end = hist_buf.size() - 1;
				hist_mod = false;

				// jeśli wciśnięto strzałkę w dół przed Enter, zniweluj jej działanie w powtarzaniu historii
				hist_down = false;

				// "wyczyść" pole wpisywanego tekstu (aby nie było widać zwłoki, np. podczas pobierania obrazka z kodem do przepisania)
				move(term_y - 1, ga.zuousername.size() + 3);	// ustaw kursor za nickiem i spacją za nawiasem
				clrtoeol();
				refresh();

				// wykonaj obsługę bufora klawiatury (zidentyfikuj polecenie i wykonaj odpowiednie działanie)
				kbd_parser(ga, chan_parm, kbd_buf);

				// sprawdź gotowość do połączenia z IRC
				if(ga.irc_ready)
				{
					// zaloguj się do czata
					auth_irc_all(ga, chan_parm);

					// od tej pory, o ile poprawnie połączono się z IRC, można dodać socketfd_irc do zestawu select()
					if(ga.irc_ok)
					{
						FD_SET(ga.socketfd_irc, &readfds);		// gniazdo IRC (socket)
					}

					// gdy połączenie do IRC nie powiedzie się, wyzeruj socket oraz przywróć nick na pasku na domyślny
					else
					{
						ga.zuousername = NICK_NOT_LOGGED;

						if(ga.socketfd_irc > 0)
						{
							close(ga.socketfd_irc);
							ga.socketfd_irc = 0;
						}
					}
				}

				// po obsłudze bufora wyczyść go
				kbd_buf.clear();
				kbd_buf_pos = 0;
				kbd_buf_end = 0;
			}

			// Enter, gdy nic nie ma w buforze klawiatury powoduje ustawienie w historii ostatnio wpisanego elementu
			else if(key_code == '\n' && kbd_buf.size() == 0)
			{
				hist_end = hist_buf.size() - 1;
				hist_mod = false;

				// jeśli wciśnięto strzałkę w dół przed Enter, zniweluj jej działanie w powtarzaniu historii
				hist_down = false;
			}

			// kody ASCII (oraz rozszerzone) wczytaj do bufora (te z zakresu 32...255), jednocześnie ogranicz pojemność bufora wejściowego
			else if(key_code >= 32 && key_code <= 255 && kbd_buf_end < KBD_BUF_MAX_SIZE)
			{
				// wstaw do bufora klawiatury odczytany znak i gdy to UTF-8, zamień go na ISO-8859-2
				kbd_buf.insert(kbd_buf_pos, key_utf2iso(key_code));
				++kbd_buf_pos;
				++kbd_buf_end;

				hist_mod = true;
			}
		}

		// gniazdo (socket), sprawdzaj tylko wtedy, gdy socket jest aktywny
		if(ga.socketfd_irc > 0 && FD_ISSET(ga.socketfd_irc, &readfds_tmp))
		{
			// pobierz dane z serwera oraz zinterpretuj odpowiedź (obsłuż otrzymane dane)
			irc_parser(ga, chan_parm);

			// gdy serwer zakończy połączenie, usuń socketfd_irc z zestawu select(), wyzeruj socket oraz przywróć nick z czata na domyślny
			if(! ga.irc_ok)
			{
				FD_CLR(ga.socketfd_irc, &readfds);
				ga.zuousername = NICK_NOT_LOGGED;

				if(ga.socketfd_irc > 0)
				{
					close(ga.socketfd_irc);
					ga.socketfd_irc = 0;
				}

				// usuń wszystkie nicki ze wszystkich otwartych pokoi z listy oraz wyświetl komunikat (z wyjątkiem "Debug" i "RawUnknown")
				for(int i = 0; i < CHAN_NORMAL; ++i)
				{
					if(chan_parm[i])
					{
						chan_parm[i]->nick_parm.clear();
						win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, xBOLD_ON xRED "# Rozłączono.");
					}
				}

				ga.nicklist_refresh = true;
			}
		}
	}

/*
	Kończenie działania programu.
*/
	if(ga.socketfd_irc > 0)	// jeśli podczas zamykania programu gniazdo IRC (socket) jest nadal otwarte, zamknij je
	{
		close(ga.socketfd_irc);
	}

	ga.f_dbg_http.close();
	ga.f_dbg_irc.close();
	delwin(ga.win_chat);
	delwin(ga.win_info);
	endwin();	// zakończ tryb ncurses
	del_all_chan(chan_parm);
	destroy_my_password(ga.my_password);
	fclose(stdin);

	return 0;
}
