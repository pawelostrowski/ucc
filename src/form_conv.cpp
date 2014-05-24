#include <string>		// std::string

#include "ucc_global.hpp"


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
