#include <string>		// std::string, setlocale()
#include <malloc.h>
#include <cerrno>		// errno
#include <sys/select.h>		// select()

#include "main_window.hpp"
#include "window_utils.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "auth.hpp"
#include "network.hpp"
#include "ucc_global.hpp"


int main_window(bool use_colors_main, bool ucc_dbg_irc)
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

	std::string buffer_irc_recv;	// bufor odebranych danych z IRC
	std::string buffer_irc_sent;	// dane wysłane do serwera w irc_auth_x() (informacje przydatne do debugowania)

	std::string msg_scr;
	std::string msg_irc;
/*
	Koniec zmiennych.
*/

/*
	Struktura globalnych zmiennych (nazwa może być myląca, chodzi o zmienne używane w wielu miejscach) oraz ustalenie wartości początkowych.
*/
	struct global_args ga;

	ga.ucc_quit = false;		// aby zakończyć program, zmienna ta musi mieć wartość prawdziwą

	ga.socketfd_irc = 0;		// gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne

	ga.captcha_ready = false;	// stan wczytania captcha (jego pobranie z serwera)
	ga.irc_ready = false;		// gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
	ga.irc_ok = false;		// stan połączenia z czatem
//	ga.channel_ok = false;		// stan wejścia do pokoju (kanału)

	ga.zuousername = "Niezalogowany";

	ga.chan_nr = 0;			// zacznij od kanału "Status"
/*
	Koniec ustalania globalnych zmiennych.
*/

	// tablica kanałów
	struct channel_irc *chan_parm[ALL_CHAN] = {};		// wyzeruj od razu tablicę

	// kanał "Status" zawsze pod numerem 0 w tablicy i zawsze istnieje w programie (nie mylić z włączaniem go kombinacją Alt+1)
	chan_parm[0] = new channel_irc;
//	chan_parm[1] = new channel_irc;

	chan_parm[ga.chan_nr]->channel_ok = false;	// w kanale "Status" nie można pisać tak jak w zwykłym pokoju czata
	chan_parm[ga.chan_nr]->channel = "Status";	// nazwa kanału statusowego

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

	// utwórz okno, w którym będą wyświetlane wszystkie kanały, privy, status i debug (gdy włączony)
	getmaxyx(stdscr, term_y, term_x);	// pobierz wymiary terminala (okna głównego)
	ga.win_chat = newwin(term_y - 3, term_x, 1, 0);
	scrollok(ga.win_chat, TRUE);		// włącz przewijanie w tym oknie

	// wpisz do bufora "Status" komunikat powitalny w kolorze zielonym oraz cyjan wraz z godziną na początku każdego wiersza (kolor będzie wtedy,
	// gdy terminal obsługuje kolory)
	chan_parm[0]->win_buf =	get_time() + xGREEN + xBOLD_ON + "Ucieszony Chat Client" + xBOLD_OFF +
				get_time() + xGREEN + "# Aby zalogować się na nick tymczasowy, wpisz:" +
				get_time() + xCYAN  + "/nick nazwa_nicka" +
				get_time() + xCYAN  + "/connect" +
				get_time() + xGREEN + "# Następnie przepisz kod z obrazka, w tym celu wpisz:" +
				get_time() + xCYAN  + "/captcha kod_z_obrazka" +
				get_time() + xGREEN + "# Aby zalogować się na nick stały (zarejestrowany), wpisz:" +
				get_time() + xCYAN  + "/nick nazwa_nicka hasło_do_nicka" +
				get_time() + xCYAN  + "/connect" +
				get_time() + xGREEN + "# Aby zobaczyć dostępne polecenia, wpisz:" +
				get_time() + xCYAN  + "/help" +
				get_time() + xGREEN + "# Aby zakończyć działanie programu, wpisz:" +
				get_time() + xCYAN  + "/quit lub /q";

	// wyświetl zawartość bufora "Status" na ekranie
	wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[0]->win_buf);

	// zapamiętaj aktualną pozycję kursora w oknie diagnostycznym
//	getyx(win_chat, cur_y, cur_x);

/*
	Tymczasowe wskaźniki pomocnicze, usunąć po testach.
*/
	int ix = 0, iy = 0;
