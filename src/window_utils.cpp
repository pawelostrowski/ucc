#include <sstream>		// std::string, std::stringstream
#include <cstring>		// memcpy(), strlen()
#include <ctime>		// czas
#include <iconv.h>		// konwersja kodowania znaków

#include "window_utils.hpp"
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

	std::stringstream time_hms_tmp;	// tymczasowo do przeniesienie *char do std::string
	char time_hms[25];

	time_t time_g;		// czas skoordynowany z Greenwich
	struct tm *time_l;	// czas lokalny

	time(&time_g);
	time_l = localtime(&time_g);
	strftime(time_hms, 20, xNORMAL "[%H:%M:%S] ", time_l);

	time_hms_tmp << time_hms;
	return time_hms_tmp.str();
}


std::string buf_utf2iso(std::string &buffer_str)
{
	char c_in[buffer_str.size() + 1];	// + 1 dla NULL
	char c_out[buffer_str.size() + 1];	// + 1 dla NULL

	memcpy(c_in, buffer_str.c_str(), buffer_str.size());
	c_in[buffer_str.size()] = '\x00';

	char *c_in_ptr = c_in;
	size_t c_in_len = buffer_str.size() + 1;
	char *c_out_ptr = c_out;
	size_t c_out_len = buffer_str.size() + 1;

	iconv_t cd = iconv_open("ISO-8859-2", "UTF-8");
	iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
	*c_out_ptr = '\x00';
	iconv_close(cd);

	return std::string(c_out);
}


std::string buf_iso2utf(std::string &buffer_str)
{
	char c_in[buffer_str.size() + 1];	// + 1 dla NULL
	char c_out[buffer_str.size() * 6 + 1];	// przyjęto najgorszy możliwy przypadek, gdzie są same 6-bajtowe znaki po konwersji w UTF-8 + kod NULL

	memcpy(c_in, buffer_str.c_str(), buffer_str.size());
	c_in[buffer_str.size()] = '\x00';

	char *c_in_ptr = c_in;
	size_t c_in_len = buffer_str.size() + 1;
	char *c_out_ptr = c_out;
	size_t c_out_len = buffer_str.size() * 6 + 1;

	iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");
	iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
	*c_out_ptr = '\x00';
	iconv_close(cd);

	return std::string(c_out);
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

	// odśwież okno (w takiej kolejności, czyli najpierw główne, aby wszystko wyświetliło się prawidłowo)
	refresh();
	wrefresh(ga.win_chat);
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

	// odśwież okno (w takiej kolejności, czyli najpierw główne, aby wszystko wyświetliło się prawidłowo)
	refresh();
	wrefresh(ga.win_chat);
}


std::string kbd_utf2iso(int key_code)
{
/*
	Zamień znak (jeden) w UTF-8 na ISO-8859-2.
	UWAGA - funkcja nie działa dla znaków więcej, niż 2-bajtowych oraz nie wykrywa nieprawidłowo wprowadzonych znaków!
*/

	std::string key_code_tmp;	// tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)

	int det_utf = key_code & 0xE0;		// iloczyn bitowy 11100000b do wykrycia 0xC0, oznaczającego znak w UTF-8

	if(det_utf != 0xC0)		// wykrycie 0xC0 oznacza, że mamy znak UTF-8 dwubajtowy
	{
		key_code_tmp = key_code;
		return key_code_tmp;	// jeśli to nie UTF-8, wróć bez zmian we wprowadzonym kodzie
	}

	char c_in[5];
	char c_out[5];

	// wpisz bajt wejściowy oraz drugi bajt znaku
	c_in[0] = key_code;
	c_in[1] = getch();
	c_in[2] = '\x00';		// NULL na końcu

	// dokonaj konwersji znaku (dwubajtowego) z UTF-8 na ISO-8859-2
	char *c_in_ptr = c_in;
	size_t c_in_len = strlen(c_in) + 1;
	char *c_out_ptr = c_out;
	size_t c_out_len = sizeof(c_out);

	iconv_t cd = iconv_open("ISO-8859-2", "UTF-8");
	iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
	iconv_close(cd);

	// po konwersji zakłada się, że znak w ISO-8859-2 ma jeden bajt (brak sprawdzania poprawności wprowadzanych znaków), zwróć ten znak
	key_code_tmp = c_out[0];
	return key_code_tmp;
}


