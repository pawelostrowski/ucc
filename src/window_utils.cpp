#include <string>		// std::string

// -std=gnu++11 - time_t, time(), localtime(), strftime()

#include "window_utils.hpp"
#include "enc_str.hpp"
#include "ucc_global.hpp"


bool check_colors()
{
	// gdy nie da się używać kolorów, używaj monochromatycznego terminala
	if(has_colors() == FALSE)
	{
		return false;
	}

	if(start_color() == ERR)
	{
		return false;
	}

	short font_color = COLOR_WHITE;
	short background_color = COLOR_BLACK;

	// jeśli da się, dopasuj kolory do ustawień terminala
	if(use_default_colors() == OK)
	{
		font_color = -1;
		background_color = -1;
	}

	init_pair(pRED, COLOR_RED, background_color);
	init_pair(pGREEN, COLOR_GREEN, background_color);
	init_pair(pYELLOW, COLOR_YELLOW, background_color);
	init_pair(pBLUE, COLOR_BLUE, background_color);
	init_pair(pMAGENTA, COLOR_MAGENTA, background_color);
	init_pair(pCYAN, COLOR_CYAN, background_color);
	init_pair(pWHITE, COLOR_WHITE, background_color);
	init_pair(pDARK, COLOR_BLACK, background_color);
	init_pair(pTERMC, font_color, background_color);
	init_pair(pWHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
	init_pair(pCYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
	init_pair(pMAGENTA_BLUE, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(pBLACK_BLUE, COLOR_BLACK, COLOR_BLUE);
	init_pair(pYELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);
	init_pair(pBLUE_WHITE, COLOR_BLUE, COLOR_WHITE);

	return true;
}


void wattron_color(WINDOW *win, bool use_colors, short color_p)
{
	if(use_colors)
	{
		wattron(win, COLOR_PAIR(color_p));	// wattrset() nadpisuje atrybuty, wattron() dodaje atrybuty do istniejących
	}

	else
	{
		wattron(win, A_NORMAL);
	}
}


std::string get_time()
{
/*
	Funkcja zwraca lokalny czas w postaci \x17[HH:MM:SS] (ze spacją na końcu), gdzie \x17 to umowny kod przywrócenia domyślnych (normalnych) ustawień
	kolorów, bolda itd. (wyłącza je).
*/

	char time_hms[25];

	time_t time_g;		// czas skoordynowany z Greenwich
	struct tm *time_l;	// czas lokalny

	time(&time_g);
	time_l = localtime(&time_g);
	strftime(time_hms, 20, xNORMAL "[%H:%M:%S] ", time_l);

	return std::string(time_hms);
}


std::string time_unixtimestamp2local(std::string &time_unixtimestamp)
{
	char time_hms[25];

	time_t time_g = std::stol("0" + time_unixtimestamp);
	struct tm *time_l;	// czas lokalny

	time_l = localtime(&time_g);
	strftime(time_hms, 20, xNORMAL "[%H:%M:%S] ", time_l);

	return std::string(time_hms);
}


std::string time_unixtimestamp2local_full(std::string &time_unixtimestamp)
{
	char time_date[50];
	std::string time_date_str;
	size_t month;

	time_t time_date_g = std::stol("0" + time_unixtimestamp);
	struct tm *time_date_l;	// czas lokalny

	time_date_l = localtime(&time_date_g);

	strftime(time_date, 45, "%A, %-d %b %Y, %H:%M:%S", time_date_l);	// %-d, aby nie było nieznaczącego zera w dniu miesiąca

	time_date_str = std::string(time_date);

	month = time_date_str.find("sty");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("sty") - 1);	// - 1, aby nie liczyć kodu NULL
		time_date_str.insert(month, "stycznia");
	}

	month = time_date_str.find("lut");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("lut") - 1);
		time_date_str.insert(month, "lutego");
	}

	month = time_date_str.find("mar");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("mar") - 1);
		time_date_str.insert(month, "marca");
	}

	month = time_date_str.find("kwi");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("kwi") - 1);
		time_date_str.insert(month, "kwietnia");
	}

	month = time_date_str.find("maj");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("maj") - 1);
		time_date_str.insert(month, "maja");
	}

	month = time_date_str.find("cze");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("cze") - 1);
		time_date_str.insert(month, "czerwca");
	}

	month = time_date_str.find("lip");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("lip") - 1);
		time_date_str.insert(month, "lipca");
	}

	month = time_date_str.find("sie");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("sie") - 1);
		time_date_str.insert(month, "sierpnia");
	}

	month = time_date_str.find("wrz");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("wrz") - 1);
		time_date_str.insert(month, "września");
	}

	month = time_date_str.find("paź");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("paź") - 1);
		time_date_str.insert(month, "października");
	}

	month = time_date_str.find("lis");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("lis") - 1);
		time_date_str.insert(month, "listopada");
	}

	month = time_date_str.find("gru");

	if(month != std::string::npos)
	{
		time_date_str.erase(month, sizeof("gru") - 1);
		time_date_str.insert(month, "grudnia");
	}

	return time_date_str;
}


