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

	// jeśli się da, dopasuj kolory do ustawień terminala
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
	init_pair(pTERMC, font_color, background_color);
	init_pair(pWHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
	init_pair(pCYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
	init_pair(pMAGENTA_BLUE, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(pYELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);

	return true;
}


void wattron_color(WINDOW *win_chat, bool use_colors, short color_p)
{
	if(use_colors)
	{
		wattron(win_chat, COLOR_PAIR(color_p));	// wattrset() nadpisuje atrybuty, wattron() dodaje atrybuty do istniejących
	}

	else
	{
		wattron(win_chat, A_NORMAL);
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

	time_t time_g = std::stoi("0" + time_unixtimestamp);
	struct tm *time_l;	// czas lokalny

	time_l = localtime(&time_g);
	strftime(time_hms, 20, xNORMAL "[%H:%M:%S] ", time_l);

	return std::string(time_hms);
}


std::string time_unixtimestamp2local_full(std::string &time_unixtimestamp)
{
	char time_date[100];

	time_t time_date_g = std::stoi("0" + time_unixtimestamp);
	struct tm *time_date_l;	// czas lokalny

	time_date_l = localtime(&time_date_g);
	strftime(time_date, 95, "%A, %-d %B %Y, %H:%M:%S", time_date_l);	// %-d, aby nie było nieznaczącego zera w dniu miesiąca

	return std::string(time_date);
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
			attrset(A_REVERSE);	// odwróć kolor tła
			printw("t");
			attrset(A_NORMAL);	// przywróć normalne atrybuty
			continue;		// kod tabulatora wyświetlono jako t z odwróconymi kolorami, więc nie idź dalej, tylko zacznij od początku
		}

		// wyświetl aktualną zawartość bufora dla pozycji w 'i'
		printw("%c", kbd_buf[i]);
	}

	// pozostałe znaki w wierszu wykasuj
	clrtoeol();

	// ustaw kursor w obecnie przetwarzany znak (+ długość nicka, nawias i spacja oraz uwzględnij ewentualny ukryty tekst z lewej w x)
	move(term_y - 1, kbd_buf_pos + zuousername.size() + 3 - x);

	// odśwież główne (standardowe) okno, aby od razu pokazać zmiany na pasku
	refresh();
}


void win_buf_common(struct global_args &ga, std::string &win_buf, int pos_win_buf_start)
{
	int clr_y, clr_x;
	int win_buf_len = win_buf.size();

	// wypisywanie w pętli
	for(int i = pos_win_buf_start; i < win_buf_len; ++i)
	{
		// wykryj formatowanie kolorów w buforze (kod xCOLOR informuje, że mamy kolor, następny bajt to kod koloru)
		if(win_buf[i] == dCOLOR)
		{
			// nie czytaj poza bufor
			if(i + 1 < win_buf_len)
			{
				++i;	// przejdź na kod koloru
				wattron_color(ga.win_chat, ga.use_colors, win_buf[i]);
			}
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

		else
		{
			// jeśli jest kod \n, wyczyść pozostałą część wiersza (czasami pojawiają się śmieci podczas przełączania buforów)
			if(win_buf[i] == '\n')
			{
				getyx(ga.win_chat, clr_y, clr_x);		// zachowaj pozycję kursora
				clrtoeol();
				wmove(ga.win_chat, clr_y, clr_x);		// przywróć pozycję kursora
			}

			// pomiń kod \r, który powoduje, że w ncurses znika tekst (przynajmniej na Linuksie, na Windowsie nie sprawdzałem)
			if(win_buf[i] != '\r')
			{
				// wyświetl aktualną zawartość bufora dla pozycji w 'i'
				wprintw(ga.win_chat, "%c", win_buf[i]);
			}
		}

	}	// for()

	// zapamiętaj po wyjściu z funkcji pozycję kursora w oknie "wirtualnym"
	getyx(ga.win_chat, ga.wcur_y, ga.wcur_x);
}


void win_buf_refresh(struct global_args &ga, std::string &win_buf)
{
/*
	Odśwież zawartość okna danego kanału.
*/

	int wterm_y, wterm_x;		// wymiary okna "wirtualnego"
	int clr_y, clr_x;		// pozycja kursora używana podczas "czyszczenia" ekranu

	size_t pos_win_buf_start;
	int rows = 1;

	// zacznij od początku okna "wirtualnego"
	wmove(ga.win_chat, 0, 0);

	// pobierz wymiary okna "wirtualnego" (tutaj chodzi o Y)
	getmaxyx(ga.win_chat, wterm_y, wterm_x);

	// wykryj początek, od którego należy zacząć wyświetlać zawartość bufora (na podstawie kodu \n)
	pos_win_buf_start = win_buf.rfind("\n");

	if(pos_win_buf_start != std::string::npos)
	{
		// zakończ szukanie, gdy pozycja zejdzie do zera lub liczba znalezionych wierszy zrówna się z liczbą wierszy okna "wirtualnego"
		while(pos_win_buf_start != std::string::npos && pos_win_buf_start > 0 && rows < wterm_y)
		{
			pos_win_buf_start = win_buf.rfind("\n", pos_win_buf_start - 1);	// - 1, aby pominąć kod \n
			++rows;
		}
	}

	// jeśli nie wykryto żadnego kodu \n lub nie wykryto go na początku bufora (np. przy pustym buforze), ustal początek wyświetlania na 0
	if(pos_win_buf_start == std::string::npos)
	{
		pos_win_buf_start = 0;
	}

	// jeśli na początku wyświetlanej części bufora jest kod \n, pomiń go, aby nie tworzyć pustego wiersza
	if(win_buf[pos_win_buf_start] == '\n')
	{
		++pos_win_buf_start;
	}

	// wyświetl ustaloną część bufora
	win_buf_common(ga, win_buf, pos_win_buf_start);

	// wyczyść pozostałą część ekranu
	getyx(ga.win_chat, clr_y, clr_x);	// potrzebna jest pozycja Y, aby wiedzieć, kiedy koniec okna "wirtualnego"
	while(clr_y < wterm_y)
	{
		wclrtoeol(ga.win_chat);
		++clr_y;
		wmove(ga.win_chat, clr_y, 0);
	}

	// odśwież okna (w takiej kolejności, aby wszystko wyświetliło się prawidłowo)
	refresh();
	wrefresh(ga.win_chat);
	refresh();
}


void win_buf_add_str(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string buffer_str, bool add_time)
{
/*
	Dodaj string do bufora danego kanału oraz wyświetl jego zawartość (jeśli to aktywny pokój) wraz z dodaniem przed wyrażeniem aktualnego czasu
	(domyślnie).
*/

	int which_chan = CHAN_STATUS;		// kanał, do którego należy dopisać bufor (domyślnie, gdy nie znaleziony, wpisz do "Status"

	// znajdź kanał, do którego należy dopisać bufor
	for(int i = 1; i < CHAN_MAX; ++i)
	{
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			which_chan = i;		// wpisz numer znalezionego kanału
			break;
		}
	}

	// początek bufora pomocniczego nie powinien zawierać kodu \n, więc aby wstawianie czasu w poniższej pętli zadziałało prawidłowo dla początku,
	// trzeba go wstawić na początku bufora pomocniczego (+ 1 załatwia sprawę, w kolejnych obiegach + 1 wstawia czas za kodem \n)
	size_t buffer_str_n_pos = -1;

	// jeśli trzeba, wstaw czas na początki każdego wiersza (opcja domyślna)
	if(add_time)
	{
		// pobierz rozmiar zajmowany przez wyświetlenie czasu, potrzebne przy szukaniu kodu \n
		int time_len = get_time().size();

		// poniższa pętla wstawia czas za każdym kodem \n (poza początkiem, gdzie go nie ma i wstawia na początku bufora pomocniczego)
		do
		{
			buffer_str.insert(buffer_str_n_pos + 1, get_time());	// wstaw czas na początku każdego wiersza

			buffer_str_n_pos = buffer_str.find("\n", buffer_str_n_pos + time_len + 1);	// kolejny raz szukaj za czasem i kodem \n

		} while(buffer_str_n_pos != std::string::npos);
	}

	// ze względu na przyjęty sposób trzymania danych w buforze, na końcu bufora głównego danego kanału nie ma kodu \n, więc aby przejść do nowej
	// linii, należy na początku bufora pomocniczego dodać kod \n
	if(buffer_str.size() > 0 && buffer_str[0] != '\n')	// na wszelki wypadek sprawdź, czy na początku nie ma już kodu \n
	{
		buffer_str.insert(0, "\n");
	}

	// dodaj zawartość bufora pomocniczego do bufora głównego znalezionego kanału
	if(chan_parm[which_chan])
	{
		chan_parm[which_chan]->win_buf += buffer_str;
	}

	// sprawdź, czy wyświetlić otrzymaną część bufora (tylko gdy aktualny kanał jest tym, do którego wpisujemy)
	if(ga.current_chan != which_chan)
	{
		return;
	}

	// ustal kursor na pozycji, gdzie jest koniec aktualnego tekstu
	wmove(ga.win_chat, ga.wcur_y, ga.wcur_x);

	// jeśli kursor jest na początku okna "wirtualnego", usuń kod \n, aby nie tworzyć pustego wiersza (kod ten już został dopisany do bufora głównego
	// danego kanału)
	if(ga.wcur_y == 0 && ga.wcur_x == 0 && buffer_str.size() > 0 && buffer_str[0] == '\n')
	{
		buffer_str.erase(0, 1);
	}

	// wyświetl otrzymaną część bufora
	win_buf_common(ga, buffer_str, 0);

	// odśwież okna (w takiej kolejności, aby wszystko wyświetliło się prawidłowo)
	refresh();
	wrefresh(ga.win_chat);
	refresh();
}


void chan_act_add(struct channel_irc *chan_parm[], std::string chan_name, int act_type)
{
/*
	Ustaw aktywność danego typu (1...3) dla danego kanału, która zostanie wyświetlona później na pasku dolnym.
*/

	for(int i = 0; i < CHAN_MAX - 1; ++i)	// - 1, bo w kanale "Debug" nie będzie wyświetlania aktywności
	{
		// znajdź kanał, którego dotyczy włączenie aktywności
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			// nie zmieniaj aktywności na "niższą"
			if(chan_parm[i]->chan_act < act_type)
			{
				chan_parm[i]->chan_act = act_type;
			}

			break;
		}
	}
}


