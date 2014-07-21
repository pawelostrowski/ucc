#include <string>		// std::string

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

	else if(onet_font == "")	// pusta czcionka w domyśle oznacza Verdana
	{
		return true;
	}

	return false;
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

	else if(onet_color == "000000")		// czarny
	{
		return xTERMC;			// jako kolor zależny od ustawień terminala
	}

	// gdy żaden z wymienionych, zwróć zamiast kodu koloru jego kod z czata, aby wyświetlić wpisaną wartość w programie (bez zmiany koloru)
	return "%C" + onet_color + "%";
}


std::string form_from_chat(std::string &irc_recv_buf)
{
/*
	Konwersja danych z serwera, gdzie jest formatowanie fontów, kolorów i emotikon (%...%).
*/

	bool was_form;		// czy było formatowanie
	int j;
	int irc_recv_buf_len = irc_recv_buf.size();
	std::string converted_buf, onet_font, onet_color, onet_emoticon;

	for(int i = 0; i < irc_recv_buf_len; ++i)
	{
		was_form = false;	// domyślnie załóż, że nie było formatowania
		j = i;			// nie zmieniaj aktualnej pozycji podczas sprawdzania (zmieniona zostanie, gdy było poprawne formatowanie)

		// znak % rozpoczyna formatowanie
		if(irc_recv_buf[j] == '%')
		{
			++j;		// kolejny znak

			// wykryj formatowanie fontów
			if(j < irc_recv_buf_len && irc_recv_buf[j] == 'F')
			{
				++j;

				// wykryj Fi (kursywa)
				if(j < irc_recv_buf_len && irc_recv_buf[j] == 'i')
				{
					++j;	// pomiń i
				}

				// wykryj bold
				if(j < irc_recv_buf_len && irc_recv_buf[j] == 'b')
				{
					++j;

					// wykryj Fbi (bold z kursywą)
					if(j < irc_recv_buf_len && irc_recv_buf[j] == 'i')
					{
						++j;	// pomiń i
					}

					// jeśli za znakiem b lub i jest %, kończy to formatowanie, trzeba teraz wstawić kod bold
					if(j < irc_recv_buf_len && irc_recv_buf[j] == '%')
					{
						was_form = true;	// było formatowanie (czyli nie wyświetlaj obecnego %)
						i = j;		// kolejny obieg pętli zacznie czytać za tym %
						converted_buf += xBOLD_ON;
					}

					// jeśli za b nie było % to powinien być :
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

							// znak % za nazwą czcionki kończy formatowanie, trzeba teraz wstawić kod bold,
							// o ile czcionka jest poprawna
							else if(irc_recv_buf[j] == '%' && onet_font_check(onet_font))
							{
								was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
								i = j;		// kolejny obieg pętli zacznie czytać za tym %
								converted_buf += xBOLD_ON;

								break;
							}

							// pobierz kolejne litery nazwy czcionki
							onet_font += irc_recv_buf[j];
						}
					}
				}

				// brak bold
				// jeśli za znakiem F lub i jest %, kończy to formatowanie, trzeba teraz wstawić kod wyłączający bold
				else if(j < irc_recv_buf_len && irc_recv_buf[j] == '%')
				{
					was_form = true;	// było formatowanie (czyli nie wyświetlaj obecnego %)
					i = j;		// kolejny obieg pętli zacznie czytać za tym %
					converted_buf += xBOLD_OFF;
				}

				// jeśli za F nie było % to powinien być :
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

						// znak % za nazwą czcionki kończy formatowanie, trzeba teraz wstawić kod wyłączający bold,
						// o ile czcionka jest poprawna
						else if(irc_recv_buf[j] == '%' && onet_font_check(onet_font))
						{
							was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
							i = j;		// kolejny obieg pętli zacznie czytać za tym %
							converted_buf += xBOLD_OFF;

							break;
						}

						// pobierz kolejne litery nazwy czcionki
						onet_font += irc_recv_buf[j];
					}
				}

			}	// F

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

					// znak % za kolorem kończy formatowanie, trzeba teraz wstawić formatowanie koloru (o ile kolor ma 6 znaków)
					else if(irc_recv_buf[j] == '%' && onet_color.size() == 6)
					{
						was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
						i = j;		// kolejny obieg pętli zacznie czytać za tym %
						converted_buf += onet_color_conv(onet_color);

						break;
					}

					// pobierz kolejne znaki koloru
					onet_color += irc_recv_buf[j];
				}

			}	// C

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

					// znak % za emotikoną kończy formatowanie, trzeba teraz wstawić nazwę emotikony z dwoma // na początku
					else if(irc_recv_buf[j] == '%')
					{
						was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
						i = j;		// kolejny obieg pętli zacznie czytać za tym %
						converted_buf += "//" + onet_emoticon;

						break;
					}

					// pobierz kolejne znaki emotikony
					onet_emoticon += irc_recv_buf[j];
				}

			}	// I

		}	// %

		// wpisuj znaki, które nie należą do formatowania
		if(! was_form)
		{
			converted_buf += irc_recv_buf[i];
		}
	}

	return converted_buf;	// zwróć przekonwertowany bufor z czata
}


