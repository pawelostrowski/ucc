/*
	Ucieszony Chat Client
	Copyright (C) 2013, 2014 Paweł Ostrowski

	This file is part of Ucieszony Chat Client.

	Ucieszony Chat Client is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	Ucieszony Chat Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Ucieszony Chat Client (in the file LICENSE); if not,
	see <http://www.gnu.org/licenses/gpl-2.0.html>.
*/


#include <sstream>		// std::string, std::stringstream

// -std=c++11 - time_t, time(), localtime(), strftime()

#include "window_utils.hpp"
#include "form_conv.hpp"
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

	short int font_color = COLOR_WHITE;
	short int background_color = COLOR_BLACK;

	// jeśli da się, dopasuj kolory do ustawień terminala
	if(use_default_colors() == OK)
	{
		font_color = -1;
		background_color = -1;
	}

	// kolory podstawowe z tłem zależnym od ustawień terminala
	init_pair(pRED, COLOR_RED, background_color);
	init_pair(pGREEN, COLOR_GREEN, background_color);
	init_pair(pYELLOW, COLOR_YELLOW, background_color);
	init_pair(pBLUE, COLOR_BLUE, background_color);
	init_pair(pMAGENTA, COLOR_MAGENTA, background_color);
	init_pair(pCYAN, COLOR_CYAN, background_color);
	init_pair(pWHITE, COLOR_WHITE, background_color);
	init_pair(pDARK, COLOR_BLACK, background_color);
	init_pair(pTERMC, font_color, background_color);

	// kolory z różnym tłem
	init_pair(pYELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);
	init_pair(pBLACK_BLUE, COLOR_BLACK, COLOR_BLUE);
	init_pair(pMAGENTA_BLUE, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(pCYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
	init_pair(pWHITE_BLUE, COLOR_WHITE, COLOR_BLUE);

	return true;
}


std::string get_time()
{
/*
	Funkcja zwraca lokalny czas w postaci [HH:MM:SS] ze spacją na końcu.
*/

	char time_hms[25];

	time_t time_g;
	struct tm *time_l;

	time(&time_g);			// czas skoordynowany z Greenwich
	time_l = localtime(&time_g);	// czas lokalny
	strftime(time_hms, 20, "[%H:%M:%S] ", time_l);

	return std::string(time_hms);
}


std::string time_utimestamp_to_local(std::string &time_unixtimestamp)
{
	char time_hms[25];

	time_t time_g = std::stol("0" + time_unixtimestamp);
	struct tm *time_l;

	time_l = localtime(&time_g);	// czas lokalny
	strftime(time_hms, 20, "[%H:%M:%S] ", time_l);

	return std::string(time_hms);
}


std::string get_time_full()
{
	time_t time_g;

	time(&time_g);

	std::string time_str = std::to_string(time_g);

	return time_utimestamp_to_local_full(time_str);
}


std::string time_utimestamp_to_local_full(std::string &time_unixtimestamp)
{
	char time_date[50];

	time_t time_date_g = std::stol("0" + time_unixtimestamp);
	struct tm *time_date_l;

	time_date_l = localtime(&time_date_g);	// czas lokalny

#ifdef __CYGWIN__

	// w Cygwin dodanie minusa wewnątrz %d nie działa, na razie data będzie z nieznaczącym zerem
	strftime(time_date, 45, "%A, %d %b %Y, %H:%M:%S", time_date_l);

#else

	// %-d, aby nie było nieznaczącego zera w dniu miesiąca
	strftime(time_date, 45, "%A, %-d %b %Y, %H:%M:%S", time_date_l);

#endif		// __CYGWIN__

	std::string time_date_str = std::string(time_date);

	size_t month = time_date_str.find("sty");

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


std::string time_sec2time(std::string &sec_str)
{
	int64_t sec_long = std::stol("0" + sec_str);

	int d = sec_long / 86400;
	int h = (sec_long / 3600) % 24;
	int m = (sec_long / 60) % 60;
	int s = sec_long % 60;

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


std::string buf_lower_to_upper(std::string buf)
{
	int buf_len = buf.size();

	for(int i = 0; i < buf_len; ++i)
	{
		if(islower(buf[i]))
		{
			buf[i] = toupper(buf[i]);
		}
	}

	return buf;
}


int buf_chars(std::string &in_buf)
{
/*
	Policz liczbę znaków, uwzględniając formatowanie tekstu oraz kodowanie UTF-8.
	Uwaga! Nie jest sprawdzana poprawność zapisu w UTF-8 (procedura zakłada, że znaki zapisane są poprawnie).
*/

	int in_buf_len = in_buf.size();

	char c;
	int count_char = 0;

	for(int l = 0; l < in_buf_len; ++l)
	{
		c = in_buf[l];

		if(c == dCOLOR)
		{
			++l;
		}

		// uwzględnij formatowanie tekstu (nie licz też kodu \r)
		else if(c != dBOLD_ON && c != dBOLD_OFF && c != dREVERSE_ON && c != dREVERSE_OFF && c != dUNDERLINE_ON && c != dUNDERLINE_OFF && c != dNORMAL
			&& c != '\r')
		{
			++count_char;
		}

		// uwzględnij kodowanie UTF-8 (sposób najprostszy, ale mający wadę opisaną na początku, polegający na pominięciu odpowiedniej liczby bajtów)
		if((c & 0xE0) == 0xC0)
		{
			l += 1;
		}

		else if((c & 0xF0) == 0xE0)
		{
			l += 2;
		}

		else if((c & 0xF8) == 0xF0)
		{
			l += 3;
		}

		else if((c & 0xFC) == 0xF8)
		{
			l += 4;
		}

		else if((c & 0xFE) == 0xFC)
		{
			l += 5;
		}
	}

	return count_char;
}


int how_lines(std::string &in_buf, int wterm_x, int time_len)
{
	int in_buf_len = in_buf.size();

	char c;
	int virt_cur = 0;
	bool first_char = true;

	std::string word;
	int line = 1;
	int x;

	for(int l = 0; l < in_buf_len; ++l)
	{
		c = in_buf[l];

		if(c == dCOLOR)
		{
			++l;
		}

		// uwzględnij formatowanie tekstu (nie licz też kodu \r)
		else if(c != dBOLD_ON && c != dBOLD_OFF && c != dREVERSE_ON && c != dREVERSE_OFF && c != dUNDERLINE_ON && c != dUNDERLINE_OFF && c != dNORMAL
			&& c != '\r')
		{
			// wykryj przejście do nowego wiersza, nie licz pierwszej spacji
			if(! first_char && virt_cur % wterm_x == 0 && c != ' ')
			{
				virt_cur = time_len;
				++line;
			}

			first_char = false;

			++virt_cur;

			if(c == ' ')
			{
				word.clear();
				x = 1;

				while(true)
				{
					if(l + x == in_buf_len || in_buf[l + x] == ' ')
					{
						if(virt_cur - wterm_x + buf_chars(word) > 0)
						{
							virt_cur = time_len;
							++line;
						}

						break;
					}

					word += in_buf[l + x];
					++x;
				}
			}
		}

		// uwzględnij kodowanie UTF-8
		if((c & 0xE0) == 0xC0)
		{
			l += 1;
		}

		else if((c & 0xF0) == 0xE0)
		{
			l += 2;
		}

		else if((c & 0xF8) == 0xF0)
		{
			l += 3;
		}

		else if((c & 0xFC) == 0xF8)
		{
			l += 4;
		}

		else if((c & 0xFE) == 0xFC)
		{
			l += 5;
		}
	}

	return line;
}


void kbd_buf_show(std::string &kbd_buf, int term_y, int term_x, int kbd_cur_pos, int &kbd_cur_pos_offset)
{
/*
	Wyświetl zawartość bufora klawiatury na dolnym pasku wpisywania tekstu.
*/

	static int prev_cut_left = 0;
	static int prev_kbd_cur_pos = 0;
	static int prev_kbd_buf_chars = 0;

	int diff_pos;
	int show_start = 0;
	int kbd_buf_len = kbd_buf.size();
	int kbd_buf_chars = buf_chars(kbd_buf);

	char c;
	int count_char = 0;	// licznik znaków używany do sprawdzenia, czy zapisano całą szerokość paska (aby nie pisać poza nim)

	int utf_i;

	// - jeśli wycinamy tekst przez Backspace, cofnij offset, aby tekst cofał się do kursora, a kursor był w miejscu
	// - jeśli przewijamy historię strzałkami i trafimy na tekst krótszy od poprzedniego, to zostanie tak ustawiony offset, że kursor znajdzie się na
	//   końcu (jeśli jest bardzo mały, offset będzie ujemny, ale jest to pilnowane, aby go wyzerować, nowa wartość offsetu, jeśli jest to wymagane,
	//   policzona wtedy zostanie w dalszej części)
	if(prev_cut_left > 0 && kbd_buf_chars < prev_kbd_buf_chars && kbd_cur_pos < prev_kbd_cur_pos)
	{
		prev_cut_left -= prev_kbd_buf_chars - kbd_buf_chars;

		if(prev_cut_left < 0)
		{
			prev_cut_left = 0;
		}
	}

	// zapamiętaj obecną pozycję kursora oraz liczbę znaków w buforze klawiatury, aby przy kolejnym wejściu można było sprawdzić powyższy warunek
	prev_kbd_cur_pos = kbd_cur_pos;
	prev_kbd_buf_chars = kbd_buf_chars;

	// oblicz różnicę liczby znaków między pozycją kursora a szerokością terminala + 1
	diff_pos = kbd_cur_pos - term_x + 1;

	// jeśli różnica jest większa od zera, policz offset (ile znaków trzeba ukryć z lewej strony przed wyświetleniem)
	if(diff_pos > 0 && diff_pos > prev_cut_left)
	{
		prev_cut_left = diff_pos;
	}

	// jeśli cofamy kursor w lewo i zejdzie on do tekstu ukrytego, zmniejszaj offset, aby go pokazać
	else if(kbd_cur_pos < prev_cut_left)
	{
		prev_cut_left -= kbd_cur_pos_offset - kbd_cur_pos;
	}

	// zapamiętaj offset kursora (przesunięcie względem ewentualnego ukrytego tekstu, aby kursor zawsze wskazywał na poprawny znak)
	kbd_cur_pos_offset = prev_cut_left;

	// uwzględnij kodowanie UTF-8
	show_start = prev_cut_left;

	for(int i = 0; i < show_start && i < kbd_buf_len; ++i)
	{
		c = kbd_buf[i];

		if((c & 0xE0) == 0xC0)
		{
			i += 1;
			show_start += 1;
		}

		else if((c & 0xF0) == 0xE0)
		{
			i += 2;
			show_start += 2;
		}

		else if((c & 0xF8) == 0xF0)
		{
			i += 3;
			show_start += 3;
		}

		else if((c & 0xFC) == 0xF8)
		{
			i += 4;
			show_start += 4;
		}

		else if((c & 0xFE) == 0xFC)
		{
			i += 5;
			show_start += 5;
		}
	}

	// normalne atrybuty fontu
	attrset(A_NORMAL);

	// ustaw kursor na początku ostatniego wiersza
	move(term_y - 1, 0);

	// wyświetl wymagany fragment bufora klawiatury
	for(int i = show_start; count_char < term_x && i < kbd_buf_len; ++i)
	{
		c = kbd_buf[i];
		++count_char;

		if(c == '\t')
		{
			attron(A_REVERSE | A_UNDERLINE);
			addch('t');
			attroff(A_REVERSE | A_UNDERLINE);
		}

		else
		{
			// uwzględnij kodowanie UTF-8
			utf_i = 0;

			if((c & 0xE0) == 0xC0)
			{
				utf_i += 1;
			}

			else if((c & 0xF0) == 0xE0)
			{
				utf_i += 2;
			}

			else if((c & 0xF8) == 0xF0)
			{
				utf_i += 3;
			}

			else if((c & 0xFC) == 0xF8)
			{
				utf_i += 4;
			}

			else if((c & 0xFE) == 0xFC)
			{
				utf_i += 5;
			}

			for(int j = 0; j <= utf_i && j < kbd_buf_len; ++j)
			{
				printw("%c", kbd_buf[i + j]);
			}

			i += utf_i;
		}
	}

	// wyczyść pozostałą część paska
	clrtoeol();
}


void win_buf_show(struct global_args &ga, struct channel_irc *ci[])
{
	int win_buf_len = ci[ga.current]->win_buf.size();	// pobierz całkowity rozmiar std::vector
	int line_len;
	int win_buf_start = 0;
	int skip_count = 0;
	int line_skip_leading = 0;

	std::string word;
	int x;

	char c;
	int virt_cur = 0;
	bool first_char;
	bool last_char = false;
	bool skip_space = false;

	int utf_i;

	// wyczyść starą zawartość i ustaw kursor na początku okna "wirtualnego" (dla bezpieczeństwa, bo różne implementacje ncurses różnie to traktują)
	werase(ga.win_chat);
	wmove(ga.win_chat, 0, 0);

	// jeśli włączony jest scroll, wczytaj początek wyświetlania
	if(ci[ga.current]->win_scroll_lock)
	{
		win_buf_start = ci[ga.current]->win_pos_first;
		line_skip_leading = ci[ga.current]->win_skip_lead_first;
	}

	// jeśli nie jest włączony scroll okna, wyznacz początek wyświetlania
	else
	{
		// zacznij od końca std::vector
		win_buf_start = win_buf_len;

		while(win_buf_start > 0 && skip_count < ga.wterm_y)
		{
			// policz, ile fizycznie wierszy okna "wirtualnego" zajmuje jedna pozycja w std::vector
			skip_count += how_lines(ci[ga.current]->win_buf[win_buf_start - 1], ga.wterm_x, ga.time_len);

			// pozycja wstecz w std::vector
			--win_buf_start;
		}

		// sprawdź, czy pierwszy wyświetlany wiersz fizycznie zajmuje więcej niż szerokość okna "wirtualnego", jeśli tak, to o ile, aby przy
		// wyświetlaniu wiersza pominąć te "nadmiarowe" wiersze, ale tylko wtedy, gdy naliczona liczba wierszy przekracza wysokość okna "wirtualnego"
		if(win_buf_len > 0 && skip_count > ga.wterm_y && how_lines(ci[ga.current]->win_buf[win_buf_start], ga.wterm_x, ga.time_len) > 1)
		{
			line_skip_leading = skip_count - ga.wterm_y;
		}

		// zapamiętaj pierwszy przetwarzany element w std::vector oraz liczbę ewentualnie pominiętych wierszy początkowych tego elementu
		ci[ga.current]->win_pos_first = win_buf_start;
		ci[ga.current]->win_skip_lead_first = line_skip_leading;
	}

	// wypisywanie w pętli
	// (getcury(ga.win_chat) != wterm_y - 1 || getcurx(ga.win_chat) == 0) - wyświetlaj, dopóki nie osiągnięto częściowo zapisanego ostatniego wiersza
	for(int w = win_buf_start; w < win_buf_len && (getcury(ga.win_chat) != ga.wterm_y - 1 || getcurx(ga.win_chat) == 0); ++w)
	{
		// zapamiętaj ostatnio przetwarzany element w std::vector
		ci[ga.current]->win_pos_last = w;

		// jeśli wyświetlany element jest jedyny (początek i koniec są sobie równe) oraz są nadmiarowe wiersze do pominięcia, dodaj je do początku
		// (różnica niewyświetlonych do wyświetlonych)
		if(win_buf_start == w && line_skip_leading > 0)
		{
			ci[ga.current]->win_skip_lead_last = line_skip_leading - 1;
		}

		else
		{
			ci[ga.current]->win_skip_lead_last = 0;
		}

		// nowy wiersz, ale tylko wtedy, gdy kursor nie jest na początku wiersza, aby nie tworzyć pustej linii
		if(getcurx(ga.win_chat) != 0)
		{
			waddch(ga.win_chat, '\n');
		}

		// warunki początkowe dla nowej części w std::vector
		first_char = true;

		// pobierz rozmiar aktualnie wyświetlanej części w std::vector
		line_len = ci[ga.current]->win_buf[w].size();

		// wyświetl aktualnie przetwarzaną część z bufora
		// ! last_char_yx - wyświetlaj, dopóki nie osiągnięto ostatniego znaku ostatniego wiersza
		for(int l = 0; l < line_len && ! last_char; ++l)
		{
			// aktualnie przetwarzany znak (lub kod formatowania)
			c = ci[ga.current]->win_buf[w][l];

			// jeśli osiągnięto ostatni znak ostatniego wiersza, zakończ kolejne przejście pętli (o ile to nie kod formatujący lub \r)
			if(getcury(ga.win_chat) == ga.wterm_y - 1 && getcurx(ga.win_chat) == ga.wterm_x - 1 && c != dCOLOR && c != dBOLD_ON
				&& c != dBOLD_OFF && c != dREVERSE_ON && c != dREVERSE_OFF && c != dUNDERLINE_ON && c != dUNDERLINE_OFF && c != dNORMAL
				&& c != '\r')
			{
				last_char = true;
			}

			// wykryj formatowanie kolorów w buforze (kod dCOLOR informuje, że mamy kolor, następny bajt to kod koloru)
			if(c == dCOLOR)
			{
				++l;	// przejdź na kod koloru

				// ustaw kolor (jeśli kolory są obsługiwane)
				if(ga.use_colors && l < line_len)
				{
					wattron(ga.win_chat, COLOR_PAIR(ci[ga.current]->win_buf[w][l]));
				}
			}

			// wykryj włączenie pogrubienia tekstu
			else if(c == dBOLD_ON)
			{
				wattron(ga.win_chat, A_BOLD);
			}

			// wykryj wyłączenie pogrubienia tekstu
			else if(c == dBOLD_OFF)
			{
				wattroff(ga.win_chat, A_BOLD);
			}

			// wykryj włączenie odwrócenia kolorów
			else if(c == dREVERSE_ON)
			{
				wattron(ga.win_chat, A_REVERSE);
			}

			// wykryj wyłączenie odwrócenia kolorów
			else if(c == dREVERSE_OFF)
			{
				wattroff(ga.win_chat, A_REVERSE);
			}

			// wykryj włączenie podkreślenia tekstu
			else if(c == dUNDERLINE_ON)
			{
				wattron(ga.win_chat, A_UNDERLINE);
			}

			// wykryj wyłączenie podkreślenia tekstu
			else if(c == dUNDERLINE_OFF)
			{
				wattroff(ga.win_chat, A_UNDERLINE);
			}

			// wykryj przywrócenie domyślnych ustawień bez formatowania
			else if(c == dNORMAL)
			{
				wattrset(ga.win_chat, A_NORMAL);
			}

			// pomiń kod \r, który powoduje, że w ncurses znika tekst (przynajmniej na Linuksie, na Windowsie nie sprawdzałem)
			else if(c != '\r')
			{
				// uwzględnij kodowanie UTF-8
				utf_i = 0;

				if((c & 0xE0) == 0xC0)
				{
					utf_i += 1;
				}

				else if((c & 0xF0) == 0xE0)
				{
					utf_i += 2;
				}

				else if((c & 0xF8) == 0xF0)
				{
					utf_i += 3;
				}

				else if((c & 0xFC) == 0xF8)
				{
					utf_i += 4;
				}

				else if((c & 0xFE) == 0xFC)
				{
					utf_i += 5;
				}

				if(line_skip_leading > 0)
				{
					// wykryj przejście do nowego wiersza, nie licz pierwszej spacji
					if(! first_char && virt_cur % ga.wterm_x == 0 && c != ' ')
					{
						virt_cur = ga.time_len;
						--line_skip_leading;
					}

					++virt_cur;

					if(c == ' ' && line_skip_leading > 0)
					{
						word.clear();
						x = 1;

						while(true)
						{
							if(l + x == line_len || ci[ga.current]->win_buf[w][l + x] == ' ')
							{
								if(virt_cur - ga.wterm_x + buf_chars(word) > 0)
								{
									virt_cur = ga.time_len;
									--line_skip_leading;
								}

								break;
							}

							word += ci[ga.current]->win_buf[w][l + x];
							++x;
						}
					}
				}

				// wyświetl znak, jeśli nie ma wierszy "nadmiarowych"
				if(line_skip_leading == 0)
				{
					// zawijanie wyrazów
					if(! first_char && c == ' ')
					{
						if(getcurx(ga.win_chat) == 0)
						{
							skip_space = true;
						}

						else
						{
							word.clear();
							x = 1;

							while(true)
							{
								if(l + x == line_len || ci[ga.current]->win_buf[w][l + x] == ' ')
								{
									if(getcurx(ga.win_chat) + buf_chars(word) >= ga.wterm_x)
									{
										waddch(ga.win_chat, '\n');
										skip_space = true;
									}

									break;
								}

								word += ci[ga.current]->win_buf[w][l + x];
								++x;
							}
						}
					}

					// jeśli było zawinięcie wyrazu (świadczy o tym pozycja X kursora na 0), wstaw tyle spacji, ile zajmuje
					// pokazanie czasu, ale tylko wtedy, gdy było już coś wyświetlone
					if(! first_char && getcurx(ga.win_chat) == 0)
					{
						for(int i = ga.time_len; i > 0; --i)
						{
							waddch(ga.win_chat, ' ');
						}

						// dodaj nowy wiersz w danej pozycji
						++ci[ga.current]->win_skip_lead_last;
					}

					// jeśli po przeniesieniu wyrazu osiągnięto ostatni wiersz, nic dalej nie wyświetlaj
					else if(skip_space && getcury(ga.win_chat) == ga.wterm_y - 1)
					{
						last_char = true;
					}

					if(! skip_space)
					{
						for(int i = 0; i <= utf_i && i < line_len; ++i)
						{
							wprintw(ga.win_chat, "%c", ci[ga.current]->win_buf[w][l + i]);
						}
					}

					skip_space = false;
				}

				first_char = false;

				l += utf_i;
			}
		}
	}

	// zaktualizuj okno "wirtualne" (bez odświeżania)
	wnoutrefresh(ga.win_chat);

	// nie odświeżaj ponownie okna "wirtualnego" w pętli głównej
	ga.win_chat_refresh = false;
}


void win_buf_refresh(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_show(ga, ci);

	wnoutrefresh(stdscr);
	doupdate();
}


void win_buf_add_str(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, std::string in_buf, bool save_log, int act_type,
	bool add_time, bool only_chan_normal)
{
/*
	Dodaj string do bufora danego kanału oraz wyświetl jego zawartość (jeśli to aktywny pokój) wraz z dodaniem przed wyrażeniem aktualnego czasu
	(domyślnie), ewentualne odświeżenie okna "wirtualnego" nastąpi w pętli głównej programu.
*/

	// kanał, do którego należy dopisać bufor (-1 przy braku kanału powoduje wypisanie w "Status")
	int which_chan = -1;

	// znajdź numer kanału w tablicy na podstawie jego nazwy
	for(int i = 0; i < CHAN_MAX; ++i)
	{
		if(ci[i] && ci[i]->channel == chan_name)
		{
			// wpisz numer znalezionego kanału
			which_chan = i;

			// po odnalezieniu pokoju przerwij pętlę
			break;
		}
	}

	// jeśli pokój o szukanej nazwie nie istnieje, komunikat wyświetl w "Status" i dodaj aktywność typu 3
	if(which_chan == -1)
	{
		which_chan = CHAN_STATUS;

		// zmień domyślną aktywność z 1 na 3, aby zwrócić uwagę, że pojawił się komunikat
		act_type = 3;
	}

	// jeśli komunikat nie jest specjalnie kierowany do "DebugIRC" lub "RawUnknown" (dla nich wyzerowana jest flaga only_chan_normal podczas używania
	// funkcji win_buf_add_str() (domyślnie jest ustawiona), to komunikat wyświetl w oknie "Status"
	else if(only_chan_normal && (chan_name == "DebugIRC" || chan_name == "RawUnknown"))
	{
		which_chan = CHAN_STATUS;

		// zmień domyślną aktywność z 1 na 2, aby zwrócić uwagę, że pojawił się komunikat
		act_type = 2;
	}

	// ustaw aktywność danego typu (0...3) dla danego kanału, która zostanie wyświetlona później na pasku dolnym (domyślnie aktywność typu 1)
	if(act_type > ci[which_chan]->chan_act)	// nie zmieniaj aktywności na "niższą"
	{
		ci[which_chan]->chan_act = act_type;
	}

	// przy włączonym scrollu ustaw aktywność dla napisu "--Więcej--" na dolnym pasku
	if(ci[which_chan]->win_scroll_lock && act_type > ci[which_chan]->lock_act)	// nie zmieniaj aktywności na "niższą"
	{
		ci[which_chan]->lock_act = act_type;
	}

	std::stringstream in_buf_stream(in_buf);
	std::string in_buf_line;

	// obsłuż bufor wejściowy
	while(std::getline(in_buf_stream, in_buf_line))
	{
		// zamień tabulatory na spacje
		size_t code_tab = in_buf_line.find("\t");

		while(code_tab != std::string::npos)
		{
			in_buf_line.replace(code_tab, 1, " ");
			code_tab = in_buf_line.find("\t", code_tab + 1);
		}

		// na początku każdego wiersza dodaj kod czyszczący formatowanie i, jeśli trzeba, wstaw czas (opcja domyślna)
		ci[which_chan]->win_buf.push_back((add_time ? xNORMAL + get_time() : xNORMAL) + in_buf_line);

		// zapisz log
		if(save_log && ci[which_chan]->chan_log.good())
		{
			ci[which_chan]->chan_log << (add_time ? get_time() : "") << remove_form(in_buf_line) << std::endl;
			ci[which_chan]->chan_log.flush();
		}
	}

	// sprawdź, czy wyświetlić otrzymaną część bufora (tylko gdy aktualny kanał jest tym, do którego wpisujemy, gdy nie użyliśmy scrolla okna
	// lub pobrano kompletny wiersz, ale sprawdzaj to tylko przy połączeniu z IRC), odświeżenie nastąpi w pętli głównej programu
	if(ga.current == which_chan && ! ci[which_chan]->win_scroll_lock && (! ga.irc_ok || ! ga.is_irc_recv_buf_incomplete))
	{
		ga.win_chat_refresh = true;
	}
}


void win_buf_all_chan_msg(struct global_args &ga, struct channel_irc *ci[], std::string msg, bool save_log)
{
/*
	Wyświetl komunikat we wszystkich otwartych pokojach, z wyjątkiem "DebugIRC" i "RawUnknown".
*/

	for(int i = 0; i < CHAN_NORMAL; ++i)
	{
		if(ci[i])
		{
			win_buf_add_str(ga, ci, ci[i]->channel, msg, save_log);
		}
	}
}


std::string get_flags_nick(struct global_args &ga, struct channel_irc *ci[], std::string nick_key)
{
	std::string nick_tmp;

	auto it = ci[ga.current]->ni.find(nick_key);

	if(it != ci[ga.current]->ni.end())
	{
		if(it->second.nf.owner)
		{
			if(it->second.nf.busy)
			{
				nick_tmp = xBOLD_ON xDARK;
			}

			nick_tmp += "`";
		}

		else if(it->second.nf.op)	// jeśli był ` to nie pokazuj @
		{
			if(it->second.nf.busy)
			{
				nick_tmp = xBOLD_ON xDARK;
			}

			nick_tmp += "@";
		}

		else if(it->second.nf.halfop)
		{
			if(it->second.nf.busy)
			{
				nick_tmp = xBOLD_ON xDARK;
			}

			nick_tmp += "%";
		}

		if(it->second.nf.moderator)
		{
			nick_tmp += xNORMAL;	// gdyby było formatowanie z w/w flag, należy się go pozbyć

			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON;
			}

			nick_tmp += xRED "!";
		}

		if(it->second.nf.voice)
		{
			nick_tmp += xNORMAL;

			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON;
			}

			nick_tmp += xMAGENTA "+";
		}

		if(it->second.nf.public_webcam)
		{
			nick_tmp += xNORMAL;

			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON;
			}

			nick_tmp += xGREEN "*";
		}

		else if(it->second.nf.private_webcam)
		{
			nick_tmp += xNORMAL;

			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON;
			}

			// czerwona gwiazdka, a przy braku obsługi kolorów dla odróżnienia prywatna kamerka będzie na odwróconym tle
			nick_tmp += (ga.use_colors ? xRED "*" : xREVERSE_ON "*");
		}

		// ustaw kolor nicka
		nick_tmp += (! it->second.nf.busy ? xNORMAL : xNORMAL xBOLD_ON xDARK);
	}

	return nick_tmp;
}