void kbd_buf_show(std::string kbd_buf, std::string &zuousername, int term_y, int term_x, int kbd_buf_pos)
{
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

	// konwersja nicka oraz zawartości bufora klawiatury z ISO-8859-2 na UTF-8
	char c_in[kbd_buf.size() + zuousername.size() + 1 + 3];	// bufor + nick (+ 1 na NULL, + 3, bo nick objęty jest nawiasem oraz spacją za nawiasem)
	char c_out[(kbd_buf.size() + zuousername.size()) * 6 + 1 + 3];	// przyjęto najgorszy możliwy przypadek, gdzie są same 6-bajtowe znaki

	c_in[0] = '<';		// początek nawiasu przed nickiem
	memcpy(c_in + 1, zuousername.c_str(), zuousername.size());	// dopisz nick z czata
	c_in[zuousername.size() + 1] = '>';	// koniec nawiasu
	c_in[zuousername.size() + 2] = ' ';	// spacja za nawiasem
	memcpy(c_in + zuousername.size() + 3, kbd_buf.c_str(), kbd_buf.size());		// dopisz bufor klawiatury
	c_in[kbd_buf.size() + zuousername.size() + 3] = '\x00';		// NULL na końcu

	char *c_in_ptr = c_in;
	size_t c_in_len = kbd_buf.size() + zuousername.size() + 1 + 3;
	char *c_out_ptr = c_out;
	size_t c_out_len = (kbd_buf.size() + zuousername.size()) * 6 + 1 + 3;

	iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");
	iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
	*c_out_ptr = '\x00';
	iconv_close(cd);

	// normalne atrybuty fontu
	attrset(A_NORMAL);

	// ustaw kursor na początku ostatniego wiersza
	move(term_y - 1, 0);

	// wyświetl nick (z czata, nie ustawiony przez /nick) oraz zawartość przekonwertowanego bufora (w pętli ze względu na tabulator)
	for(int i = 0; i < static_cast<int>(strlen(c_out)); ++i)
	{
		// kod tabulatora wyświetl jako t o odwróconych kolorach, aby można było kursorami manipulować we wprowadzanym tekście
		if(c_out[i] == '\t')
		{
			attrset(A_REVERSE);	// odwróć kolor tła
			printw("t");
			attrset(A_NORMAL);	// przywróć normalne atrybuty
			continue;		// kod tabulatora wyświetlono jako t z odwróconymi kolorami, więc nie idź dalej, tylko zacznij od początku
		}

		// wyświetl aktualną zawartość bufora dla pozycji w 'i'
		printw("%c", c_out[i]);
	}

	// pozostałe znaki w wierszu wykasuj
	clrtoeol();

	// ustaw kursor w obecnie przetwarzany znak (+ długość nicka, nawias i spacja oraz uwzględnij ewentualny ukryty tekst z lewej w x)
	move(term_y - 1, kbd_buf_pos + zuousername.size() + 3 - x);

	// odśwież główne (standardowe) okno, aby od razu pokazać zmiany na pasku
	refresh();
}


