#include <string>		// std::string, setlocale()
#include <cerrno>		// errno
#include <sys/select.h>		// select()
#include <sys/time.h>		// gettimeofday()
#include <unistd.h>		// close() - socket

#include "main_window.hpp"
#include "window_utils.hpp"
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

	// aby polskie znaki w UTF-8 wyświetlały się prawidłowo
	setlocale(LC_ALL, "");

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
	int kbd_buf_max = 0;		// początkowy maksymalny rozmiar bufora klawiatury
	int key_code;			// kod ostatnio wciśniętego klawisza
	std::string kbd_buf;		// bufor odczytanych znaków z klawiatury
	long hist_end = 0;
	bool hist_mod = false;
	bool hist_up = false;
	bool hist_down = false;
	std::string hist_buf, hist_ignore;
	int top_excess;
	bool chan_act_ok = false;
	short act_color;
	int ping_counter = 0;
/*
	Koniec zmiennych.
*/

/*
	Struktura globalnych zmiennych (nazwa może być myląca, chodzi o zmienne używane w wielu miejscach) oraz ustalenie wartości początkowych.
*/
	struct global_args ga;

	ga.ucc_dbg_irc = ucc_dbg_irc_main;

	ga.ucc_quit = false;		// aby zakończyć program, zmienna ta musi mieć wartość prawdziwą

	ga.socketfd_irc = 0;		// gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne

	ga.captcha_ready = false;	// stan wczytania captcha (jego pobranie z serwera)
	ga.irc_ready = false;		// gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
	ga.irc_ok = false;		// stan połączenia z czatem

	ga.zuousername = NICK_NOT_LOGGED;

	ga.nicklist = true;
	ga.nicklist_refresh = false;

	ga.ping = 0;
	ga.pong = 0;
	ga.lag = 0;
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
	if(use_colors_main)
	{
		ga.use_colors = check_colors();
	}

	// przy wyłączonych kolorach wyłącz je w zmiennej
	else
	{
		ga.use_colors = false;
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
			xBOLD_ON xGREEN "# Witaj w programie Ucieszony Chat Client\n"
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

/*
	Pętla główna programu.
*/
	while(! ga.ucc_quit)
	{
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

					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							xRED "# Serwer nie odpowiadał przez ponad " + std::to_string(PING_TIMEOUT) + "s, rozłączono.");
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

		// co 0.25s select() ma wychodzić z oczekiwania na klawiaturę lub socket (chodzi o pokazanie aktualnego czasu na dolnym pasku
		// oraz o aktualizację aktywności pokoi)
		tv.tv_sec = 0;
		tv.tv_usec = 250000;

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
	Informacje na pasku górnym.
*/
		move(0, 0);

		// temat pokoju
		// wyświetl, uwzględniając szerokość terminala (wyświetl tyle, ile się zmieści)
		top_excess = 0;

		for(int i = 0; i < static_cast<int>(chan_parm[ga.current]->topic.size()) && i < term_x + top_excess; ++i)
		{
			// wykryj znaki wielobajtowe w UTF-8 (konkretnie 2-bajtowe, wersja uproszczona, zakładająca, że nie będzie innych znaków),
			// aby zniwelować szerokość wyświetlania
			if((chan_parm[ga.current]->topic[i] & 0xE0) == 0xC0)
			{
				++top_excess;
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

		// wyświetl aktywność kanałów
		for(int i = 0; i < CHAN_MAX; ++i)
		{
			if(chan_parm[i])
			{
				// wykryj aktywność 3
				if(chan_parm[i]->chan_act == 3)
				{
					act_color = pMAGENTA_BLUE;
				}

				// wykryj aktywność 2
				else if(chan_parm[i]->chan_act == 2)
				{
					act_color = pWHITE_BLUE;
				}

				// wykryj aktywność 1
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

				// wyświetl wynik
				if(! chan_act_ok)
				{
					printw("[Pokoje: ");

					// numer pokoju w kolorze
					wattron_color(stdscr, ga.use_colors, act_color);

					if(act_color == pMAGENTA_BLUE || act_color == pWHITE_BLUE)
					{
						attron(A_BOLD);
					}

					printw("%d", i + 1);	// + 1, bo kanały od 1 a nie od 0

					chan_act_ok = true;	// gdy napisano "Aktywność: ", nie dopisuj tego ponownie
				}

				else
				{
					printw(",");

					// numer pokoju w kolorze
					wattron_color(stdscr, ga.use_colors, act_color);

					if(act_color == pMAGENTA_BLUE || act_color == pWHITE_BLUE)
					{
						attron(A_BOLD);
					}

					printw("%d", i + 1);	// + 1, bo kanały od 1 a nie od 0
				}

				// przywróć domyślny kolor paska bez bolda i podkreślenia
				wattron_color(stdscr, ga.use_colors, pWHITE_BLUE);
				attroff(A_BOLD);
				attroff(A_UNDERLINE);

			}
		}

		if(chan_act_ok)
		{
			printw("]");
		}

		chan_act_ok = false;

		// nazwa kanału
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
				delwin(ga.win_chat);
				delwin(ga.win_info);
				endwin();	// zakończ tryb ncurses
				del_all_chan(chan_parm);
				destroy_my_password(ga);
				fclose(stdin);

				return 3;
			}
		}

		// klawiatura
		if(FD_ISSET(0, &readfds_tmp))
		{
			key_code = getch();	// pobierz kod wciśniętego klawisza

			// Left Arrow
			if(key_code == KEY_LEFT)
			{
				if(kbd_buf_pos > 0)
				{
					--kbd_buf_pos;
				}
			}

			// Right Arrow
			else if(key_code == KEY_RIGHT)
			{
				if(kbd_buf_pos < kbd_buf_max)
				{
					++kbd_buf_pos;
				}
			}

			// Up Arrow
			else if(key_code == KEY_UP)
			{
				if(hist_mod && kbd_buf.size() > 0 && hist_ignore != kbd_buf)
				{
					hist_ignore = kbd_buf;
					hist_buf += kbd_buf + "\n";

					hist_up = true;

					hist_mod = false;
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
					kbd_buf_max = kbd_buf_pos;

					hist_end = hist_prev - 1;

					hist_up = true;
				}
			}

			// Down Arrow
			else if(key_code == KEY_DOWN)
			{
				if(hist_mod && kbd_buf.size() > 0 && hist_ignore != kbd_buf)
				{
					hist_ignore = kbd_buf;
					hist_buf += kbd_buf + "\n";

					hist_end = hist_buf.size() - 1;

					kbd_buf.clear();
					kbd_buf_pos = 0;
					kbd_buf_max = 0;

					hist_down = false;

					hist_mod = false;
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
						kbd_buf_max = kbd_buf_pos;

						hist_end = hist_next;

						hist_down = true;
					}

					else
					{
						kbd_buf.clear();
						kbd_buf_pos = 0;
						kbd_buf_max = 0;

						hist_down = false;
					}
				}

				// gdy to ostatni element historii, wyczyść bufor klawiatury
				else
				{
					kbd_buf.clear();
					kbd_buf_pos = 0;
					kbd_buf_max = 0;

					hist_down = false;
				}
			}

			// Backspace
			else if(key_code == KEY_BACKSPACE)
			{
				if(kbd_buf_pos > 0)
				{
					--kbd_buf_pos;
					--kbd_buf_max;
					kbd_buf.erase(kbd_buf_pos, 1);
				}
			}

			// Delete
			else if(key_code == KEY_DC)
			{
				if(kbd_buf_pos < kbd_buf_max)
				{
					--kbd_buf_max;
					kbd_buf.erase(kbd_buf_pos, 1);
				}
			}

			// Home
			else if(key_code == KEY_HOME)
			{
				kbd_buf_pos = 0;
			}

			// End
			else if(key_code == KEY_END)
			{
				kbd_buf_pos = kbd_buf_max;
			}

			// Page Up
			else if(key_code == KEY_PPAGE)
			{
			}

			// Page Down
			else if(key_code == KEY_NPAGE)
			{
			}

			// Alt Left + (...)
			else if(key_code == 0x1b)
			{
				// lewy Alt generuje też kod klawisza, z którym został wciśnięty (dla poniższych sprawdzeń), dlatego pobierz ten kod
				key_code = getch();

				if(key_code >= '1' && key_code <= '9')	// okna od 1 do 9
				{
					if(chan_parm[key_code - '1'])	// jeśli kanał istnieje, wybierz go (- '1', aby zamienić na cyfry 0x00...0x08)
					{
						ga.current = key_code - '1';
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == '0')	// okno 10 (0 jest traktowane jak 10)
				{
					if(chan_parm[9])	// to nie pomyłka, że 9, bo numery są od 0
					{
						ga.current = 9;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'q')	// okno 11
				{
					if(chan_parm[10])
					{
						ga.current = 10;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'w')	// okno 12
				{
					if(chan_parm[11])
					{
						ga.current = 11;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'e')	// okno 13
				{
					if(chan_parm[12])
					{
						ga.current = 12;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'r')	// okno 14
				{
					if(chan_parm[13])
					{
						ga.current = 13;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 't')	// okno 15
				{
					if(chan_parm[14])
					{
						ga.current = 14;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'y')	// okno 16
				{
					if(chan_parm[15])
					{
						ga.current = 15;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'u')	// okno 17
				{
					if(chan_parm[16])
					{
						ga.current = 16;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'i')	// okno 18
				{
					if(chan_parm[17])
					{
						ga.current = 17;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'o')	// okno 19
				{
					if(chan_parm[18])
					{
						ga.current = 18;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'p')	// okno 20
				{
					if(chan_parm[19])
					{
						ga.current = 19;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 'a')	// okno 21
				{
					if(chan_parm[20])
					{
						ga.current = 20;
						win_buf_refresh(ga, chan_parm);
					}
				}

				else if(key_code == 's')	// okno 22
				{
					if(chan_parm[21])
					{
						ga.current = 21;
						win_buf_refresh(ga, chan_parm);
					}
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
					// zaloguj do czata
					irc_auth(ga, chan_parm);

					// od tej pory, o ile poprawnie połączono się do IRC, można dodać socketfd_irc do zestawu select()
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

				}	// if(irc_ready)

				// po obsłudze bufora wyczyść go
				kbd_buf.clear();
				kbd_buf_pos = 0;
				kbd_buf_max = 0;
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
			else if(key_code >= 32 && key_code <= 255 && kbd_buf_max < KBD_BUF_MAX_SIZE)
			{
				// wstaw do bufora klawiatury odczytany znak i gdy to UTF-8, zamień go na ISO-8859-2
				kbd_buf.insert(kbd_buf_pos, key_utf2iso(key_code));
				++kbd_buf_pos;
				++kbd_buf_max;

				hist_mod = true;
			}

/*
			// kod tabulatora dodaj do bufora
			else if(key_code == '\t')
			{
				kbd_buf.insert(kbd_buf_pos, "\t");
				++kbd_buf_pos;
				++kbd_buf_max;
			}
*/

		}	// if(FD_ISSET(0, &readfds_tmp))

		// gniazdo (socket), sprawdzaj tylko, gdy socket jest aktywny
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
			}
		}

	}	// while(! ga.ucc_quit)

/*
	Kończenie działania programu.
*/
	if(ga.socketfd_irc > 0)	// jeśli podczas zamykania programu gniazdo IRC (socket) jest nadal otwarte, zamknij je
	{
		close(ga.socketfd_irc);
	}

	delwin(ga.win_chat);
	delwin(ga.win_info);
	endwin();	// zakończ tryb ncurses
	del_all_chan(chan_parm);
	destroy_my_password(ga);
	fclose(stdin);

	return 0;
}