std::string time_sec2time(std::string &sec)
{
	long sec_l = std::stol("0" + sec);
	int d, h, m, s;

	d = sec_l / 86400;
	h = (sec_l / 3600) % 24;
	m = (sec_l / 60) % 60;
	s = sec_l % 60;

	if(d > 0)
	{
		return std::to_string(d) + "d " + std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s";
	}

	else if(h > 0)
	{
		return std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s";
	}

	else if(m > 0)
	{
		return std::to_string(m) + "m " + std::to_string(s) + "s";
	}

	else
	{
		return std::to_string(s) + "s";
	}
}


std::string key_utf2iso(int key_code)
{
/*
	Zamień znak (jeden) w UTF-8 na ISO-8859-2.
*/

	int utf_i = 0;

	std::string key_code_tmp;	// tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)

	key_code_tmp = key_code;	// wpisz do bufora tymczasowego pierwszy bajt znaku

	// iloczyn bitowy 0xE0 (11100000b) do wykrycia 0xC0 (11000000b), oznaczającego 2-bajtowy znak w UTF-8
	if((key_code & 0xE0) == 0xC0)
	{
		utf_i = 1;
	}

	// iloczyn bitowy 0xF0 (11110000b) do wykrycia 0xE0 (11100000b), oznaczającego 3-bajtowy znak w UTF-8
	else if((key_code & 0xF0) == 0xE0)
	{
		utf_i = 2;
	}

	// iloczyn bitowy 0xF8 (11111000b) do wykrycia 0xF0 (11110000b), oznaczającego 4-bajtowy znak w UTF-8
	else if((key_code & 0xF8) == 0xF0)
	{
		utf_i = 3;
	}

	// iloczyn bitowy 0xFC (11111100b) do wykrycia 0xF8 (11111000b), oznaczającego 5-bajtowy znak w UTF-8
	else if((key_code & 0xFC) == 0xF8)
	{
		utf_i = 4;
	}

	// iloczyn bitowy 0xFE (11111110b) do wykrycia 0xFC (11111100b), oznaczającego 6-bajtowy znak w UTF-8
	else if((key_code & 0xFE) == 0xFC)
	{
		utf_i = 5;
	}

	// gdy to 1-bajtowy znak nie wymagający konwersji
	else
	{
		return key_code_tmp;	// zwróć kod bez zmian
	}

	// dla znaków wielobajtowych w pętli pobierz resztę bajtów
	for(int i = 0; i < utf_i; ++i)
	{
		key_code_tmp += getch();
	}

	// dokonaj konwersji z UTF-8 na ISO-8859-2
	key_code_tmp = buf_utf2iso(key_code_tmp);

	// gdy przekonwertowany znak nie istnieje (nie ma odpowiednika), co najczęściej da zerową ilość bajtów, zwróć w kodzie ASCII znak zapytania
	// (zwrócenie zerowej liczby bajtów spowodowałoby wywalenie się programu ze względu na pisanie poza bufor klawiatury)
	if(key_code_tmp.size() != 1)
	{
		return "?";
	}

	// natomiast gdy konwersja się udała, zwróć przekonwertowany znak w ISO-8859-2
	return key_code_tmp;
}


