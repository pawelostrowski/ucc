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
	init_pair(pBLUE_WHITE, COLOR_BLUE, COLOR_WHITE);

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
	Funkcja zwraca lokalny czas w postaci \n\x03\x08[HH:MM:SS] (ze spacją na końcu), gdzie \x03 to umowny kod formatowania koloru, a \x08 to przyjęty
	w "ucc_global.hpp" kod koloru terminala.
*/

	std::stringstream time_hms_tmp;	// tymczasowo do przeniesienie *char do std::string
	char time_hms[25];

	time_t time_g;		// czas skoordynowany z Greenwich
	struct tm *time_l;	// czas lokalny

	time(&time_g);
	time_l = localtime(&time_g);
	strftime(time_hms, 20, "\n"xTERMC"[%H:%M:%S] ", time_l);

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


void wprintw_buffer(WINDOW *win_chat, bool use_colors, std::string &buffer_str)
{
	int term_y, term_x;	// to nie są te same zmienne co w "main_window.cpp", tam dotyczą one stdscr, a tutaj okna "wirtualnego"
	int cur_y, cur_x;

	size_t pos_buf_start;
	int rows = 1;

	// zacznij od początku okna "wirtualnego"
	wmove(win_chat, 0, 0);

	// pobierz wymiary terminala (tutaj chodzi o Y)
	getmaxyx(win_chat, term_y, term_x);

	// wykryj początek, od którego należy zacząć wyświetlać zawartość bufora (na podstawie kodu \n)
	pos_buf_start = buffer_str.rfind("\n");

	if(pos_buf_start != std::string::npos)
	{
		// zakończ szukanie, gdy pozycja zejdzie do zera lub liczba znalezionych wierszy zrówna się z liczbą wierszy okna "wirtualnego"
		while(pos_buf_start != std::string::npos && pos_buf_start > 0 && rows < term_y)
		{
			pos_buf_start = buffer_str.rfind("\n", pos_buf_start - 1);	// - 1, aby pominąć kod \n
			++rows;
		}
	}

	// jeśli nie wykryto żadnego kodu \n lub na początku bufora, ustal początek na 0 (taka sytuacja może mieć miejsce, gdy nie całe okno "wirtualne"
	// jest zapełnione)
	if(pos_buf_start == std::string::npos)
	{
		pos_buf_start = 0;
	}

	// jeśli na początku wyświetlanej części bufora jest kod \n, pomiń go, aby nie tworzyć pustego wiersza
	if(buffer_str[pos_buf_start] == '\n')
	{
		++pos_buf_start;
	}

	// wypisywanie w pętli
	for(int i = static_cast<int>(pos_buf_start); i < static_cast<int>(buffer_str.size()); ++i)
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
			if(i + 1 < static_cast<int>(buffer_str.size()))
			{
				++i;	// przejdź na kod koloru
				wattron_color(win_chat, use_colors, static_cast<short>(buffer_str[i]));
			}

			continue;	// kodu koloru nie wyświetlaj jako ASCII, rozpocznij od początku pętlę wyświetlającą
		}

		// wykryj pogrubienie tekstu (kod \x04 włącza pogrubienie)
		if(buffer_str[i] == '\x04')
		{
			wattron(win_chat, A_BOLD);
			continue;
		}

		// wykryj pogrubienie tekstu (kod \x5 wyłącza pogrubienie)
		if(buffer_str[i] == '\x05')
		{
			wattroff(win_chat, A_BOLD);
			continue;
		}

		// wykryj odwrócenie kolorów (kod \x11 włącza odwrócenie kolorów)
		if(buffer_str[i] == '\x11')
		{
			wattron(win_chat, A_REVERSE);
			continue;
		}

		// wykryj odwrócenie kolorów (kod \x12 wyłącza odwrócenie kolorów)
		if(buffer_str[i] == '\x12')
		{
			wattroff(win_chat, A_REVERSE);
			continue;
		}

		// wykryj podkreślenie tekstu (kod \x13 włącza podkreślenie)
		if(buffer_str[i] == '\x13')
		{
			wattron(win_chat, A_UNDERLINE);
			continue;
		}

		// wykryj podkreślenie tekstu (kod \x14 wyłącza podkreślenie)
		if(buffer_str[i] == '\x14')
		{
			wattroff(win_chat, A_UNDERLINE);
			continue;
		}

		// jeśli jest kod \n, wyczyść pozostałą część wiersza (czasami pojawiają się śmieci)
		getyx(win_chat, cur_y, cur_x);		// zachowaj pozycję kursora
		clrtoeol();
		wmove(win_chat, cur_y, cur_x);		// przywróć pozycję kursora

		// wyświetl aktualną zawartość bufora dla pozycji w 'i'
		wprintw(win_chat, "%c", buffer_str[i]);
	}

	// pobierz aktualną pozycję kursora (tutaj chodzi o Y), aby wyczyścić potem do końca ekran i nie wyjść poza niego
	getyx(win_chat, cur_y, cur_x);

	// wyczyść pozostałą część ekranu
	while(cur_y < term_y)
	{
		wclrtoeol(win_chat);
		++cur_y;
		wmove(win_chat, cur_y, 0);
	}

	// odśwież okno (w takiej kolejności, czyli najpierw główne, aby wszystko wyświetliło się prawidłowo)
	refresh();
	wrefresh(win_chat);
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
	static int kbd_buf_len = term_x;
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
	if(static_cast<int>(kbd_buf.size()) < kbd_buf_len && x > 0 && static_cast<int>(kbd_buf.size()) - kbd_buf_pos == kbd_buf_rest)
	{
		--x;
	}

	// zachowaj rozmiar bufora dla wyżej wymienionego sprawdzania
	kbd_buf_len = kbd_buf.size();

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

	// wykryj formatowanie fontów, kolorów i emotek, a następnie odpowiednio je skonwertuj
	for(int i = 0; i < static_cast<int>(buffer_irc_recv.size()); ++i)
	{
		// znak % rozpoczyna formatowanie
		if(buffer_irc_recv[i] == '%')
		{
			j = i;		// zachowaj punkt wycięcia przy dokonaniu konwersji
			++i;		// kolejny znak

			// wykryj fonty, jednocześnie sprawdzając, czy nie koniec bufora
			if(i < static_cast<int>(buffer_irc_recv.size()) && buffer_irc_recv[i] == 'F')
			{
				++i;

				// wykryj bold, jednocześnie sprawdzając, czy nie koniec bufora
				if(i < static_cast<int>(buffer_irc_recv.size()) && buffer_irc_recv[i] == 'b')
				{
					for(++i; i < static_cast<int>(buffer_irc_recv.size()); ++i)
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
					for(++i; i < static_cast<int>(buffer_irc_recv.size()); ++i)
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
			else if(i < static_cast<int>(buffer_irc_recv.size()) && buffer_irc_recv[i] == 'C')
			{
				std::string onet_color;

				// wczytaj kolor
				for(++i; i < static_cast<int>(buffer_irc_recv.size()); ++i)
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
			else if(i < static_cast<int>(buffer_irc_recv.size()) && buffer_irc_recv[i] == 'I')
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
