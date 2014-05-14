#include <string>		// std::string, setlocale()
#include <cerrno>		// errno
#include <sys/select.h>		// select()

#include "main_window.hpp"
#include "window_utils.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "auth.hpp"
#include "network.hpp"
#include "ucc_global.hpp"


int main_window(bool use_colors, bool ucc_dbg_irc)
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
	bool ucc_quit = false;		// aby zakończyć program, zmienna ta musi mieć wartość prawdziwą

	bool irc_auth_status;		// status wykonania którejś z funkcji irc_auth_x()
	bool captcha_ready = false;	// stan wczytania captcha (jego pobranie z serwera)
	bool irc_ready = false;		// gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
	bool irc_ok = false;		// stan połączenia z czatem
	bool channel_ok = false;	// stan wejścia do pokoju (kanału)

	int term_y, term_x;		// wymiary terminala
//	int cur_y, cur_x;		// aktualna pozycja kursora

	int kbd_buf_pos = 0;		// początkowa pozycja bufora klawiatury (istotne podczas używania strzałek, Home, End, Delete itd.)
	int kbd_buf_max = 0;		// początkowy maksymalny rozmiar bufora klawiatury
	int key_code;			// kod ostatnio wciśniętego klawisza
	std::string kbd_buf;		// bufor odczytanych znaków z klawiatury

	int socketfd_irc = 0;		// gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne
	std::string buffer_irc_recv;	// bufor odebranych danych z IRC
	std::string buffer_irc_sent;	// dane wysłane do serwera w irc_auth_x() (informacje przydatne do debugowania)

	std::string my_nick;
	std::string my_password;
	std::string cookies;
	std::string zuousername = "Niezalogowany";
	std::string uokey;
	std::string channel;

	std::string msg_scr;
	std::string msg_irc;

	channel_irc *chan_list[22];
	chan_list[0] = new channel_irc;
	chan_list[1] = new channel_irc;

	int chan_nr = 0;
/*
	Koniec zmiennych.
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
	// gdy uruchomiliśmy main_window() z use_colors = true, gdy terminal nie obsługuje kolorów, check_colors() zwróci false
	if(use_colors)
	{
		use_colors = check_colors();
	}

	// utwórz okno, w którym będą komunikaty serwera oraz inne (np. diagnostyczne)
	getmaxyx(stdscr, term_y, term_x);	// pobierz wymiary terminala (okna głównego)
	WINDOW *win_chat;
	win_chat = newwin(term_y - 3, term_x, 1, 0);
	scrollok(win_chat, TRUE);		// włącz przewijanie w tym oknie

	// wpisz do bufora komunikat powitalny w kolorze zielonym oraz cyjan wraz z godziną na początku każdego wiersza (kolor będzie wtedy, gdy terminal
	// obsługuje kolory) (.erase(0, 1) - usuń kod \n, na początku nie jest potrzebny)
	chan_list[0]->win_buf_chan =	get_time().erase(0, 1) + xGREEN + xBOLD_ON + "Ucieszony Chat Client" + xBOLD_OFF +
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

	// wyświetl zawartość bufora na ekranie
	wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);

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
	while(! ucc_quit)
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
			wresize(win_chat, term_y - 3, term_x);	// j/w, ale dla okna diagnostycznego

/*
			// po zmianie rozmiaru terminala sprawdź, czy maksymalna pozycja kursora Y nie jest większa od wymiarów okna
			if(cur_y >= term_y - 3)
			{
				cur_y = term_y - 4;		// - 4, bo piszemy do max granicy, nie wchodząc na nią
			}
*/

			// po zmianie wielkości okna terminala należy uaktualnić jego zawartość
			wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
		}

		// paski (jeśli terminal obsługuje kolory, paski będą niebieskie)
		wattron_color(stdscr, use_colors, pBLUE_WHITE);
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

		move(term_y - 2, 0);
		//pokaż aktualny czas
		printw("%s", get_time().erase(0, 3).c_str());	// .erase(0, 3) - usuń kody \n\x3\x8 ze stringa (nowy wiersz i formatowanie kolorów)
		if(channel.size() > 0)
		{
			printw("[%s] ", channel.c_str());
		}
		printw("kbd+irc: %d, kbd: %d, irc: %d, socketfd_irc: %d", ix + iy, ix, iy, socketfd_irc);