void kbd_buf_show(std::string kbd_buf, std::string &zuousername, int term_y, int term_x, int kbd_buf_pos)
{
	// należy zauważyć, że operujemy na kopii bufora klawiatury (brak referencji), bo funkcja ta modyfikuje (kopię) przed wyświetleniem

	static int x = 0;
	static int term_x_len = 0;
	static int kbd_buf_len_prev = term_x;
	static int kbd_buf_rest = 0;
	int cut_left = 0;
	int cut_right = 0;

	// rozsuwaj tekst, gdy terminal jest powiększany w poziomie oraz gdy z lewej strony tekst jest ukryty
	// (dopracować, aby szybka zmiana też poprawnie rozsuwała tekst!!!)
	if(term_x > term_x_len && x > 0)
	{
		--x;
	}

	term_x_len = term_x;

	// Backspace powoduje przesuwanie się tekstu z lewej do kursora, gdy ta część jest niewidoczna
	if(static_cast<int>(kbd_buf.size()) < kbd_buf_len_prev && x > 0 && static_cast<int>(kbd_buf.size()) - kbd_buf_pos == kbd_buf_rest)
	{
		--x;
	}

	// zachowaj rozmiar bufora dla wyżej wymienionego sprawdzania
	kbd_buf_len_prev = kbd_buf.size();

	// zapamiętaj, ile po kursorze zostało w buforze, aby powyższe przesuwanie z lewej do kursora nie działało dla Delete
	kbd_buf_rest = kbd_buf.size() - kbd_buf_pos;

	// gdy kursor cofamy w lewo i jest tam ukryty tekst, odsłaniaj go co jedno przesunięcie
	if(kbd_buf_pos <= x)
	{
		x = kbd_buf_pos;
	}

	// wycinaj ukryty od lewej strony tekst
	if(x > 0)
	{
		kbd_buf.erase(0, x);
	}

	// gdy pozycja kursora przekracza rozmiar terminala, obetnij tekst z lewej (po jednym znaku, bo reszta z x była obcięta wyżej)
	if(kbd_buf_pos - x + static_cast<int>(zuousername.size()) + 4 > term_x)
	{
		cut_left = kbd_buf_pos - x + zuousername.size() + 4 - term_x;
		kbd_buf.erase(0, cut_left);
	}

	// dopisuj, ile ukryto tekstu z lewej strony
	if(cut_left > 0)
	{
		x += cut_left;
	}

	// obetnij to, co wystaje poza terminal
	if(static_cast<int>(kbd_buf.size()) + static_cast<int>(zuousername.size()) + 3 > term_x)
	{
		// jeśli szerokość terminala jest mniejsza od długości nicka wraz z nawiasem i spacją, nic nie obcinaj,
		// bo doprowadzi to do wywalenia się programu
		if(term_x > static_cast<int>(zuousername.size()) + 3)
		{
			cut_right = kbd_buf.size() + zuousername.size() + 3 - term_x;
			kbd_buf.erase((zuousername.size() + 3 - term_x) * (-1), cut_right);
		}
	}

	// konwersja nicka oraz zawartości bufora klawiatury z ISO-8859-2 na UTF-8, aby prawidłowo wyświetlić w terminalu z kodowaniem UTF-8
	kbd_buf.insert(0, "<" + zuousername + "> ");	// dodaj nick
	kbd_buf = buf_iso2utf(kbd_buf);

	// normalne atrybuty fontu
	attrset(A_NORMAL);

	// ustaw kursor na początku ostatniego wiersza
	move(term_y - 1, 0);

	// wyświetl nick (z czata, nie ustawiony przez /nick) oraz zawartość przekonwertowanego bufora (w pętli ze względu na tabulator)
	for(int i = 0; i < static_cast<int>(kbd_buf.size()); ++i)
	{
		// kod tabulatora wyświetl jako t w odwróconych kolorach, aby można było kursorami manipulować we wprowadzanym tekście
		if(kbd_buf[i] == '\t')
		{
			attron(A_REVERSE);	// odwróć kolor tła
			printw("t");
			attroff(A_NORMAL);	// przywróć normalne atrybuty
		}

		// wyświetl aktualną zawartość bufora dla pozycji w 'i'
		else
		{
			printw("%c", kbd_buf[i]);
		}
	}

	// pozostałe znaki w wierszu wykasuj
	clrtoeol();

	// ustaw kursor w obecnie przetwarzany znak (+ długość nicka, nawias i spacja oraz uwzględnij ewentualny ukryty tekst z lewej w x)
	move(term_y - 1, kbd_buf_pos + zuousername.size() + 3 - x);

	// odśwież główne (standardowe) okno, aby od razu pokazać zmiany na pasku
	refresh();
}


