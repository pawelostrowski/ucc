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

	strftime(time_date, 45, "%A, %-d %b %Y, %H:%M:%S", time_date_l);	// %-d, aby nie było nieznaczącego zera w dniu miesiąca

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


int buf_chars(std::string &buf_in)
{
/*
	Policz liczbę znaków, uwzględniając formatowanie tekstu oraz kodowanie UTF-8.
	Uwaga! Nie jest sprawdzana poprawność zapisu w UTF-8 (procedura zakłada, że znaki zapisane są poprawnie).
*/

	int buf_in_len = buf_in.size();

	char c;
	int count_char = 0;

	for(int l = 0; l < buf_in_len; ++l)
	{
		c = buf_in[l];

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


int how_lines(std::string &buf_in, int wterm_x, int time_len)
{
	int buf_in_len = buf_in.size();

	char c;
	int virt_cur = 0;
	bool first_char = true;

	std::string word;
	int line = 1;
	int x;

	for(int l = 0; l < buf_in_len; ++l)
	{
		c = buf_in[l];

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
					if(l + x == buf_in_len || buf_in[l + x] == ' ')
					{
						if(virt_cur - wterm_x + buf_chars(word) > 0)
						{
							virt_cur = time_len;
							++line;
						}

						break;
					}

					word += buf_in[l + x];
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
	std::string time_tmp = get_time();
	int time_len = buf_chars(time_tmp);	// pobierz liczbę znaków zajmowaną przez pokazanie czasu

	int utf_i;

	// wyczyść starą zawartość i ustaw kursor na początku okna "wirtualnego" (dla bezpieczeństwa, bo różne implementacje ncurses różnie to traktują)
	werase(ga.win_chat);
	wmove(ga.win_chat, 0, 0);

	// sprawdź, czy włączony jest scroll okna
	if(ci[ga.current]->win_scroll_lock)
	{
	}

	// jeśli nie jest włączony scroll okna, wyznacz początek wyświetlania
	else
	{
		// zacznij od końca std::vector
		win_buf_start = win_buf_len;

		while(win_buf_start > 0 && skip_count < ga.wterm_y)
		{
			// policz, ile fizycznie wierszy okna "wirtualnego" zajmuje jedna pozycja w std::vector
			skip_count += how_lines(ci[ga.current]->win_buf[win_buf_start - 1], ga.wterm_x, time_len);

			// pozycja wstecz w std::vector
			--win_buf_start;
		}

		// sprawdź, czy pierwszy wyświetlany wiersz fizycznie zajmuje więcej niż szerokość okna "wirtualnego", jeśli tak, to o ile, aby przy
		// wyświetlaniu wiersza pominąć te "nadmiarowe" wiersze, ale tylko wtedy, gdy liczba wierszy przekracza wysokość okna "wirtualnego"
		if(win_buf_len > 0 && skip_count > ga.wterm_y && how_lines(ci[ga.current]->win_buf[win_buf_start], ga.wterm_x, time_len) > 1)
		{
			line_skip_leading = skip_count - ga.wterm_y;
		}
	}

	// wypisywanie w pętli
	// (getcury(ga.win_chat) != wterm_y - 1 || getcurx(ga.win_chat) == 0) - wyświetlaj, dopóki nie osiągnięto częściowo zapisanego ostatniego wiersza
	for(int w = win_buf_start; w < win_buf_len && (getcury(ga.win_chat) != ga.wterm_y - 1 || getcurx(ga.win_chat) == 0); ++w)
	{
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
			// jeśli osiągnięto ostatni znak ostatniego wiersza, zakończ kolejne przejście pętli
			if(getcury(ga.win_chat) == ga.wterm_y - 1 && getcurx(ga.win_chat) == ga.wterm_x - 1)
			{
				last_char = true;
			}

			// aktualnie przetwarzany znak (lub kod formatowania)
			c = ci[ga.current]->win_buf[w][l];

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
						virt_cur = time_len;
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
									virt_cur = time_len;
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
						for(int i = time_len; i > 0; --i)
						{
							waddch(ga.win_chat, ' ');
						}
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


void win_buf_add_str(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, std::string in_buf, int act_type, bool add_time,
	bool only_chan_normal)
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
		if(ci[which_chan]->chan_log.good())
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


void win_buf_all_chan_msg(struct global_args &ga, struct channel_irc *ci[], std::string msg)
{
/*
	Wyświetl komunikat we wszystkich otwartych pokojach, z wyjątkiem "DebugIRC" i "RawUnknown".
*/

	for(int i = 0; i < CHAN_NORMAL; ++i)
	{
		if(ci[i])
		{
			win_buf_add_str(ga, ci, ci[i]->channel, msg);
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
			if(! it->second.nf.busy)
			{
				nick_tmp += "`";
			}

			else
			{
				nick_tmp += xWHITE "`";
			}
		}

		else if(it->second.nf.op)	// jeśli był ` to nie pokazuj @
		{
			if(! it->second.nf.busy)
			{
				nick_tmp += "@";
			}

			else
			{
				nick_tmp += xWHITE "@";
			}
		}

		if(it->second.nf.halfop)
		{
			if(! it->second.nf.busy)
			{
				nick_tmp += "%";
			}

			else
			{
				nick_tmp += xWHITE "%";
			}
		}

		if(it->second.nf.moderator)
		{
			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON xMAGENTA "!";
			}

			else
			{
				nick_tmp += xMAGENTA "!";
			}
		}

		if(it->second.nf.voice)
		{
			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON xBLUE "+";
			}

			else
			{
				nick_tmp += xBLUE "+";
			}
		}

		if(it->second.nf.public_webcam)
		{
			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON xGREEN "*";
			}

			else
			{
				nick_tmp += xGREEN "*";
			}
		}

		if(it->second.nf.private_webcam)
		{
			// przy braku obsługi kolorów dla odróżnienia gwiazdek prywatna kamerka będzie na odwróconym tle
			if(! ga.use_colors)
			{
				nick_tmp += xREVERSE_ON;
			}

			if(! it->second.nf.busy)
			{
				nick_tmp += xBOLD_ON xRED "*";
			}

			else
			{
				nick_tmp += xRED "*";
			}
		}

		if(! it->second.nf.busy)
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


void nicklist_refresh(struct global_args &ga, struct channel_irc *ci[])
{
	// PRZEBUDOWAĆ LISTĘ NICKÓW!

	int y = 0;
	int wterm_y, wterm_x;
	int wcur_x;
	std::string nicklist, nick_owner, nick_op, nick_halfop, nick_moderator, nick_voice, nick_pub_webcam, nick_priv_webcam, nick_normal;

	// zacznij od wyczyszczenia listy
	wattrset(ga.win_info, A_NORMAL);
	werase(ga.win_info);

	// narysuj linię z lewej strony od góry do dołu (jeśli jest obsługa kolorów, to na niebiesko)
	if(ga.use_colors)
	{
		wattron(ga.win_info, COLOR_PAIR(pBLUE));
	}

	wborder(ga.win_info, ACS_VLINE, ' ', ' ', ' ', ACS_VLINE, ' ', ACS_VLINE, ' ');

	// pasek na szaro dla napisu "Użytkownicy:"
	wmove(ga.win_info, y, 1);

	if(ga.use_colors)
	{
		wattron(ga.win_info, COLOR_PAIR(pWHITE));
	}

	wattron(ga.win_info, A_REVERSE);
	getmaxyx(ga.win_info, wterm_y, wterm_x);

	for(int i = 1; i < wterm_x; ++i)
	{
		wprintw(ga.win_info, " ");
	}

	// pobierz nicki w kolejności zależnej od uprawnień
	for(auto it = ci[ga.current]->ni.begin(); it != ci[ga.current]->ni.end(); ++it)
	{
		if(it->second.nf.owner)
		{
			nick_owner += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else if(it->second.nf.op)
		{
			nick_op += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else if(it->second.nf.halfop)
		{
			nick_halfop += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else if(it->second.nf.moderator)
		{
			nick_moderator += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else if(it->second.nf.voice)
		{
			nick_voice += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else if(it->second.nf.public_webcam)
		{
			nick_pub_webcam += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else if(it->second.nf.private_webcam)
		{
			nick_priv_webcam += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}

		else
		{
			nick_normal += get_flags_nick(ga, ci, it->first) + it->second.nick + "\n";
		}
	}

	// liczba osób w pokoju
	wmove(ga.win_info, y, 1);
	nicklist = "[Użytkownicy: " + std::to_string(ci[ga.current]->ni.size()) + "]\n";

	// połącz nicki w jedną listę
	nicklist += nick_owner + nick_op + nick_halfop + nick_moderator + nick_voice + nick_pub_webcam + nick_priv_webcam + nick_normal;

	++y;

	// wyświetl listę
	for(unsigned int i = 0; i >= 0 && i < nicklist.size() - 1 && y <= wterm_y; ++i)	// - 1, bo bez ostatniego kodu \n z listy
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
			if(nicklist[i] == dCOLOR && i + 1 < nicklist.size())
			{
				++i;	// przejdź na kod koloru

				if(ga.use_colors)
				{
					wattron(ga.win_info, COLOR_PAIR(nicklist[i]));
				}
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

					for(++i; i < nicklist.size() - 1; ++i)
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

	wnoutrefresh(ga.win_info);
}