void nicklist_refresh(struct global_args &ga, struct channel_irc *ci[])
{
	std::map<std::string, std::string> nicklist;
	std::string nick_key, nick_status;

	int nick_max_show = 0;
	int nick_len;
	int nick_max_len = 0;

	char c;

	// zapisz nicki w kolejności zależnej od uprawnień
	for(auto it = ci[ga.current]->ni.begin(); it != ci[ga.current]->ni.end(); ++it)
	{
		if(it->second.nf.owner)
		{
			nick_status = "1";
		}

		else if(it->second.nf.op)
		{
			nick_status = "2";
		}

		else if(it->second.nf.halfop)
		{
			nick_status = "3";
		}

		else if(it->second.nf.moderator)
		{
			nick_status = "4";
		}

		else if(it->second.nf.voice)
		{
			nick_status = "5";
		}

		else if(it->second.nf.public_webcam)
		{
			nick_status = "6";
		}

		else if(it->second.nf.private_webcam)
		{
			nick_status = "7";
		}

		else
		{
			nick_status = "8";
		}

		nicklist[nick_status + it->first] = get_flags_nick(ga, ci, it->first) + it->second.nick;
	}

	// pobierz najdłuższy nick, który wyświetli się w oknie
	for(auto it = nicklist.begin(); it != nicklist.end() && nick_max_show < getmaxy(ga.win_info) - 1; ++it, ++nick_max_show)
	{
		nick_len = buf_chars(it->second);

		// zapisz długość nicka, jeśli jest krótsza od ostatniego, nie zmieniaj wartości (trzeba wykryć najdłuższy nick wraz z jego statusami)
		if(nick_len > nick_max_len)
		{
			nick_max_len = nick_len;
		}
	}

	// jeśli trzeba, zmień rozmiary okien
	if(nick_max_len >= NICKLIST_WIDTH_MIN && ga.win_info_current_width == NICKLIST_WIDTH_MIN)
	{
		delwin(ga.win_info);

		wresize(ga.win_chat, getmaxy(stdscr) - 3, getmaxx(stdscr) - NICKLIST_WIDTH_MAX);

		getmaxyx(ga.win_chat, ga.wterm_y, ga.wterm_x);

		ga.win_info = newwin(getmaxy(stdscr) - 3, NICKLIST_WIDTH_MAX, 1, getmaxx(stdscr) - NICKLIST_WIDTH_MAX);

		leaveok(ga.win_info, TRUE);

		ga.win_info_current_width = NICKLIST_WIDTH_MAX;

		ga.win_chat_refresh = true;
	}

	else if(nick_max_len < NICKLIST_WIDTH_MIN && ga.win_info_current_width == NICKLIST_WIDTH_MAX)
	{
		delwin(ga.win_info);

		wresize(ga.win_chat, getmaxy(stdscr) - 3, getmaxx(stdscr) - NICKLIST_WIDTH_MIN);

		getmaxyx(ga.win_chat, ga.wterm_y, ga.wterm_x);

		ga.win_info = newwin(getmaxy(stdscr) - 3, NICKLIST_WIDTH_MIN, 1, getmaxx(stdscr) - NICKLIST_WIDTH_MIN);

		leaveok(ga.win_info, TRUE);

		ga.win_info_current_width = NICKLIST_WIDTH_MIN;

		ga.win_chat_refresh = true;
	}

	// wyczyść listę
	werase(ga.win_info);

	// narysuj linię z lewej strony od góry do dołu (jeśli jest obsługa kolorów, to na niebiesko)
	ga.use_colors ? wattrset(ga.win_info, COLOR_PAIR(pBLUE)) : wattrset(ga.win_info, A_REVERSE);

	wborder(ga.win_info, ACS_VLINE, ' ', ' ', ' ', ACS_VLINE, ' ', ACS_VLINE, ' ');

	// atrybuty paska z napisem "Użytkownicy"
	if(ga.use_colors)
	{
		wattron(ga.win_info, COLOR_PAIR(pWHITE));
	}

	wattron(ga.win_info, A_REVERSE);
	wmove(ga.win_info, 0, 1);

	for(int i = 1; i < getmaxx(ga.win_info); ++i)
	{
		waddch(ga.win_info, ' ');
	}

	std::string users_header = "Użytkownicy (" + std::to_string(ci[ga.current]->ni.size()) + ")";
	int users_header_len = buf_chars(users_header);

	mvwprintw(ga.win_info, 0, ((getmaxx(ga.win_info) - users_header_len) + 1) / 2, users_header.c_str());

	// wyświetl nicki
	for(auto it = nicklist.begin(); it != nicklist.end() && getcury(ga.win_info) < getmaxy(ga.win_info) - 1; ++it)
	{
		// nowy wiersz (o ile poprzedni napis nie spowodował przejścia do nowego wiersza, jeśli tak, dodaj odstęp, aby nie pisać na lewym pasku)
		wmove(ga.win_info, getcury(ga.win_info) + (getcurx(ga.win_info) > 0 ? 1 : 0), 1);

		// nowy wiersz przywraca domyślne atrybuty
		wattrset(ga.win_info, A_NORMAL);

		nick_len = it->second.size();

		for(int i = 0; i < nick_len; ++i)
		{
			c = it->second[i];

			// wykryj formatowanie kolorów
			if(c == dCOLOR)
			{
				++i;	// przejdź na kod koloru

				// ustaw kolor (jeśli kolory są obsługiwane)
				if(ga.use_colors && i < nick_len)
				{
					wattron(ga.win_info, COLOR_PAIR(it->second[i]));
				}
			}

			else if(c == dBOLD_ON)
			{
				wattron(ga.win_info, A_BOLD);
			}

			else if(c == dBOLD_OFF)
			{
				wattroff(ga.win_info, A_BOLD);
			}

			else if(c == dREVERSE_ON)
			{
				wattron(ga.win_info, A_REVERSE);
			}

			else if(c == dREVERSE_OFF)
			{
				wattroff(ga.win_info, A_REVERSE);
			}

			else if(c == dUNDERLINE_ON)
			{
				wattron(ga.win_info, A_UNDERLINE);
			}

			else if(c == dUNDERLINE_OFF)
			{
				wattroff(ga.win_info, A_UNDERLINE);
			}

			else if(c == dNORMAL)
			{
				wattrset(ga.win_info, A_NORMAL);
			}

			else
			{
				wprintw(ga.win_info, "%c", c);
			}
		}
	}

	wnoutrefresh(ga.win_info);
}
