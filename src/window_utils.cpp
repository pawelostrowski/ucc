#include <string>		// std::string
#include <ctime>		// czas

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


void win_buf_common(struct global_args &ga, std::string &buffer_str, int pos_buf_str_start)
{
	int clr_y, clr_x;

	int buffer_str_len = buffer_str.size();

	// wypisywanie w pętli
	for(int i = pos_buf_str_start; i < buffer_str_len; ++i)
	{
		// pomiń kod \r, który powoduje, że w ncurses znika tekst (przynajmniej na Linuksie, na Windowsie nie sprawdzałem)
		if(buffer_str[i] == '\r')
		{
			continue;
		}

		// wykryj formatowanie kolorów w buforze (kod \x03 informuje, że mamy kolor, następny bajt to kod koloru)
		if(buffer_str[i] == '\x03')
		{
			// nie czytaj poza bufor
			if(i + 1 < buffer_str_len)
			{
				++i;	// przejdź na kod koloru
				wattron_color(ga.win_chat, ga.use_colors, buffer_str[i]);
			}

			continue;	// kodu koloru nie wyświetlaj jako ASCII, rozpocznij od początku pętlę wyświetlającą
		}

		// wykryj pogrubienie tekstu (kod \x04 włącza pogrubienie)
		if(buffer_str[i] == '\x04')
		{
			wattron(ga.win_chat, A_BOLD);
			continue;
		}

		// wykryj pogrubienie tekstu (kod \x5 wyłącza pogrubienie)
		if(buffer_str[i] == '\x05')
		{
			wattroff(ga.win_chat, A_BOLD);
			continue;
		}

		// wykryj odwrócenie kolorów (kod \x11 włącza odwrócenie kolorów)
		if(buffer_str[i] == '\x11')
		{
			wattron(ga.win_chat, A_REVERSE);
			continue;
		}

		// wykryj odwrócenie kolorów (kod \x12 wyłącza odwrócenie kolorów)
		if(buffer_str[i] == '\x12')
		{
			wattroff(ga.win_chat, A_REVERSE);
			continue;
		}

		// wykryj podkreślenie tekstu (kod \x13 włącza podkreślenie)
		if(buffer_str[i] == '\x13')
		{
			wattron(ga.win_chat, A_UNDERLINE);
			continue;
		}

		// wykryj podkreślenie tekstu (kod \x14 wyłącza podkreślenie)
		if(buffer_str[i] == '\x14')
		{
			wattroff(ga.win_chat, A_UNDERLINE);
			continue;
		}

		// wykryj przywrócenie domyślnych ustawień bez formatowania (kod \x17 przywraca ustawienia domyślne)
		if(buffer_str[i] == '\x17')
		{
			wattrset(ga.win_chat, A_NORMAL);
			continue;
		}

		// jeśli jest kod \n, wyczyść pozostałą część wiersza (czasami pojawiają się śmieci podczas przełączania buforów)
		if(buffer_str[i] == '\n')
		{
			getyx(ga.win_chat, clr_y, clr_x);		// zachowaj pozycję kursora
			clrtoeol();
			wmove(ga.win_chat, clr_y, clr_x);		// przywróć pozycję kursora
		}

		// wyświetl aktualną zawartość bufora dla pozycji w 'i'
		wprintw(ga.win_chat, "%c", buffer_str[i]);
	}

	// zapamiętaj po wyjściu z funkcji pozycję kursora w oknie "wirtualnym"
	getyx(ga.win_chat, ga.wcur_y, ga.wcur_x);
}


