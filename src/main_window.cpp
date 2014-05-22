#include <string>		// std::string, setlocale()
#include <cerrno>		// errno
#include <sys/select.h>		// select()
#include <unistd.h>		// close() - socket

#include "main_window.hpp"
#include "window_utils.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "auth.hpp"
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
	std::string topic_bar;		// bufor pomocniczy tematu, aby nie wyjść poza szerokość terminala, gdy temat jest za długi
	int top_excess;
	bool chan_act_ok = false;
	short act_color;
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

	ga.zuousername = "Niezalogowany";

	ga.chan_nr = CHAN_STATUS;	// zacznij od kanału "Status" (zerowy element w tablicy kanałów)

	ga.wcur_y = 0;
	ga.wcur_x = 0;
/*
	Koniec ustalania globalnych zmiennych.
*/

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
	ga.win_chat = newwin(term_y - 3, term_x, 1, 0);
	scrollok(ga.win_chat, TRUE);		// włącz przewijanie w tym oknie

	// tablica kanałów
	struct channel_irc *chan_parm[CHAN_MAX] = {};		// wyzeruj od razu tablicę

	// kanał "Status" zawsze pod numerem 0 w tablicy (bo jest tworzony jako pierwszy, a kanały są tworzone w taki sposób, że wybierany jest najniższy
	// wolny indeks) i zawsze istnieje w programie (nie mylić z włączaniem go kombinacją Alt+1)
	new_chan(ga, chan_parm, "Status", false);

	// Na górnym pasku wyświetl napis
	chan_parm[CHAN_STATUS]->topic = "Ucieszony Chat Client - wersja rozwojowa";

	// wpisz do bufora "Status" komunikat startowy w kolorze zielonym oraz cyjan (kolor będzie wtedy, gdy terminal obsługuje kolory) i go wyświetl
	add_show_win_buf(ga, chan_parm,	xGREEN "# Aby zalogować się na nick tymczasowy, wpisz:\n"
					xCYAN  "/nick nazwa_nicka\n"
					xCYAN  "/connect\n"
					xGREEN "# Następnie przepisz kod z obrazka, w tym celu wpisz:\n"
					xCYAN  "/captcha kod_z_obrazka\n"
					xGREEN "# Aby zalogować się na nick stały (zarejestrowany), wpisz:\n"
					xCYAN  "/nick nazwa_nicka hasło_do_nicka\n"
					xCYAN  "/connect\n"
					xGREEN "# Aby zobaczyć dostępne polecenia, wpisz:\n"
					xCYAN  "/help\n"
					xGREEN "# Aby zakończyć działanie programu, wpisz:\n"
					xCYAN  "/quit lub /q");		// ze względu na przyjętą budowę bufora na końcu nie ma \n

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

			// po zmianie wielkości okna terminala należy uaktualnić jego zawartość
			win_buf_refresh(ga, chan_parm[ga.chan_nr]->win_buf);
		}

		// paski (jeśli terminal obsługuje kolory, paski będą niebieskie)
		wattron_color(stdscr, ga.use_colors, pWHITE_BLUE);

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

		// temat pokoju (jeśli to kanał czata, dopisz "Temat: "
		topic_bar.clear();
		if(chan_parm[ga.chan_nr]->channel_ok)
		{
			topic_bar = "Temat: ";
		}

		// przygotuj cały bufor
		topic_bar += chan_parm[ga.chan_nr]->topic;

		// wyświetl, uwzględniając szerokość terminala (wyświetl tyle, ile się zmieści)
		top_excess = 0;
		for(int i = 0; i < static_cast<int>(topic_bar.size()) && i < term_x + top_excess; ++i)
		{
			// wykryj znaki wielobajtowe w UTF-8 (konkretnie 2B), aby zniwelować szerokość wyświetlania
			if((topic_bar[i] & 0xe0) == 0xc0)
			{
				++top_excess;
			}

			printw("%c", topic_bar[i]);
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

		// nr kanału i jego nazwa
		printw("[%d:%s] ", ga.chan_nr + 1, chan_parm[ga.chan_nr]->channel.c_str());	// + 1, bo kanały od 1 a nie od 0

		// w aktywnym pokoju skasuj flagi aktywności
		chan_parm[ga.chan_nr]->chan_act = 0;

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

				// gdy była aktywność, wyświetl wynik
				if(chan_parm[i]->chan_act > 0 && chan_parm[i]->chan_act <= 3)
				{
					if(! chan_act_ok)
					{
						printw("[Aktywność: ");

						// numer pokoju w kolorze
						wattron_color(stdscr, ga.use_colors, act_color);

						if(act_color == pMAGENTA_BLUE || act_color == pWHITE_BLUE)
						{
							wattron(stdscr, A_BOLD);
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
							wattron(stdscr, A_BOLD);
						}

						printw("%d", i + 1);	// + 1, bo kanały od 1 a nie od 0
					}

					// przywróć domyślny kolor paska bez bolda
					wattron_color(stdscr, ga.use_colors, pWHITE_BLUE);
					wattroff(stdscr, A_BOLD);
				}
			}
		}

		if(chan_act_ok)
		{
			printw("] ");
		}

		chan_act_ok = false;
