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


#include <string>		// std::string

#include "form_conv.hpp"
#include "ucc_global.hpp"


bool onet_font_check(std::string &onet_font)
{
/*
	Funkcja ta służy tylko do sprawdzenia, czy nazwa czcionki istnieje, aby uznać, że formatowanie jest poprawne.
*/

	if(onet_font == "arial")
	{
		return true;
	}

	else if(onet_font == "times")
	{
		return true;
	}

	else if(onet_font == "verdana")
	{
		return true;
	}

	else if(onet_font == "tahoma")
	{
		return true;
	}

	else if(onet_font == "courier")
	{
		return true;
	}

	return false;
}


bool onet_color_conv(std::string &onet_color, std::string &x_color)
{
/*
	Ze względu na to, że terminal w podstawowej wersji kolorów nie obsługuje kolorów, jakie są na czacie, trzeba było niektóre "oszukać" i wybrać
	inne, które są podobne (mało eleganckie, ale w tej sytuacji inaczej nie można, zanim nie zostanie dodana obsługa xterm-256).
*/

	if(onet_color == "623c00")		// brązowy
	{
		x_color = xYELLOW;
	}

	else if(onet_color == "c86c00")		// ciemny pomarańczowy
	{
		x_color = xYELLOW;
	}

	else if(onet_color == "ff6500")		// pomarańczowy
	{
		x_color = xYELLOW;
	}

	else if(onet_color == "ff0000")		// czerwony
	{
		x_color = xRED;
	}

	else if(onet_color == "e40f0f")		// ciemniejszy czerwony
	{
		x_color = xRED;
	}

	else if(onet_color == "990033")		// bordowy
	{
		x_color = xRED;
	}

	else if(onet_color == "8800ab")		// fioletowy
	{
		x_color = xMAGENTA;
	}

	else if(onet_color == "ce00ff")		// magenta
	{
		x_color = xMAGENTA;
	}

	else if(onet_color == "0f2ab1")		// granatowy
	{
		x_color = xBLUE;
	}

	else if(onet_color == "3030ce")		// ciemny niebieski
	{
		x_color = xBLUE;
	}

	else if(onet_color == "006699")		// cyjan
	{
		x_color = xCYAN;
	}

	else if(onet_color == "1a866e")		// zielono-cyjanowy
	{
		x_color = xCYAN;
	}

	else if(onet_color == "008100")		// zielony
	{
		x_color = xGREEN;
	}

	else if(onet_color == "959595")		// szary
	{
		x_color = xWHITE;
	}

	else if(onet_color == "000000")		// czarny
	{
		x_color = xTERMC;		// jako kolor zależny od ustawień terminala
	}

	else					// nieobsługiwany kod koloru zwraca błąd
	{
		return false;
	}

	return true;
}