void win_buf_show(struct global_args &ga, std::string &win_buf, int pos_win_buf_start)
{
	int wcur_x;
	int win_buf_len = win_buf.size();

	// wypisywanie w pętli
	for(int i = pos_win_buf_start; i < win_buf_len; ++i)
	{
		// pobierz pozycję X kursora, aby przy wyświetlaniu kolejnych znaków wykryć, czy należy pominąć wyświetlanie kodu \n
		wcur_x = getcurx(ga.win_chat);

		// wykryj formatowanie kolorów w buforze (kod dCOLOR informuje, że mamy kolor, następny bajt to kod koloru)
		if(win_buf[i] == dCOLOR && i + 1 < win_buf_len)		// i + 1 < win_buf_len - nie czytaj poza bufor (przy czytaniu kodu koloru)
		{
			++i;	// przejdź na kod koloru
			wattron_color(ga.win_chat, ga.use_colors, static_cast<short>(win_buf[i]));
		}

		// wykryj włączenie pogrubienia tekstu
		else if(win_buf[i] == dBOLD_ON)
		{
			wattron(ga.win_chat, A_BOLD);
		}

		// wykryj wyłączenie pogrubienia tekstu
		else if(win_buf[i] == dBOLD_OFF)
		{
			wattroff(ga.win_chat, A_BOLD);
		}

		// wykryj włączenie odwrócenia kolorów
		else if(win_buf[i] == dREVERSE_ON)
		{
			wattron(ga.win_chat, A_REVERSE);
		}

		// wykryj wyłączenie odwrócenia kolorów
		else if(win_buf[i] == dREVERSE_OFF)
		{
			wattroff(ga.win_chat, A_REVERSE);
		}

		// wykryj włączenie podkreślenia tekstu
		else if(win_buf[i] == dUNDERLINE_ON)
		{
			wattron(ga.win_chat, A_UNDERLINE);
		}

		// wykryj wyłączenie podkreślenia tekstu
		else if(win_buf[i] == dUNDERLINE_OFF)
		{
			wattroff(ga.win_chat, A_UNDERLINE);
		}

		// wykryj przywrócenie domyślnych ustawień bez formatowania
		else if(win_buf[i] == dNORMAL)
		{
			wattrset(ga.win_chat, A_NORMAL);
		}

		// pomiń kod \r, który powoduje, że w ncurses znika tekst (przynajmniej na Linuksie, na Windowsie nie sprawdzałem), wykryj też czy pozycja
		// kursora jest na początku wiersza i jest wtedy kod \n, w takiej sytuacji nie wyświetlaj go, aby nie tworzyć pustego wiersza
		else if(win_buf[i] != '\r' && (wcur_x != 0 || win_buf[i] != '\n'))
		{
			// wyświetl aktualną zawartość bufora dla pozycji w 'i'
			wprintw(ga.win_chat, "%c", win_buf[i]);
		}
	}

	// zapamiętaj pozycję kursora w oknie "wirtualnym"
	getyx(ga.win_chat, ga.wcur_y, ga.wcur_x);

	// odśwież okna (w takiej kolejności, aby wszystko wyświetliło się prawidłowo)
	refresh();
	wrefresh(ga.win_chat);
	refresh();
}


void win_buf_refresh(struct global_args &ga, struct channel_irc *chan_parm[])
{
/*
	Odśwież zawartość okna aktualnie otwartego pokoju (ga.current).
*/

	int wterm_y;		// wymiar Y okna "wirtualnego"

	size_t pos_win_buf_start;
	int rows = 1;

	// usuń starą zawartość okna, aby uniknąć ewentualnych "śmieci" na ekranie
	wclear(ga.win_chat);

	// zacznij od początku okna "wirtualnego" (co prawda wclear() powinien przenieść kursor na początek okna, ale nie jest to udokumentowane w każdej
	// implementacji ncurses, dlatego na wszelki wypadek bezpieczniej jest przenieść kursor)
	wmove(ga.win_chat, 0, 0);

	// pobierz wymiary okna "wirtualnego" (tutaj chodzi o Y)
	wterm_y = getmaxy(ga.win_chat);

	// wykryj początek, od którego należy zacząć wyświetlać zawartość bufora (na podstawie kodu \n)
	pos_win_buf_start = chan_parm[ga.current]->win_buf.rfind("\n");

	if(pos_win_buf_start != std::string::npos)
	{
		// zakończ szukanie, gdy pozycja zejdzie do zera lub liczba znalezionych wierszy zrówna się z liczbą wierszy okna "wirtualnego"
		while(pos_win_buf_start != std::string::npos && pos_win_buf_start > 0 && rows < wterm_y)
		{
			pos_win_buf_start = chan_parm[ga.current]->win_buf.rfind("\n", pos_win_buf_start - 1);	// - 1, aby pominąć kod \n
			++rows;
		}
	}

	// jeśli nie wykryto żadnego kodu \n lub nie wykryto go na początku bufora (np. przy pustym buforze), ustal początek wyświetlania na 0
	if(pos_win_buf_start == std::string::npos)
	{
		pos_win_buf_start = 0;
	}

	// jeśli na początku wyświetlanej części bufora jest kod \n, pomiń go, aby nie tworzyć pustego wiersza
	if(chan_parm[ga.current]->win_buf[pos_win_buf_start] == '\n')
	{
		++pos_win_buf_start;
	}

	// wyświetl ustaloną część bufora
	win_buf_show(ga, chan_parm[ga.current]->win_buf, pos_win_buf_start);

	ga.nicklist_refresh = true;
}