/*
	Koniec wskaźników pomocniczych.
*/

	// pętla główna programu
	while(! ga.ucc_quit)
	{
		// co 0.25s select() ma wychodzić z oczekiwania na klawiaturę lub socket (chodzi o pokazanie aktualnego czasu na dolnym pasku)
		tv.tv_sec = 0;
		tv.tv_usec = 250000;

		readfds_tmp = readfds;

		// wykryj zmianę rozmiaru okna terminala
		if(is_term_resized(term_y, term_x))
		{
			getmaxyx(stdscr, term_y, term_x);	// pobierz nowe wymiary terminala (okna głównego) po jego zmianie
			wresize(stdscr, term_y, term_x);	// zmień rozmiar okna głównego po zmianie rozmiaru okna terminala
			wresize(ga.win_chat, term_y - 3, term_x);	// j/w, ale dla okna diagnostycznego

/*
			// po zmianie rozmiaru terminala sprawdź, czy maksymalna pozycja kursora Y nie jest większa od wymiarów okna
			if(cur_y >= term_y - 3)
			{
				cur_y = term_y - 4;		// - 4, bo piszemy do max granicy, nie wchodząc na nią
			}
*/

			// po zmianie wielkości okna terminala należy uaktualnić jego zawartość
			wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
		}

		// paski (jeśli terminal obsługuje kolory, paski będą niebieskie)
		wattron_color(stdscr, ga.use_colors, pBLUE_WHITE);
		attron(A_REVERSE);
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
		Tymczasowo pokazuj informacje pomocnicze, usunąć po testach (poza pokazywaniem czasu).
*/
		// wyświetl wymiary terminala
		move(0, 0);
		printw("term: %dx%d", term_x, term_y);
//		printw("\t\tkey_code: 0x%x", key_code);

		move(term_y - 2, 0);
		//pokaż aktualny czas
		printw("%s", get_time().erase(0, 3).c_str());	// .erase(0, 3) - usuń kody \n\x03\x08 ze string (nowy wiersz i formatowanie kolorów)
		// nr kanału i jego nazwa
		printw("[%d:%s] ", ga.chan_nr + 1, chan_parm[ga.chan_nr]->channel.c_str());

		printw("kbd+irc: %d, kbd: %d, irc: %d, socketfd_irc: %d", ix + iy, ix, iy, ga.socketfd_irc);
/*
		Koniec informacji tymczasowych.
*/

		// wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
		// oraz ustaw kursor w obecnie przetwarzany znak
		kbd_buf_show(kbd_buf, ga.zuousername, term_y, term_x, kbd_buf_pos);

		// czekaj na aktywność klawiatury lub gniazda (socket)
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
				endwin();	// zakończ tryb ncurses
				del_all_chan(chan_parm);
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

			// Alt Left
			else if(key_code == 0x1b)
			{
				key_code = getch();	// lewy Alt zawsze generuje też kod klawisza, z którym został wciśnięty, dlatego pobierz ten kod

				if(key_code >= '1' || key_code <= '9')
				{
					if(chan_parm[key_code - '1'])	// jeśli kanał istnieje, wybierz go (- '1', aby zamienić na cyfry 0x00...0x08)
					{
						ga.chan_nr = key_code - '1';
						wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
					}
				}

				else if(key_code == '0')	// 0 jest traktowane jak 10
				{
					if(chan_parm[9])	// to nie pomyłka, że 9, bo numery są od 0
					{
						ga.chan_nr = 9;
						wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
					}
				}

				else if(key_code == 'd' && ucc_dbg_irc)
				{
					//ga.chan_nr = ALL_CHAN - 1;	// debugowanie w ostatnim kanale pod kombinacją Alt+d
					//wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
				}
			}

			// Enter (0x0A) - wykonaj obsługę bufora klawiatury, ale tylko wtedy, gdy coś w nim jest
			else if(key_code == '\n' && kbd_buf.size() > 0)
			{
				// "wyczyść" pole wpisywanego tekstu (aby nie było widać zwłoki, np. podczas pobierania obrazka z kodem do przepisania)
				move(term_y - 1, ga.zuousername.size() + 3);	// ustaw kursor za nickiem i spacją za nawiasem
				clrtoeol();
				refresh();

				// wykonaj obsługę bufora klawiatury (zidentyfikuj polecenie lub zwróć wpisany tekst do wysłania do IRC)
				kbd_parser(kbd_buf, msg_scr, msg_irc, chan_parm, ga);

				// gdy parser zwrócił jakiś komunikat do wyświetlenia, pokaż go
				if(msg_scr.size() > 0)
				{
					chan_parm[ga.chan_nr]->win_buf += msg_scr;		// dopisz do bufora komunikat z parsera
					wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
				}

				// jeśli w buforze IRC parser zwrócił jakiś komunikat i jeśli połączono z IRC (sprawdzanie nie jest konieczne, ale dodano
				// na wszelki wypadek), wyślij go na serwer
				if(msg_irc.size() > 0 && ga.irc_ok)
				{
					if(! irc_send(ga.socketfd_irc, ga.irc_ok, msg_irc, msg_scr))
					{
						// w przypadku błędu pokaż co się stało
						chan_parm[ga.chan_nr]->win_buf += msg_scr;		// dopisz do bufora
						wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
					}
				}

				// sprawdź gotowość do połączenia z IRC
				if(ga.irc_ready)
				{
					// zaloguj do czata
					irc_auth_all(ga, chan_parm);

					// od tej pory, o ile poprawnie połączono się do IRC, można dodać socketfd_irc do zestawu select()
					if(ga.irc_ok)
					{
						FD_SET(ga.socketfd_irc, &readfds);		// gniazdo IRC (socket)
					}

					// gdy połączenie do IRC nie powiedzie się, wyzeruj socket i ustaw z powrotem nick w pasku wpisywania na Niezalogowany
					else
					{
						ga.socketfd_irc = 0;
						ga.zuousername = "Niezalogowany";
					}

				}	// if(irc_ready)

				// zachowaj pozycję kursora dla kolejnego komunikatu
//				getyx(win_chat, cur_y, cur_x);

				// po obsłudze bufora wyczyść go
				kbd_buf.clear();
				kbd_buf_pos = 0;
				kbd_buf_max = 0;

			}	// else if(key_code == '\n' && kbd_buf.size() > 0)

			// kody ASCII (oraz rozszerzone) wczytaj do bufora (te z zakresu 32...255), jednocześnie ogranicz pojemność bufora wejściowego
			else if(key_code >= 32 && key_code <= 255 && kbd_buf_max < 256)
			{
				kbd_buf.insert(kbd_buf_pos, kbd_utf2iso(key_code));	// wstaw do bufora klawiatury odczytany znak
											// i gdy to UTF-8, zamień go ISO-8859-2 (chodzi o polskie znaki)
				++kbd_buf_pos;
				++kbd_buf_max;
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

			++ix;

		}	// if(FD_ISSET(0, &readfds_tmp))

		// gniazdo (socket), sprawdzaj tylko, gdy socket jest aktywny
		if(FD_ISSET(ga.socketfd_irc, &readfds_tmp) && ga.socketfd_irc > 0)
		{
			// pobierz odpowiedź z serwera
			if(! irc_recv(ga.socketfd_irc, ga.irc_ok, buffer_irc_recv, msg_scr))
			{
				// w przypadku błędu pokaż, co się stało
				chan_parm[ga.chan_nr]->win_buf += msg_scr;		// dopisz do bufora
				wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
			}

			// zinterpretuj odpowiedź
			irc_parser(buffer_irc_recv, msg_irc, ga.irc_ok, chan_parm, ga.chan_nr);

			// jeśli był PING, odpowiedz PONG
			if(msg_irc.size() > 0)
			{
				if(! irc_send(ga.socketfd_irc, ga.irc_ok, msg_irc, msg_scr))
				{
					// w przypadku błędu pokaż, co się stało
					chan_parm[ga.chan_nr]->win_buf += msg_scr;		// dopisz do bufora
					wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
				}
			}


			wprintw_buffer(ga.win_chat, ga.use_colors, chan_parm[ga.chan_nr]->win_buf);
/*
			// pokaż komunikaty serwera
			if(msg_scr.size() > 0)		// UWAGA! powyżej w PONG też jest msg_scr, zrobić coś z tym!!!!!
			{
				chan_parm[chan_nr]->win_buf += msg_scr;		// dopisz do bufora
				wprintw_buffer(win_chat, use_colors, chan_parm[chan_nr]->win_buf);
			}
*/

			// zachowaj pozycję kursora dla kolejnego komunikatu
//			getyx(win_status, cur_y, cur_x);

			// gdy serwer zakończy połączenie, usuń socketfd_irc z zestawu select(), wyzeruj socket oraz ustaw z powrotem nick
			// w pasku wpisywania na Niezalogowany
			if(! ga.irc_ok)
			{
				FD_CLR(ga.socketfd_irc, &readfds);
				close(ga.socketfd_irc);
				ga.socketfd_irc = 0;
				ga.zuousername = "Niezalogowany";
			}

			++iy;

		}	// if(FD_ISSET(socketfd_irc, &readfds_tmp))

	}	// while(! ga.ucc_quit)

/*
	Kończenie działania programu.
*/
	if(ga.socketfd_irc > 0)	// jeśli podczas zamykania programu gniazdo IRC (socket) jest nadal otwarte (co nie powinno się zdarzyć), zamknij je
	{
		close(ga.socketfd_irc);
	}

	delwin(ga.win_chat);
	endwin();	// zakończ tryb ncurses
	del_all_chan(chan_parm);
	fclose(stdin);

	return 0;
}