void new_chan_status(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_STATUS] == 0)
	{
		ga.current_chan = CHAN_STATUS;		// ustaw nowoutworzony kanał jako aktywny

		chan_parm[CHAN_STATUS] = new channel_irc;
		chan_parm[CHAN_STATUS]->channel = "Status";
		chan_parm[CHAN_STATUS]->channel_ok = false;	// w kanale "Status" nie można pisać tekstu jak w kanale czata
		chan_parm[CHAN_STATUS]->topic = "Ucieszony Chat Client - wersja rozwojowa";	// napis wyświetlany na górnym pasku
		chan_parm[CHAN_STATUS]->chan_act = 0;		// zacznij od braku aktywności kanału
	}
}


void new_chan_debug_irc(struct global_args &ga, struct channel_irc *chan_parm[])
{
	if(chan_parm[CHAN_DEBUG_IRC] == 0)
	{
		chan_parm[CHAN_DEBUG_IRC] = new channel_irc;
		chan_parm[CHAN_DEBUG_IRC]->channel = "Debug";
		chan_parm[CHAN_DEBUG_IRC]->channel_ok = false;	// w kanale "Debug" nie można pisać tekstu jak w kanale czata
		chan_parm[CHAN_DEBUG_IRC]->chan_act = 0;	// zacznij od braku aktywności kanału
	}
}