std::string form_from_chat(std::string &irc_recv_buf)
{
/*
	Konwersja danych z serwera, gdzie jest formatowanie fontów, kolorów i emotikon (%...%).
*/

	bool was_form;		// czy było formatowanie
	int j;
	int irc_recv_buf_len = irc_recv_buf.size();
	std::string converted_buf, onet_font, onet_color, x_color, onet_emoticon;

	for(int i = 0; i < irc_recv_buf_len; ++i)
	{
		was_form = false;	// domyślnie załóż, że nie było formatowania
		j = i;			// nie zmieniaj aktualnej pozycji podczas sprawdzania (zmieniona zostanie, gdy było poprawne formatowanie)

		// znak "%" rozpoczyna formatowanie
		if(irc_recv_buf[j] == '%')
		{
			++j;		// kolejny znak

			// wykryj formatowanie fontów
			if(j < irc_recv_buf_len && irc_recv_buf[j] == 'F')
			{
				++j;

				// wykryj "Fi" (kursywa)
				if(j < irc_recv_buf_len && irc_recv_buf[j] == 'i')
				{
					++j;	// pomiń "i"
				}

				// wykryj bold
				if(j < irc_recv_buf_len && irc_recv_buf[j] == 'b')
				{
					++j;

					// wykryj "Fbi" (bold z kursywą)
					if(j < irc_recv_buf_len && irc_recv_buf[j] == 'i')
					{
						++j;	// pomiń "i"
					}

					// jeśli za znakiem "b" lub "i" jest "%", kończy to formatowanie, trzeba teraz wstawić kod włączający bold
					if(j < irc_recv_buf_len && irc_recv_buf[j] == '%')
					{
						was_form = true;	// było formatowanie (czyli nie wyświetlaj obecnego "%")
						i = j;		// kolejny obieg pętli zacznie czytać za tym "%"
						converted_buf += xBOLD_ON;
					}

					// jeśli za "b" nie było "%" to powinien być ":"
					else if(j < irc_recv_buf_len && irc_recv_buf[j] == ':')
					{
						onet_font.clear();

						// dalej powinna być nazwa czcionki
						for(++j; j < irc_recv_buf_len; ++j)
						{
							// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
							if(irc_recv_buf[j] == ' ')
							{
								break;
							}

							// znak "%" za nazwą czcionki kończy formatowanie, trzeba teraz wstawić kod włączający bold,
							// o ile czcionka jest poprawna
							else if(irc_recv_buf[j] == '%')
							{
								if(onet_font_check(onet_font))
								{
									was_form = true;// było poprawne formatowanie (czyli nie wyświetlaj obecnego "%")
									i = j;		// kolejny obieg pętli zacznie czytać za tym "%"
									converted_buf += xBOLD_ON;
								}

								break;
							}

							// pobierz kolejne litery nazwy czcionki
							onet_font += irc_recv_buf[j];
						}
					}
				}

				// brak bold
				// jeśli za znakiem "F" lub "i" jest '%", kończy to formatowanie, trzeba teraz wstawić kod wyłączający bold
				else if(j < irc_recv_buf_len && irc_recv_buf[j] == '%')
				{
					was_form = true;	// było formatowanie (czyli nie wyświetlaj obecnego "%")
					i = j;		// kolejny obieg pętli zacznie czytać za tym "%"
					converted_buf += xBOLD_OFF;
				}

				// jeśli za "F" nie było "%" to powinien być ":"
				else if(j < irc_recv_buf_len && irc_recv_buf[j] == ':')
				{
					onet_font.clear();

					// dalej powinna być nazwa czcionki
					for(++j; j < irc_recv_buf_len; ++j)
					{
						// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
						if(irc_recv_buf[j] == ' ')
						{
							break;
						}

						// znak "%" za nazwą czcionki kończy formatowanie, trzeba teraz wstawić kod wyłączający bold,
						// o ile czcionka jest poprawna
						else if(irc_recv_buf[j] == '%')
						{
							if(onet_font_check(onet_font))
							{
								was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego "%")
								i = j;		// kolejny obieg pętli zacznie czytać za tym "%"
								converted_buf += xBOLD_OFF;
							}

							break;
						}

						// pobierz kolejne litery nazwy czcionki
						onet_font += irc_recv_buf[j];
					}
				}

			}	// "F"

			// wykryj formatowanie kolorów
			else if(j < irc_recv_buf_len && irc_recv_buf[j] == 'C')
			{
				onet_color.clear();

				// wczytaj kolor
				for(++j; j < irc_recv_buf_len; ++j)
				{
					// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
					if(irc_recv_buf[j] == ' ')
					{
						break;
					}

					// znak "%" za kolorem kończy formatowanie, trzeba teraz wstawić formatowanie koloru (o ile kolor ma 6 znaków)
					else if(irc_recv_buf[j] == '%')
					{
						if(onet_color.size() == 6 && onet_color_conv(onet_color, x_color))
						{
							was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego "%")
							i = j;		// kolejny obieg pętli zacznie czytać za tym "%"
							converted_buf += x_color;
						}

						break;
					}

					// pobierz kolejne znaki koloru
					onet_color += irc_recv_buf[j];
				}

			}	// "C"

			// wykryj formatowanie emotikon
			else if(j < irc_recv_buf_len && irc_recv_buf[j] == 'I')
			{
				onet_emoticon.clear();

				// wczytaj nazwę emotikony
				for(++j; j < irc_recv_buf_len; ++j)
				{
					// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
					if(irc_recv_buf[j] == ' ')
					{
						break;
					}

					// znak "%" za emotikoną kończy formatowanie, trzeba teraz wstawić nazwę emotikony z dwoma "//" na początku
					else if(irc_recv_buf[j] == '%')
					{
						was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego "%")
						i = j;		// kolejny obieg pętli zacznie czytać za tym "%"
						converted_buf += "//" + onet_emoticon;

						break;
					}

					// pobierz kolejne znaki emotikony
					onet_emoticon += irc_recv_buf[j];
				}

			}	// "I"

		}	// "%"

		// wpisuj znaki, które nie należą do formatowania
		if(! was_form)
		{
			converted_buf += irc_recv_buf[i];
		}
	}

	// zwróć przekonwertowany bufor
	return converted_buf;
}