void win_buf_add_str(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string in_buf, int act_type,
			bool add_time, bool only_chan_normal)
{
/*
	Dodaj string do bufora danego kanału oraz wyświetl jego zawartość (jeśli to aktywny pokój) wraz z dodaniem przed wyrażeniem aktualnego czasu
	(domyślnie).
*/

	int which_chan = -1;		// kanał, do którego należy dopisać bufor (-1 przy braku kanału powoduje wyjście)

	// jeśli komunikat ma być wyświetlony wyłącznie w pokojach normalnych oraz w "Status", a chan_name wskazuje na "Debug" lub "RawUnknown",
	// wyświetl bez sprawdzania komunikat w oknie "Status" (domyślnie sprawdzaj)
	if(only_chan_normal && (chan_name == "Debug" || chan_name == "RawUnknown"))
	{
		which_chan = CHAN_STATUS;

		// zmień domyślną aktywność z 1 na 2, aby zwrócić uwagę, że pojawił się komunikat
		act_type = 2;
	}

	else
	{
		// znajdź numer kanału w tablicy na podstawie jego nazwy
		for(int i = 0; i < CHAN_MAX; ++i)
		{
			if(chan_parm[i] && chan_parm[i]->channel == chan_name)
			{
				which_chan = i;		// wpisz numer znalezionego kanału
				break;
			}
		}

		// jeśli pokój o szukanej nazwie nie istnieje, zakończ
		if(which_chan == -1)
		{
			return;
		}
	}

	// ustaw aktywność danego typu (1...3) dla danego kanału, która zostanie wyświetlona później na pasku dolnym (domyślnie aktywność typu 1)
	if(act_type > chan_parm[which_chan]->chan_act)	// nie zmieniaj aktywności na "niższą"
	{
		chan_parm[which_chan]->chan_act = act_type;
	}

	// jeśli trzeba, wstaw czas na początku każdego wiersza (opcja domyślna)
	if(add_time)
	{
		// początek bufora pomocniczego nie powinien zawierać kodu \n, więc aby wstawianie czasu w poniższej pętli zadziałało prawidłowo dla początku,
		// trzeba go wstawić na początku bufora pomocniczego (+ 1 załatwia sprawę, w kolejnych obiegach + 1 wstawia czas za kodem \n)
		size_t pos_n_in_buf = -1;

		// pobierz rozmiar zajmowany przez wyświetlenie czasu, potrzebne przy szukaniu kodu \n
		int time_len = get_time().size();

		// poniższa pętla wstawia czas za każdym kodem \n (poza początkiem, gdzie go nie ma i wstawia na początku bufora pomocniczego)
		do
		{
			in_buf.insert(pos_n_in_buf + 1, get_time());	// wstaw czas na początku każdego wiersza

			pos_n_in_buf = in_buf.find("\n", pos_n_in_buf + time_len + 1);	// kolejny raz szukaj za czasem i kodem \n

		} while(pos_n_in_buf != std::string::npos);
	}

	// ze względu na przyjęty sposób trzymania danych w buforze, na końcu bufora głównego danego kanału nie ma kodu \n, więc aby przejść do nowego
	// wiersza, należy przed dopisaniem bufora pomocniczego dodać kod \n
	if(in_buf.size() > 0 && in_buf[0] != '\n')	// na wszelki wypadek sprawdź, czy na początku nie ma już kodu \n
	{
		chan_parm[which_chan]->win_buf += "\n" + in_buf;
	}

	// sprawdź, czy wyświetlić otrzymaną część bufora (tylko gdy aktualny kanał jest tym, do którego wpisujemy)
	if(ga.current != which_chan)
	{
		return;
	}

	// przenieś kursor na pozycję, gdzie jest koniec aktualnego tekstu
	wmove(ga.win_chat, ga.wcur_y, ga.wcur_x);

	// jeśli kursor jest na początku okna "wirtualnego", i jest tam kod \n, to go usuń, aby nie tworzyć pustego wiersza
	if(ga.wcur_y == 0 && ga.wcur_x == 0 && in_buf.size() > 0 && in_buf[0] == '\n')
	{
		in_buf.erase(0, 1);
	}

	// jeśli kursor nie jest na początku okna "wirtualnego", dodaj kod \n, aby przejść do nowego wiersza
	else if(ga.wcur_y != 0 || ga.wcur_x != 0)
	{
		in_buf.insert(0, "\n");
	}

	// wyświetl otrzymaną część bufora
	win_buf_show(ga, in_buf, 0);
}