void win_buf_refresh(struct global_args &ga, std::string &buffer_str)
{
	int wterm_y, wterm_x;		// wymiary okna "wirtualnego"
	int clr_y, clr_x;		// pozycja kursora używana podczas "czyszczenia" ekranu

	size_t pos_buf_str_start;
	int rows = 1;

	// zacznij od początku okna "wirtualnego"
	wmove(ga.win_chat, 0, 0);

	// pobierz wymiary okna "wirtualnego" (tutaj chodzi o Y)
	getmaxyx(ga.win_chat, wterm_y, wterm_x);

	// wykryj początek, od którego należy zacząć wyświetlać zawartość bufora (na podstawie kodu \n)
	pos_buf_str_start = buffer_str.rfind("\n");

	if(pos_buf_str_start != std::string::npos)
	{
		// zakończ szukanie, gdy pozycja zejdzie do zera lub liczba znalezionych wierszy zrówna się z liczbą wierszy okna "wirtualnego"
		while(pos_buf_str_start != std::string::npos && pos_buf_str_start > 0 && rows < wterm_y)
		{
			pos_buf_str_start = buffer_str.rfind("\n", pos_buf_str_start - 1);	// - 1, aby pominąć kod \n
			++rows;
		}
	}

	// jeśli nie wykryto żadnego kodu \n lub nie wykryto go na początku bufora (np. przy pustym buforze), ustal początek wyświetlania na 0
	if(pos_buf_str_start == std::string::npos)
	{
		pos_buf_str_start = 0;
	}

	// jeśli na początku wyświetlanej części bufora jest kod \n, pomiń go, aby nie tworzyć pustego wiersza
	if(buffer_str[pos_buf_str_start] == '\n')
	{
		++pos_buf_str_start;
	}

	// wyświetl ustaloną część bufora
	win_buf_common(ga, buffer_str, pos_buf_str_start);

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


void add_show_win_buf(struct global_args &ga, struct channel_irc *chan_parm[], std::string buffer_str, bool show_buf)
{
/*
	Dodaj string do bufora oraz wyświetl jego zawartość (jeśli trzeba) wraz z dodaniem przed wyrażeniem aktualnego czasu.
*/

	// początek bufora pomocniczego nie powinien zawierać kodu \n, więc aby wstawianie czasu w poniższej pętli zadziałało prawidłowo dla początku,
	// trzeba go wstawić na początku bufora pomocniczego (+ 1 załatwia sprawę, w kolejnych obiegach + 1 wstawia czas za kodem \n)
	size_t buffer_str_n_pos = -1;

	// pobierz rozmiar zajmowany przez wyświetlenie czasu, potrzebne przy szukaniu kodu \n
	int time_len = get_time().size();

	// poniższa pętla wstawia czas za każdym kodem \n (poza początkiem, gdzie go nie ma i wstawia na początku bufora pomocniczego)
	do
	{
		buffer_str.insert(buffer_str_n_pos + 1, get_time());	// wstaw czas na początku każdego wiersza

		buffer_str_n_pos = buffer_str.find("\n", buffer_str_n_pos + time_len + 1);	// kolejny raz szukaj za czasem i kodem \n

	} while(buffer_str_n_pos != std::string::npos);

	// ze względu na przyjęty sposób trzymania danych w buforze, na końcu bufora głównego danego kanału nie ma kodu \n, więc aby przejść do nowej
	// linii, należy na początku bufora pomocniczego dodać kod \n
	if(buffer_str.size() > 0 && buffer_str[0] != '\n')	// na wszelki wypadek sprawdź, czy na początku nie ma już kodu \n
	{
		buffer_str.insert(0, "\n");
	}

	// dodaj zawartość bufora pomocniczego do bufora głównego danego kanału
	chan_parm[ga.chan_nr]->win_buf += buffer_str;

	// sprawdź, czy wyświetlić otrzymaną część bufora (nie zawsze jest to wymagane)
	if(! show_buf)
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


void add_act_chan(struct channel_irc *chan_parm[], std::string chan_name, int act_type)
{
	for(int i = 0; i < CHAN_MAX - 1; ++i)	// - 1, bo w kanale "Debug" nie będzie wyświetlania aktywności
	{
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


void new_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, bool chan_ok)
{
	// kanał "Debug" zawsze pod ostatnim numerem
	if(chan_name == "Debug")
	{
		// na wszelki wypadek sprawdź, czy kanał nie istnieje jeszcze
		if(chan_parm[CHAN_DEBUG_IRC] == 0)
		{
			ga.chan_nr = CHAN_DEBUG_IRC;	// ustaw nowoutworzony kanał jako aktywny

			chan_parm[CHAN_DEBUG_IRC] = new channel_irc;
			chan_parm[CHAN_DEBUG_IRC]->channel = "Debug";	// nazwa kanału "Debug"
			chan_parm[CHAN_DEBUG_IRC]->channel_ok = false;	// w kanale "Debug" nie można wysyłać wiadomości jak w kanale czata

			// zacznij od braku aktywności kanału
			chan_parm[CHAN_DEBUG_IRC]->chan_act = 0;

			// wyczyść okno
			win_buf_refresh(ga, chan_parm[ga.chan_nr]->win_buf);

			return;		// zakończ
		}
	}

	for(int i = 0; i < CHAN_MAX - 1; ++i)	// - 1, bo tutaj nie będzie tworzony kanał "Debug" (tworzony jest wyżej)
	{
		// nie twórz dwóch kanałów o takiej samej nazwie
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			return;
		}

		if(chan_parm[i] == 0)
		{
			ga.chan_nr = i;		// ustaw nowoutworzony kanał jako aktywny

			chan_parm[ga.chan_nr] = new channel_irc;
			chan_parm[ga.chan_nr]->channel = chan_name;
			chan_parm[ga.chan_nr]->channel_ok = chan_ok;	// true dla kanałów czata, false dla "Status"

			// zacznij od braku aktywności kanału
			chan_parm[ga.chan_nr]->chan_act = 0;

			// wyczyść okno
			win_buf_refresh(ga, chan_parm[ga.chan_nr]->win_buf);

			return;		// gdy utworzono kanał, przerwij szukanie wolnego kanału
		}
	}
}


void del_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// nie można usunąć kanału "Status" oraz "Debug"
	if(ga.chan_nr == CHAN_STATUS || ga.chan_nr == CHAN_DEBUG_IRC)
	{
		return;
	}

	// jeśli kanał nie istnieje, zakończ
	if(chan_parm[ga.chan_nr] == 0)
	{
		return;
	}

	int current_chan = ga.chan_nr;	// zapamiętaj, który kanał usunąć

	// tymczasowo przełącz na "Status", potem przerobić, aby przechodziło do poprzedniego, który był otwarty
	ga.chan_nr = CHAN_STATUS;
	win_buf_refresh(ga, chan_parm[CHAN_STATUS]->win_buf);

	// usuń kanał, który był przed zmianą na "Status"
	delete chan_parm[current_chan];

	// wyzeruj go w tablicy, w ten sposób wiadomo, czy istnieje
	chan_parm[current_chan] = 0;
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


void add_show_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string buffer_str)
{
/*
	Na podstawie tego, jaki kanał był w RAW, wrzuć go do odpowiedniego bufora kanału lub do kanału "Status", jeśli kanał nie istnieje lub RAW
	nie zawiera kanału.
*/

	bool chan_found = false;	// przyjmij, że nie znaleziono szukanego kanału

	int current_chan = ga.chan_nr;	// zachowaj aktualny kanał

	for(int i = 1; i < CHAN_MAX; ++i)
	{
		if(chan_parm[i] && chan_parm[i]->channel == chan_name)
		{
			// jeśli to aktywny kanał, pokaż wpisany bufor
			if(i == ga.chan_nr)
			{
				ga.chan_nr = i;		// zmień kanał, aby do niego wpisać bufor
				add_show_win_buf(ga, chan_parm, buffer_str);	// gdy znaleziono kanał, wpisz do niego zawartość bufora
			}

			// w przeciwnym razie dodaj tylko bufor
			else
			{
				ga.chan_nr = i;		// zmień kanał, aby do niego wpisać bufor
				add_show_win_buf(ga, chan_parm, buffer_str, false);
			}

			ga.chan_nr = current_chan;	// po wpisaniu przywróć kanał
			chan_found = true;

			break;
		}
	}

	// jeśli nie znaleziono szukanego kanału, wpisz bufor do aktualnego kanału
	if(! chan_found)
	{
		add_show_win_buf(ga, chan_parm, buffer_str);
	}
}


void destroy_my_password(struct global_args &ga)
{
	for(int i = 0; i < static_cast<int>(ga.my_password.size()); ++i)
	{
		ga.my_password[i] = '*';
	}
}