/*
	Koniec informacji na pasku dolnym.
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
				// lewy Alt generuje też kod klawisza, z którym został wciśnięty (dla poniższych sprawdzeń), dlatego pobierz ten kod
				key_code = getch();

				if(key_code >= '1' && key_code <= '9')
				{
					if(chan_parm[key_code - '1'])	// jeśli kanał istnieje, wybierz go (- '1', aby zamienić na cyfry 0x00...0x08)
					{
						ga.chan_nr = key_code - '1';
						win_buf_refresh(ga, chan_parm[ga.chan_nr]->win_buf);
					}
				}

				else if(key_code == '0')	// 0 jest traktowane jak 10
				{
					if(chan_parm[9])	// to nie pomyłka, że 9, bo numery są od 0
					{
						ga.chan_nr = 9;
						win_buf_refresh(ga, chan_parm[9]->win_buf);
					}
				}

				else if(key_code == 'd' && ucc_dbg_irc)
				{
					//ga.chan_nr = CHAN_DEBUG_IRC;	// debugowanie w ostatnim kanale pod kombinacją Alt+d
					//win_buf_refresh(ga, chan_parm[CHAN_DEBUG_IRC]->win_buf);
				}
			}

			// Alt Left + Arrow Left
			else if(key_code == 0x21e)
			{
				for(int i = 0; i < CHAN_MAX; ++i)
				{
					--ga.chan_nr;

					if(ga.chan_nr < 0)
					{
						ga.chan_nr = CHAN_MAX - 1;
					}

					if(chan_parm[ga.chan_nr])
					{
						win_buf_refresh(ga, chan_parm[ga.chan_nr]->win_buf);
						break;
					}
				}
			}

			// Alt Left + Arrow Right
			else if(key_code == 0x22d)
			{
				for(int i = 0; i < CHAN_MAX; ++i)
				{
					++ga.chan_nr;

					if(ga.chan_nr == CHAN_MAX)
					{
						ga.chan_nr = 0;
					}

					if(chan_parm[ga.chan_nr])
					{
						win_buf_refresh(ga, chan_parm[ga.chan_nr]->win_buf);
						break;
					}
				}
			}

			// Enter (0x0A) - wykonaj obsługę bufora klawiatury, ale tylko wtedy, gdy coś w nim jest
			else if(key_code == '\n' && kbd_buf.size() > 0)
			{
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

				// po obsłudze bufora wyczyść go
				kbd_buf.clear();
				kbd_buf_pos = 0;
				kbd_buf_max = 0;
			}

			// kody ASCII (oraz rozszerzone) wczytaj do bufora (te z zakresu 32...255), jednocześnie ogranicz pojemność bufora wejściowego
			else if(key_code >= 32 && key_code <= 255 && kbd_buf_max < KBD_BUF_MAX_SIZE)
			{
				// wstaw do bufora klawiatury odczytany znak i gdy to UTF-8, zamień go na ISO-8859-2 (chodzi o polskie znaki)
				kbd_buf.insert(kbd_buf_pos, kbd_utf2iso(key_code));
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

		}	// if(FD_ISSET(0, &readfds_tmp))

		// gniazdo (socket), sprawdzaj tylko, gdy socket jest aktywny
		if(FD_ISSET(ga.socketfd_irc, &readfds_tmp) && ga.socketfd_irc > 0)
		{
			// pobierz dane z serwera oraz zinterpretuj odpowiedź (obsłuż otrzymane dane)
			irc_parser(ga, chan_parm);

			// gdy serwer zakończy połączenie, usuń socketfd_irc z zestawu select(), wyzeruj socket oraz ustaw z powrotem nick
			// w pasku wpisywania na Niezalogowany
			if(! ga.irc_ok)
			{
				FD_CLR(ga.socketfd_irc, &readfds);
				close(ga.socketfd_irc);
				ga.socketfd_irc = 0;
				ga.zuousername = "Niezalogowany";
			}
		}

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
	destroy_my_password(ga);
	fclose(stdin);

	return 0;
}