std::string form_from_chat(std::string &buffer_irc_recv)
{
	int j;

	int buffer_irc_recv_len = buffer_irc_recv.size();

	// wykryj formatowanie fontów, kolorów i emotek, a następnie odpowiednio je skonwertuj
	for(int i = 0; i < buffer_irc_recv_len; ++i)
	{
		// znak % rozpoczyna formatowanie
		if(buffer_irc_recv[i] == '%')
		{
			j = i;		// zachowaj punkt wycięcia przy dokonaniu konwersji
			++i;		// kolejny znak

			// wykryj fonty, jednocześnie sprawdzając, czy nie koniec bufora
			if(i < buffer_irc_recv_len && buffer_irc_recv[i] == 'F')
			{
				++i;

				// wykryj bold, jednocześnie sprawdzając, czy nie koniec bufora
				if(i < buffer_irc_recv_len && buffer_irc_recv[i] == 'b')
				{
					for(++i; i < buffer_irc_recv_len; ++i)
					{
						// spacja wewnątrz formatowania przerywa przetwarzanie
						if(buffer_irc_recv[i] == ' ')
						{
							break;
						}

						// znak % kończy formatowanie, trzeba teraz wyciąć tę część
						else if(buffer_irc_recv[i] == '%')
						{
							buffer_irc_recv.insert(j, xBOLD_ON);	// dodaj kod włączenia bolda

							size_t b_off = buffer_irc_recv.find("\n", i);
							if(b_off != std::string::npos)
							{
								buffer_irc_recv.insert(b_off, xBOLD_OFF);
							}

							// wytnij z bufora kod formatujący %Fb[...]%
							buffer_irc_recv.erase(j + 1, i - j + 1);	// + 1: pomiń kod bolda, kolejny + 1: wytnij drugi %

							// wycięto część z bufora, trzeba wyrównać koniec licznika
							i -= i - j;

							break;
						}
					}
				}

				// jeśli nie bold, to trzeba wyciąć (jeśli są) 'i' lub nazwę fontu
				else
				{
					for(++i; i < buffer_irc_recv_len; ++i)
					{
						// spacja wewnątrz formatowania przerywa przetwarzanie
						if(buffer_irc_recv[i] == ' ')
						{
							break;
						}

						// znak % kończy formatowanie, trzeba teraz wyciąć tę część
						else if(buffer_irc_recv[i] == '%')
						{
							buffer_irc_recv.erase(j, i - j + 1);	// + 1, aby wyciąć do drugiego %
							i -= i - j + 1;

							// gdy to nie bold, wyłącz go
							buffer_irc_recv.insert(i + 1, xBOLD_OFF);

							break;
						}
					}
				}

			}

			// wykryj kolory, jednocześnie sprawdzając, czy nie koniec bufora
			else if(i < buffer_irc_recv_len && buffer_irc_recv[i] == 'C')
			{
				std::string onet_color;

				// wczytaj kolor
				for(++i; i < buffer_irc_recv_len; ++i)
				{
					// spacja wewnątrz formatowania przerywa przetwarzanie
					if(buffer_irc_recv[i] == ' ')
					{
						break;
					}

					if(buffer_irc_recv[i] == '%' && onet_color.size() == 6)
					{
						buffer_irc_recv.erase(i - 8, 9);
						buffer_irc_recv.insert(i - 8, onet_color_conv(onet_color));
						i -= 9;

						break;		// przerwij, aby nie czytać dalej kolorów, tylko zacznij od nowa
					}

					onet_color += buffer_irc_recv[i];
				}
			}

			// wykryj emotki, jednocześnie sprawdzając, czy nie koniec bufora
			else if(i < buffer_irc_recv_len && buffer_irc_recv[i] == 'I')
			{

			}
		}
	}

	return buffer_irc_recv;
}


std::string onet_color_conv(std::string &onet_color)
{
/*
	Ze względu na to, że terminal w podstawowej wersji kolorów nie obsługuje kolorów, jakie są na czacie, trzeba było niektóre "oszukać" i wybrać
	inne, które są podobne (mało eleganckie, ale w tej sytuacji inaczej nie można, zanim nie zostanie dodana obsługa xterm-256).
*/

	if(onet_color == "623c00")		// brązowy
	{
		return xYELLOW;
	}

	else if(onet_color == "c86c00")		// ciemny pomarańczowy
	{
		return xYELLOW;
	}

	else if(onet_color == "ff6500")		// pomarańczowy
	{
		return xYELLOW;
	}

	else if(onet_color == "ff0000")		// czerwony
	{
		return xRED;
	}

	else if(onet_color == "e40f0f")		// ciemniejszy czerwony
	{
		return xRED;
	}

	else if(onet_color == "990033")		// bordowy
	{
		return xRED;
	}

	else if(onet_color == "8800ab")		// fioletowy
	{
		return xMAGENTA;
	}

	else if(onet_color == "ce00ff")		// magenta
	{
		return xMAGENTA;
	}

	else if(onet_color == "0f2ab1")		// granatowy
	{
		return xBLUE;
	}

	else if(onet_color == "3030ce")		// ciemny niebieski
	{
		return xBLUE;
	}

	else if(onet_color == "006699")		// cyjan
	{
		return xCYAN;
	}

	else if(onet_color == "1a866e")		// zielono-cyjanowy
	{
		return xCYAN;
	}

	else if(onet_color == "008100")		// zielony
	{
		return xGREEN;
	}

	else if(onet_color == "959595")		// szary
	{
		return xWHITE;
	}

	return xTERMC;			// gdy żaden z wymienionych
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
		}

		return;		// zakończ
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