std::string form_to_chat(std::string &kbd_buf)
{
/*
	Konwersja danych do serwera. Przekształcaj //emotikona na %Iemotikona%, kolory i bold będą dodawane inaczej.
	Konwertuj wybrane skróty, np. :d ;d ;o ;x na %Iemotikona% (aby użytkownicy apletu mogli je zobaczyć, są to skróty, które normalnie w aplecie nie są
	widoczne, dlatego nie są to oficjalne skróty).
*/

	bool was_form;		// czy było formatowanie
	bool was_colon = false;	// wykrycie dwukropka przed "//" (zacznij od braku dwukropka)
	int j;
	int kbd_buf_len = kbd_buf.size();
	std::string converted_buf, onet_emoticon;

	for(int i = 0; i < kbd_buf_len; ++i)
	{
		was_form = false;	// domyślnie załóż, że nie było formatowania
		j = i;			// nie zmieniaj aktualnej pozycji podczas sprawdzania (zmieniona zostanie, gdy było poprawne formatowanie)

		// znak "/" rozpoczyna formatowanie (jeśli wcześniej był dwukropek, nie konwertuj emotikony, ma to znaczenie np. przy wstawianiu http://adres)
		if(! was_colon && kbd_buf[j] == '/')
		{
			++j;		// kolejny znak

			// wykryj drugi "/"
			if(j < kbd_buf_len && kbd_buf[j] == '/')
			{
				++j;

				// nie przekształcaj emotikony jeśli za "//" jest spacja lub kolejny "/"
				if(j < kbd_buf_len && kbd_buf[j] != ' ' && kbd_buf[j] != '/')
				{
					onet_emoticon.clear();

					while(true)
					{
						// koniec bufora przerywa (kończy formatowanie emotikony)
						if(j == kbd_buf_len)
						{
							was_form = true;
							i = j;		// to już koniec bufora, więc pętla for() się zakończy
							converted_buf += "%I" + onet_emoticon + "%";

							break;
						}

						// spacja oraz "/" przerywa (kończy formatowanie emotikony)
						else if(kbd_buf[j] == ' ' || kbd_buf[j] == '/')
						{
							was_form = true;
							i = j - 1;	// kolejny obieg pętli ma wczytać spację lub "/" do bufora
							converted_buf += "%I" + onet_emoticon + "%";

							break;
						}

						// pobierz kolejne znaki emotikony
						onet_emoticon += kbd_buf[j];

						++j;	// kolejny znak emotikony
					}
				}
			}
		}

/*
	DO POPRAWY JESZCZE

		// znak ":" rozpoczyna formatowanie wybranych emotikon (nie będą wyświetlone w programie, tylko wysłane w odpowiedniej formie na serwer)
		// (jeśli wcześniej był już dwukropek, nie dokonuj konwersji, ma znaczenie przy wklejaniu np. "std::string")
		else if(! was_colon && j + 1 < kbd_buf_len && kbd_buf[j] == ':')
		{
			++j;		// kolejny znak

			// wykrycie ":d", konwertowanego na "%Ihehe%"
			if(kbd_buf[j] == 'd')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "d"
				converted_buf += "%Ihehe%";
			}

			// wykrycie ":o" lub ":O", konwertowanego na "%Ipanda%"
			else if(kbd_buf[j] == 'o' || kbd_buf[j] == 'O')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "o"/"O"
				converted_buf += "%Ipanda%";
			}

			// wykrycie ":p", konwertowanego na "%Ijezyk%"
			else if(kbd_buf[j] == 'p')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "p"
				converted_buf += "%Ijezyk%";
			}

			// wykrycie ":s" lub ":S", konwertowanego na "%Iskwaszony%"
			else if(kbd_buf[j] == 's' || kbd_buf[j] == 'S')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "s"/"S"
				converted_buf += "%Iskwaszony%";
			}

			// wykrycie ":X", konwertowanego na "%Inie_powiem%"
			else if(kbd_buf[j] == 'X')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "X"
				converted_buf += "%Inie_powiem%";
			}
		}

		// znak ";" rozpoczyna formatowanie wybranych emotikon (nie będą wyświetlone w programie, tylko wysłane w odpowiedniej formie na serwer)
		else if(j + 1 < kbd_buf_len && kbd_buf[j] == ';')
		{
			++j;		// kolejny znak

			// wykrycie ";d", konwertowanego na "%Ihehe%"
			if(kbd_buf[j] == 'd')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "d"
				converted_buf += "%Ihehe%";
			}

			// wykrycie ";o", konwertowanego na "%Ipanda%"
			else if(kbd_buf[j] == 'o')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "o"
				converted_buf += "%Ipanda%";
			}

			// wykrycie ";O", konwertowanego na "%Ixpanda%"
			else if(kbd_buf[j] == 'O')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "O"
				converted_buf += "%Ixpanda%";
			}

			// wykrycie ";p", konwertowanego na "%Ijezor%"
			else if(kbd_buf[j] == 'p')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "p"
				converted_buf += "%Ijezor%";
			}

			// wykrycie ";s" lub ";S", konwertowanego na "%Iskwaszony%"
			else if(kbd_buf[j] == 's' || kbd_buf[j] == 'S')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "s"/"S"
				converted_buf += "%Iskwaszony%";
			}

			// wykrycie ";x" lub ";X", konwertowanego na "%Inie_powiem%"
			else if(kbd_buf[j] == 'x' || kbd_buf[j] == 'X')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "x"/"X"
				converted_buf += "%Inie_powiem%";
			}

			// wykrycie ";*", konwertowanego na "%Icmok2%"
			else if(kbd_buf[j] == '*')
			{
				was_form = true;
				i = j;		// kolejny obieg pętli zacznie czytać za "*"
				converted_buf += "%Icmok2%";
			}
		}
*/

		// wykrycie dwukropka, aby nie konwertować wpisanego linka (http://adres) na emotikonę
		was_colon = (kbd_buf[i] == ':' ? true : false);

		// wpisuj znaki, które nie należą do formatowania
		if(! was_form)
		{
			converted_buf += kbd_buf[i];
		}
	}

	// zwróć przekonwertowany bufor
	return converted_buf;
}