std::string form_to_chat(std::string &kbd_buf)
{
/*
	Konwersja danych do serwera. Przekształcaj //emotikona na %Iemotikona%, kolory i bold będą dodawane inaczej.
	W przyszłości dodać konwersję wybranych skrótów, np. ;d ;o ;x na %Iemotikona%
*/

	bool was_form;		// czy było formatowanie
	bool colon = false;	// wykrycie dwukropka przed // (zacznij od braku dwukropka)
	int j;
	int irc_recv_buf_len = kbd_buf.size();
	std::string converted_buf, onet_emoticon;

	for(int i = 0; i < irc_recv_buf_len; ++i)
	{
		was_form = false;	// domyślnie załóż, że nie było formatowania
		j = i;			// nie zmieniaj aktualnej pozycji podczas sprawdzania (zmieniona zostanie, gdy było poprawne formatowanie)

		// znak / rozpoczyna formatowanie (jeśli wcześniej był dwukropek, nie konwertuj emotikony, ma to znaczenie np. przy wstawianiu http://adres)
		if(! colon && kbd_buf[j] == '/')
		{
			++j;		// kolejny znak

			// wykryj drugi /
			if(j < irc_recv_buf_len && kbd_buf[j] == '/')
			{
				++j;

				// nie przekształcaj emotikony jeśli za // jest spacja lub kolejny /
				if(j < irc_recv_buf_len && kbd_buf[j] != ' ' && kbd_buf[j] != '/')
				{
					onet_emoticon.clear();

					while(true)
					{
						// koniec bufora przerywa (kończy formatowanie emotikony)
						if(j == irc_recv_buf_len)
						{
							was_form = true;
							i = j;		// to już koniec bufora, więc pętla for() się zakończy
							converted_buf += "%I" + onet_emoticon + "%";

							break;
						}

						// spacja oraz / przerywa (kończy formatowanie emotikony)
						else if(kbd_buf[j] == ' ' || kbd_buf[j] == '/')
						{
							was_form = true;
							i = j - 1;	// kolejny obieg pętli ma wczytać spację lub / do bufora
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

		if(kbd_buf[i] == ':')
		{
			colon = true;
		}

		else
		{
			colon = false;
		}

		// wpisuj znaki, które nie należą do formatowania
		if(! was_form)
		{
			converted_buf += kbd_buf[i];
		}
	}

	return converted_buf;
}


std::string dayen2daypl(std::string &day_en)
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


std::string monthen2monthpl(std::string &month_en)
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
	std::string out_buf;

	// usuń formatowanie fontu i kolorów
	for(unsigned int i = 0; i < in_buf.size(); ++i)
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