bool new_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name)
{
/*
	Utwórz nowy kanał czata w programie. Poniższa pętla wyszukuje pierwsze wolne miejsce w tablicy i wtedy tworzy kanał.
*/

	for(int i = 1; i < CHAN_MAX - 1; ++i)	// i = 1 oraz i < CHAN_MAX - 1, bo tutaj nie będą tworzone kanały "Status" oraz "Debug" (tylko kanały czata)
	{
		// nie twórz dwóch kanałów o takiej samej nazwie
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			return true;	// co prawda nie utworzono nowego kanału, ale nie jest to błąd, bo kanał już istnieje
		}

		else if(chan_parm[i] == 0)
		{
			ga.current_chan = i;		// ustaw nowoutworzony kanał jako aktywny

			chan_parm[ga.current_chan] = new channel_irc;
			chan_parm[ga.current_chan]->channel = chan_name;	// nazwa kanału czata
			chan_parm[ga.current_chan]->channel_ok = true;	// w kanałach czata można pisać normalny tekst do wysłania na serwer
			chan_parm[ga.current_chan]->chan_act = 0;	// zacznij od braku aktywności kanału

			// wyczyść okno (by nie było zawartości poprzedniego okna na ekranie)
			wclear(ga.win_chat);

			// ustaw w nim kursor na pozycji początkowej
			ga.wcur_y = 0;
			ga.wcur_x = 0;

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

	for(int i = 1; i < CHAN_MAX - 1; ++i)	// i = 1 oraz i < CHAN_MAX - 1, bo kanały "Status" oraz "Debug" nie mogą zostać usunięte
	{
		// znajdź po nazwie kanału jego numer w tablicy
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			// tymczasowo przełącz na "Status", potem przerobić, aby przechodziło do poprzedniego, który był otwarty
			ga.current_chan = CHAN_STATUS;
			win_buf_refresh(ga, chan_parm[CHAN_STATUS]->win_buf);

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


void destroy_my_password(struct global_args &ga)
{
	for(int i = 0; i < static_cast<int>(ga.my_password.size()); ++i)
	{
		ga.my_password[i] = '*';
	}
}
