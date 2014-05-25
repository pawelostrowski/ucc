#include <string>		// std::string

#include "ucc_global.hpp"


bool check_font(std::string &onet_font)
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

	return xTERMC;			// gdy żaden z wymienionych
}


std::string form_from_chat(std::string &buffer_irc_recv)
{
/*
	Konwersja danych z serwera, gdzie jest formatowanie fontów, kolorów i emotikon (%...%).
*/

	bool was_form;		// czy było formatowanie
	int j;
	int buffer_len = buffer_irc_recv.size();
	std::string buffer_converted, onet_font, onet_color, onet_emoticon;

	for(int i = 0; i < buffer_len; ++i)
	{
		was_form = false;	// domyślnie załóż, że nie było formatowania
		j = i;			// nie zmieniaj aktualnej pozycji podczas sprawdzania (zmieniona zostanie, gdy było poprawne formatowanie)

		// znak % rozpoczyna formatowanie
		if(buffer_irc_recv[j] == '%')
		{
			++j;		// kolejny znak

			// wykryj formatowanie fontów
			if(j < buffer_len && buffer_irc_recv[j] == 'F')
			{
				++j;

				// wykryj Fi (kursywa)
				if(j < buffer_len && buffer_irc_recv[j] == 'i')
				{
					++j;	// pomiń i
				}

				// wykryj bold
				if(j < buffer_len && buffer_irc_recv[j] == 'b')
				{
					++j;

					// wykryj Fbi (bold z kursywą)
					if(j < buffer_len && buffer_irc_recv[j] == 'i')
					{
						++j;	// pomiń i
					}

					// jeśli za znakiem b lub i jest %, kończy to formatowanie, trzeba teraz wstawić kod bold
					if(j < buffer_len && buffer_irc_recv[j] == '%')
					{
						was_form = true;	// było formatowanie (czyli nie wyświetlaj obecnego %)
						i = j;		// kolejny obieg pętli zacznie czytać za tym %
						buffer_converted += xBOLD_ON;
					}

					// jeśli za b nie było % to powinien być :
					else if(j < buffer_len && buffer_irc_recv[j] == ':')
					{
						onet_font.clear();

						// dalej powinna być nazwa czcionki
						for(++j; j < buffer_len; ++j)
						{
							// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
							if(buffer_irc_recv[j] == ' ')
							{
								break;
							}

							// znak % za nazwą czcionki kończy formatowanie, trzeba teraz wstawić kod bold,
							// o ile czcionka jest poprawna
							else if(buffer_irc_recv[j] == '%' && check_font(onet_font))
							{
								was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
								i = j;		// kolejny obieg pętli zacznie czytać za tym %
								buffer_converted += xBOLD_ON;

								break;
							}

							// pobierz kolejno litery nazwy czcionki
							onet_font += buffer_irc_recv[j];
						}
					}
				}

				// brak bold
				// jeśli za znakiem F lub i jest %, kończy to formatowanie, trzeba teraz wstawić kod wyłączający bold
				else if(j < buffer_len && buffer_irc_recv[j] == '%')
				{
					was_form = true;	// było formatowanie (czyli nie wyświetlaj obecnego %)
					i = j;		// kolejny obieg pętli zacznie czytać za tym %
					buffer_converted += xBOLD_OFF;
				}

				// jeśli za F nie było % to powinien być :
				else if(j < buffer_len && buffer_irc_recv[j] == ':')
				{
					onet_font.clear();

					// dalej powinna być nazwa czcionki
					for(++j; j < buffer_len; ++j)
					{
						// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
						if(buffer_irc_recv[j] == ' ')
						{
							break;
						}

						// znak % za nazwą czcionki kończy formatowanie, trzeba teraz wstawić kod wyłączający bold,
						// o ile czcionka jest poprawna
						else if(buffer_irc_recv[j] == '%' && check_font(onet_font))
						{
							was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
							i = j;		// kolejny obieg pętli zacznie czytać za tym %
							buffer_converted += xBOLD_OFF;

							break;
						}

						// pobierz kolejne litery nazwy czcionki
						onet_font += buffer_irc_recv[j];
					}
				}

			}	// F

			// wykryj formatowanie kolorów
			else if(j < buffer_len && buffer_irc_recv[j] == 'C')	// dodać reakcję na nieprawidłowe nazwy kolorów (wyświetlać je)
			{
				onet_color.clear();

				// wczytaj kolor
				for(++j; j < buffer_len; ++j)
				{
					// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
					if(buffer_irc_recv[j] == ' ')
					{
						break;
					}

					// znak % za kolorem kończy formatowanie, trzeba teraz wstawić formatowanie koloru (o ile kolor ma 6 znaków)
					else if(buffer_irc_recv[j] == '%' && onet_color.size() == 6)
					{
						was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
						i = j;		// kolejny obieg pętli zacznie czytać za tym %
						buffer_converted += onet_color_conv(onet_color);

						break;
					}

					// pobierz kolejne znaki koloru
					onet_color += buffer_irc_recv[j];
				}

			}	// C

			// wykryj formatowanie emotikon
			else if(j < buffer_len && buffer_irc_recv[j] == 'I')
			{
				onet_emoticon.clear();

				// wczytaj nazwę emotikony
				for(++j; j < buffer_len; ++j)
				{
					// spacja wewnątrz formatowania przerywa (nie jest to wtedy poprawne formatowanie)
					if(buffer_irc_recv[j] == ' ')
					{
						break;
					}

					// znak % za emotikoną kończy formatowanie, trzeba teraz wstawić nazwę emotikony z dwoma // na początku
					else if(buffer_irc_recv[j] == '%')
					{
						was_form = true;	// było poprawne formatowanie (czyli nie wyświetlaj obecnego %)
						i = j;		// kolejny obieg pętli zacznie czytać za tym %
						buffer_converted += "//" + onet_emoticon;

						break;
					}

					// pobierz kolejne znaki emotikony
					onet_emoticon += buffer_irc_recv[j];
				}

			}	// I

		}	// %

		// wpisuj znaki, które nie należą do formatowania
		if(! was_form)
		{
			buffer_converted += buffer_irc_recv[i];
		}
	}

	return buffer_converted;	// zwróć przekonwertowany bufor z czata
}