void win_buf_all_chan_msg(struct global_args &ga, struct channel_irc *chan_parm[], std::string msg)
{
/*
	Wyświetl komunikat we wszystkich otwartych pokojach, z wyjątkiem "Debug" i "RawUnknown".
*/

	for(int i = 0; i < CHAN_NORMAL; ++i)
	{
		if(chan_parm[i])
		{
			win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, msg);
		}
	}
}


void nicklist_on(struct global_args &ga)
{
	int term_y, term_x;

	getmaxyx(stdscr, term_y, term_x);

	ga.win_info = newwin(term_y - 3, NICKLIST_WIDTH, 1, term_x - NICKLIST_WIDTH);
	wattron_color(ga.win_info, ga.use_colors, pBLUE);
	wborder(ga.win_info, ACS_VLINE, ' ', ' ', ' ', ACS_VLINE, ' ', ACS_VLINE, ' ');

	refresh();
	wrefresh(ga.win_chat);
	wrefresh(ga.win_info);
	refresh();

	ga.nicklist = true;
}


std::string get_flags_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string nick)
{
	std::string nick_tmp;

	auto it = chan_parm[ga.current]->nick_parm.find(nick);

	if(it != chan_parm[ga.current]->nick_parm.end())
	{
		if(it->second.flags.owner)
		{
			if(! it->second.flags.busy)
			{
				nick_tmp += "`";
			}

			else
			{
				nick_tmp += xWHITE "`";
			}
		}

		else if(it->second.flags.op)	// jeśli był ` to nie pokazuj @
		{
			if(! it->second.flags.busy)
			{
				nick_tmp += "@";
			}

			else
			{
				nick_tmp += xWHITE "@";
			}
		}

		if(it->second.flags.halfop)
		{
			if(! it->second.flags.busy)
			{
				nick_tmp += "%";
			}

			else
			{
				nick_tmp += xWHITE "%";
			}
		}

		if(it->second.flags.moderator)
		{
			if(! it->second.flags.busy)
			{
				nick_tmp += xBOLD_ON xRED "!";
			}

			else
			{
				nick_tmp += xRED "!";
			}
		}

		if(it->second.flags.voice)
		{
			if(! it->second.flags.busy)
			{
				nick_tmp += xBOLD_ON xMAGENTA "+";
			}

			else
			{
				nick_tmp += xMAGENTA "+";
			}
		}

		if(it->second.flags.public_webcam)
		{
			if(! it->second.flags.busy)
			{
				nick_tmp += xBOLD_ON xGREEN "*";
			}

			else
			{
				nick_tmp += xGREEN "*";
			}
		}

		if(it->second.flags.private_webcam)
		{
			// przy braku obsługi kolorów dla odróżnienia gwiazdek prywatna kamerka będzie na odwróconym tle
			if(! ga.use_colors)
			{
				nick_tmp += xREVERSE_ON;
			}

			if(! it->second.flags.busy)
			{
				nick_tmp += xBOLD_ON xRED "*";
			}

			else
			{
				nick_tmp += xRED "*";
			}
		}

		if(! it->second.flags.busy)
		{
			nick_tmp += xNORMAL;
		}

		else
		{
			nick_tmp += xNORMAL xWHITE;
		}
	}

	return nick_tmp;
}