/*
		Koniec informacji tymczasowych.
*/

		// wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
		// oraz ustaw kursor w obecnie przetwarzany znak
		kbd_buf_show(kbd_buf, zuousername, term_y, term_x, kbd_buf_pos);

		// czekaj na aktywność klawiatury lub gniazda (socket)
		if(select(socketfd_irc + 1, &readfds_tmp, NULL, NULL, &tv) == -1)
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
				delwin(win_chat);
				endwin();	// zakończ tryb ncurses
				fclose(stdin);
				delete chan_list[0];
				delete chan_list[1];
				return 3;
			}
		}

		// klawiatura
		if(FD_ISSET(0, &readfds_tmp))
		{
			key_code = getch();

			if(key_code == KEY_LEFT)		// Left Arrow
			{
				if(kbd_buf_pos > 0)
				{
					--kbd_buf_pos;
				}
			}

			else if(key_code == KEY_RIGHT)		// Right Arrow
			{
				if(kbd_buf_pos < kbd_buf_max)
				{
					++kbd_buf_pos;
				}
			}

			else if(key_code == KEY_BACKSPACE)	// Backspace
			{
				if(kbd_buf_pos > 0)
				{
					--kbd_buf_pos;
					--kbd_buf_max;
					kbd_buf.erase(kbd_buf_pos, 1);
				}
			}

			else if(key_code == KEY_DC)		// Delete
			{
				if(kbd_buf_pos < kbd_buf_max)
				{
					--kbd_buf_max;
					kbd_buf.erase(kbd_buf_pos, 1);
				}
			}

			else if(key_code == KEY_HOME)		// Home
			{
				kbd_buf_pos = 0;
			}

			else if(key_code == KEY_END)		// End
			{
				kbd_buf_pos = kbd_buf_max;
			}

			else if(key_code == KEY_PPAGE)		// PageUp
			{
				//wscrl(win_chat, 1);
				chan_nr = 1;
				wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);

			}

			else if(key_code == KEY_NPAGE)		// PageDown
			{
				//wscrl(win_chat, -1);
				chan_nr = 0;
				wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
			}

			else if(key_code == '\n' && kbd_buf.size() > 0)		// Enter (0x0A), wykonaj obsługę bufora tylko, gdy coś w nim jest
			{
				// "wyczyść" pole wpisywanego tekstu (aby nie było widać zwłoki, np. podczas pobierania obrazka z kodem do przepisania)
				move(term_y - 1, zuousername.size() + 3);	// ustaw kursor za nickiem i spacją za nawiasem
				clrtoeol();
				refresh();

				// wykonaj obsługę bufora (zidentyfikuj polecenie)
				kbd_parser(kbd_buf, msg_scr, msg_irc, my_nick, my_password, cookies, zuousername, uokey, captcha_ready, irc_ready,
					   irc_ok, channel_ok, channel, ucc_quit);

				// gdy parser zwrócił jakiś komunikat do wyświetlenia, pokaż go
				if(msg_scr.size() > 0)
				{
					chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego) komunikat z parsera
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
				}

				// jeśli w buforze IRC parser zwrócił jakiś komunikat i jeśli połączono z IRC (sprawdzanie nie jest konieczne, ale dodano
				// na wszelki wypadek), wyślij go na serwer
				if(msg_irc.size() > 0 && irc_ok)
				{
					if(! irc_send(socketfd_irc, irc_ok, msg_irc, msg_scr))
					{
						// w przypadku błędu pokaż co się stało
						chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
						wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					}
				}

				// sprawdź gotowość do połączenia z IRC
				if(irc_ready)
				{
					irc_ready = false;      // nie próbuj się znowu łączyć do IRC od zera

					// połącz z serwerem IRC
					irc_auth_status = irc_auth_1(socketfd_irc, irc_ok, buffer_irc_recv, msg_scr);
					// pokaż odpowiedź serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_recv;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// w przypadku błędu pokaż, co się stało
					if(! irc_auth_status)
					{
						chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
						wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					}

					// wyślij: NICK <zuousername>
					irc_auth_status = irc_auth_2(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, zuousername, msg_scr);
					// pokaż, co wysłano do serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_sent;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// pokaż odpowiedź serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_recv;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// w przypadku błędu pokaż, co się stało
					if(! irc_auth_status)
					{
						chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
						wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					}

					// wyślij: AUTHKEY
					irc_auth_status = irc_auth_3(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, msg_scr);
					// pokaż, co wysłano do serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_sent;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// pokaż odpowiedź serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_recv;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// w przypadku błędu pokaż, co się stało
					if(! irc_auth_status)
					{
						chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
						wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					}

					// wyślij: AUTHKEY <AUTHKEY>
					irc_auth_status = irc_auth_4(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, msg_scr);
					// pokaż, co wysłano do serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_sent;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// w przypadku błędu pokaż, co się stało
					if(! irc_auth_status)
					{
						chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
						wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					}

					// wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>\r\nPROTOCTL ONETNAMESX
					irc_auth_status = irc_auth_5(socketfd_irc, irc_ok, buffer_irc_sent, zuousername, uokey, msg_scr);
					// pokaż, co wysłano do serwera
					chan_list[chan_nr]->win_buf_chan += buffer_irc_sent;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					// w przypadku błędu pokaż, co się stało
					if(! irc_auth_status)
					{
						chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
						wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
					}

					// od tej pory, o ile poprawnie połączono się do IRC, można dodać socketfd_irc do zestawu select()
					if(irc_ok)
					{
						FD_SET(socketfd_irc, &readfds);		// gniazdo IRC (socket)
					}

					// gdy połączenie do IRC nie powiedzie się, wyzeruj socket i ustaw z powrotem nick w pasku wpisywania na Niezalogowany
					else
					{
						socketfd_irc = 0;
						zuousername = "Niezalogowany";
					}

				}	// if(ucc_ga->irc_ready)


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

			// kod tabulatora dodaj do bufora
			else if(key_code == '\t')
			{
				kbd_buf.insert(kbd_buf_pos, "\t");
				++kbd_buf_pos;
				++kbd_buf_max;
			}

			++ix;

		}	// if(FD_ISSET(0, &readfds_tmp))

		// gniazdo (socket), sprawdzaj tylko, gdy socket jest aktywny
		if(FD_ISSET(socketfd_irc, &readfds_tmp) && socketfd_irc > 0)
		{
			// pobierz odpowiedź z serwera
			if(! irc_recv(socketfd_irc, irc_ok, buffer_irc_recv, msg_scr))
			{
				// w przypadku błędu pokaż, co się stało
				chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
				wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
			}

			// zinterpretuj odpowiedź
			irc_parser(buffer_irc_recv, msg_scr, msg_irc, channel, irc_ok);

			// jeśli był PING, odpowiedz PONG
			if(msg_irc.size() > 0)
			{
				if(! irc_send(socketfd_irc, irc_ok, msg_irc, msg_scr))
				{
					// w przypadku błędu pokaż, co się stało
					chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
					wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
				}
			}

			// pokaż komunikaty serwera
			if(msg_scr.size() > 0)		// UWAGA! powyżej w PONG też jest msg_scr, zrobić coś z tym!!!!!
			{
				chan_list[chan_nr]->win_buf_chan += msg_scr;	// dopisz do bufora (głównego)
				wprintw_buffer(win_chat, use_colors, chan_list[chan_nr]->win_buf_chan);
			}

			// zachowaj pozycję kursora dla kolejnego komunikatu
//			getyx(win_status, cur_y, cur_x);

			// gdy serwer zakończy połączenie, usuń socketfd_irc z zestawu select(), wyzeruj socket oraz ustaw z powrotem nick
			// w pasku wpisywania na Niezalogowany
			if(! irc_ok)
			{
				FD_CLR(socketfd_irc, &readfds);
				close(socketfd_irc);
				socketfd_irc = 0;
				zuousername = "Niezalogowany";
			}

			++iy;

		}	// if(FD_ISSET(socketfd_irc, &readfds_tmp))

	}	// while(! ucc_quit)

/*
	Kończenie działania programu.
*/
	if(socketfd_irc > 0)	// jeśli podczas zamykania programu gniazdo IRC (socket) jest nadal otwarte (co nie powinno się zdarzyć), zamknij je
	{
		close(socketfd_irc);
	}

	delwin(win_chat);
	endwin();	// zakończ tryb ncurses
	fclose(stdin);

	delete chan_list[0];
	delete chan_list[1];

	return 0;
}