std::string day_en_to_pl(std::string &day_en)
{
	if(day_en == "Mon")
	{
		return "poniedziałek";
	}

	else if(day_en == "Tue")
	{
		return "wtorek";
	}

	else if(day_en == "Wed")
	{
		return "środa";
	}

	else if(day_en == "Thu")
	{
		return "czwartek";
	}

	else if(day_en == "Fri")
	{
		return "piątek";
	}

	else if(day_en == "Sat")
	{
		return "sobota";
	}

	else if(day_en == "Sun")
	{
		return "niedziela";
	}

	// gdyby zmieniono sposób zapisu, zwróć oryginalną formę
	else
	{
		return day_en;
	}
}


std::string month_en_to_pl(std::string &month_en)
{
	if(month_en == "Jan")
	{
		return "stycznia";
	}

	else if(month_en == "Feb")
	{
		return "lutego";
	}

	else if(month_en == "Mar")
	{
		return "marca";
	}

	else if(month_en == "Apr")
	{
		return "kwietnia";
	}

	else if(month_en == "May")
	{
		return "maja";
	}

	else if(month_en == "Jun")
	{
		return "czerwca";
	}

	else if(month_en == "Jul")
	{
		return "lipca";
	}

	else if(month_en == "Aug")
	{
		return "sierpnia";
	}

	else if(month_en == "Sep")
	{
		return "września";
	}

	else if(month_en == "Oct")
	{
		return "października";
	}

	else if(month_en == "Nov")
	{
		return "listopada";
	}

	else if(month_en == "Dec")
	{
		return "grudnia";
	}

	// gdyby zmieniono sposób zapisu, zwróć oryginalną formę
	else
	{
		return month_en;
	}
}


std::string remove_form(std::string &in_buf)
{
	int in_buf_len = in_buf.size();
	std::string out_buf;

	// usuń formatowanie fontu i kolorów
	for(int i = 0; i < in_buf_len; ++i)
	{
		if(in_buf[i] == dCOLOR)
		{
			++i;
		}

		else if(in_buf[i] != dBOLD_ON && in_buf[i] != dBOLD_OFF && in_buf[i] != dREVERSE_ON && in_buf[i] != dREVERSE_OFF
			&& in_buf[i] != dUNDERLINE_ON && in_buf[i] != dUNDERLINE_OFF && in_buf[i] != dNORMAL)
		{
			out_buf += in_buf[i];
		}
	}

	return out_buf;
}