void nicklist_refresh(struct global_args &ga, struct channel_irc *chan_parm[])
{
	int y = 0;
	int wterm_y, wterm_x;
	int wcur_x;
	std::string nicklist, nick_owner, nick_op, nick_halfop, nick_moderator, nick_voice, nick_pub_webcam, nick_priv_webcam, nick_normal;

	// zacznij od wyczyszczenia listy
	wattrset(ga.win_info, A_NORMAL);
	wclear(ga.win_info);

	// narysuj linię z lewej strony od góry do dołu (jeśli jest obsługa kolorów, to na niebiesko)
	wattron_color(ga.win_info, ga.use_colors, pBLUE);
	wborder(ga.win_info, ACS_VLINE, ' ', ' ', ' ', ACS_VLINE, ' ', ACS_VLINE, ' ');

	// pasek na szaro dla napisu "Osoby:"
	wmove(ga.win_info, y, 1);
	wattron_color(ga.win_info, ga.use_colors, pWHITE);
	wattron(ga.win_info, A_REVERSE);
	getmaxyx(ga.win_info, wterm_y, wterm_x);

	for(int i = 1; i < wterm_x; ++i)
	{
		wprintw(ga.win_info, " ");
	}

	// pobierz nicki w kolejności zależnej od uprawnień
	for(auto it = chan_parm[ga.current]->nick_parm.begin(); it != chan_parm[ga.current]->nick_parm.end(); ++it)
	{
		if(it->second.flags.owner)
		{
			nick_owner += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else if(it->second.flags.op)
		{
			nick_op += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else if(it->second.flags.halfop)
		{
			nick_halfop += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else if(it->second.flags.moderator)
		{
			nick_moderator += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else if(it->second.flags.voice)
		{
			nick_voice += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else if(it->second.flags.public_webcam)
		{
			nick_pub_webcam += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else if(it->second.flags.private_webcam)
		{
			nick_priv_webcam += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}

		else
		{
			nick_normal += get_flags_nick(ga, chan_parm, it->first) + it->second.nick + "\n";
		}
	}

	// liczba osób w pokoju
	wmove(ga.win_info, y, 1);
	nicklist = "[Osoby: " + std::to_string(chan_parm[ga.current]->nick_parm.size()) + "]\n";

	// połącz nicki w jedną listę
	nicklist += nick_owner + nick_op + nick_halfop + nick_moderator + nick_voice + nick_pub_webcam + nick_priv_webcam + nick_normal;

	++y;

	// wyświetl listę
	for(int i = 0; i >= 0 && i < static_cast<int>(nicklist.size()) - 1 && y <= wterm_y; ++i)	// - 1, bo bez ostatniego kodu \n z listy
	{
		if(nicklist[i] == '\n')
		{
			// nowy wiersz
			wmove(ga.win_info, y, 1);
			wattrset(ga.win_info, A_NORMAL);	// nowy wiersz przywraca domyślne ustawienia
			++y;
		}

		else
		{
			// wykryj formatowanie kolorów i bolda
			if(nicklist[i] == dCOLOR && i + 1 < static_cast<int>(nicklist.size()))
			{
				++i;	// przejdź na kod koloru
				wattron_color(ga.win_info, ga.use_colors, static_cast<short>(nicklist[i]));
			}

			else if(nicklist[i] == dBOLD_ON)
			{
				wattron(ga.win_info, A_BOLD);
			}

			else if(nicklist[i] == dBOLD_OFF)
			{
				wattroff(ga.win_info, A_BOLD);
			}

			else if(nicklist[i] == dREVERSE_ON)
			{
				wattron(ga.win_info, A_REVERSE);
			}

			else if(nicklist[i] == dREVERSE_OFF)
			{
				wattroff(ga.win_info, A_REVERSE);
			}

			else if(nicklist[i] == dUNDERLINE_ON)
			{
				wattron(ga.win_info, A_UNDERLINE);
			}

			else if(nicklist[i] == dUNDERLINE_OFF)
			{
				wattroff(ga.win_info, A_UNDERLINE);
			}

			else if(nicklist[i] == dNORMAL)
			{
				wattrset(ga.win_info, A_NORMAL);
			}

			else
			{
				wcur_x = getcurx(ga.win_info);

				if(wcur_x < wterm_x - 1)
				{
					wprintw(ga.win_info, "%c", nicklist[i]);
				}

				else
				{
					wprintw(ga.win_info, "→");

					wmove(ga.win_info, y, 1);
					wattrset(ga.win_info, A_NORMAL);
					++y;

					for(++i; i < static_cast<int>(nicklist.size()) - 1; ++i)
					{
						if(nicklist[i] == '\n')
						{
							break;
						}
					}
				}
			}
		}
	}

	refresh();
	wrefresh(ga.win_chat);
	wrefresh(ga.win_info);
	refresh();

	ga.nicklist_refresh = false;
}


void nicklist_off(struct global_args &ga)
{
	delwin(ga.win_info);

	ga.nicklist = false;
}


void new_chan_status(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_STATUS] == 0)
	{
		ga.current = CHAN_STATUS;		// ustaw nowoutworzony kanał jako aktywny

		chan_parm[CHAN_STATUS] = new channel_irc;
		chan_parm[CHAN_STATUS]->channel = "Status";
		chan_parm[CHAN_STATUS]->topic = UCC_NAME " " UCC_VER;	// napis wyświetlany na górnym pasku
		chan_parm[CHAN_STATUS]->chan_act = 0;		// zacznij od braku aktywności kanału

		chan_parm[CHAN_STATUS]->win_scroll = -1;	// ciągłe przesuwanie aktualnego tekstu

		// ustaw w nim kursor na pozycji początkowej (to pierwszy tworzony pokój, więc zawsze należy zacząć od pozycji początkowej)
		ga.wcur_y = 0;
		ga.wcur_x = 0;
	}
}


void new_chan_debug_irc(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_DEBUG_IRC] == 0)
	{
		chan_parm[CHAN_DEBUG_IRC] = new channel_irc;
		chan_parm[CHAN_DEBUG_IRC]->channel = "Debug";
		chan_parm[CHAN_DEBUG_IRC]->topic = "Debug";
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
		chan_parm[CHAN_RAW_UNKNOWN]->topic = "RawUnknown";
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
			return true;	// co prawda nie utworzono nowego kanału, ale nie jest to błąd, bo kanał już istnieje
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

			return true;	// gdy utworzono kanał, przerwij szukanie wolnego kanału z kodem sukcesu
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


void new_or_update_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &chan_name, std::string nick, std::string zuo)
{
	// w kluczu trzymaj nick zapisany wielkimi literami (w celu poprawienia sortowania zapewnianego przez std::map)
	std::string nick_key = nick;

	for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
	{
		if(islower(nick_key[i]))
		{
			nick_key[i] = toupper(nick_key[i]);
		}
	}

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// znajdź kanał, którego dotyczy dodanie nicka
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			// jeśli nick istniał, posiadał ZUO na liście i nie podano nowego ZUO, to nie nadpisuj ZUO
			if(zuo.size() == 0)
			{
				auto it = chan_parm[i]->nick_parm.find(nick_key);

				if(it != chan_parm[i]->nick_parm.end() && it->second.zuo.size() > 0)
				{
					zuo = it->second.zuo;
				}
			}

			chan_parm[i]->nick_parm[nick_key] = {nick, zuo};

			break;		// po odnalezieniu pokoju przerwij pętlę
		}
	}
}


void update_nick_flags_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &chan_name, std::string nick, struct nick_flags flags)
{
	std::string nick_key = nick;

	for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
	{
		if(islower(nick_key[i]))
		{
			nick_key[i] = toupper(nick_key[i]);
		}
	}

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			auto it = chan_parm[i]->nick_parm.find(nick_key);

			if(it != chan_parm[i]->nick_parm.end())
			{
				it->second.flags = flags;
			}

			break;		// po odnalezieniu pokoju przerwij pętlę
		}
	}
}


void del_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string nick)
{
	std::string nick_key = nick;

	for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
	{
		if(islower(nick_key[i]))
		{
			nick_key[i] = toupper(nick_key[i]);
		}
	}

	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		// znajdź kanał, którego dotyczy usunięcie nicka
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			chan_parm[i]->nick_parm.erase(nick_key);

			break;		// po odnalezieniu pokoju przerwij pętlę
		}
	}
}


void erase_passwd_nick(std::string &kbd_buf, std::string &hist_buf, std::string &hist_ignore)
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


void destroy_my_password(struct global_args &ga)
{
	for(int i = 0; i < static_cast<int>(ga.my_password.size()); ++i)
	{
		ga.my_password[i] = rand();
	}
}
