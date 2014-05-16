#include <string>		// std::string

#include "irc_parser.hpp"
#include "window_utils.hpp"
#include "ucc_global.hpp"


int find_value_irc(std::string buffer_irc_raw, std::string expr_before, std::string expr_after, std::string &f_value)
{
	size_t pos_expr_before, pos_expr_after;		// pozycja początkowa i końcowa szukanych wyrażeń

	pos_expr_before = buffer_irc_raw.find(expr_before);	// znajdź pozycję początku szukanego wyrażenia
	if(pos_expr_before == std::string::npos)
		return 1;	// kod błędu, gdy nie znaleziono początku szukanego wyrażenia

	// znajdź pozycję końca szukanego wyrażenia, zaczynając od znalezionego początku + jego jego długości
	pos_expr_after = buffer_irc_raw.find(expr_after, pos_expr_before + expr_before.size());
	if(pos_expr_after == std::string::npos)
		return 2;	// kod błędu, gdy nie znaleziono końca szukanego wyrażenia

	// wstaw szukaną wartość
	f_value.clear();	// wyczyść bufor szukanej wartości
	f_value.insert(0, buffer_irc_raw, pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());

	return 0;
}


void irc_parser(std::string &buffer_irc_recv, std::string &msg_irc, bool &irc_ok, struct channel_irc *chan_parm[], int &chan_nr)
{
	std::string f_value;
	std::string f_channel;
	size_t f_channel_start, f_channel_end;

	std::string buffer_irc_raw;
	size_t pos_raw_start = 0, pos_raw_end = 0;

	// zacznij od wyczyszczenia bufora komunikatu oraz bufora IRC
//	msg_scr.clear();
	msg_irc.clear();

	// konwersja formatowania fontu, kolorów i emotek
	buffer_irc_recv = form_from_chat(buffer_irc_recv);

	// obsłuż bufor
	do
	{
		// znajdź koniec wiersza
		pos_raw_end = buffer_irc_recv.find("\n", pos_raw_start);

		// nie może dojść do sytuacji, że na końcu wiersza nie ma \n
		if(pos_raw_end == std::string::npos)
		{
			pos_raw_start = 0;
			pos_raw_end = 0;
			chan_parm[chan_nr]->win_buf += get_time() + xRED + "# Błąd w buforze IRC!";
			return;
		}

		// wstaw aktualnie obsługiwany wiersz (raw)
		buffer_irc_raw.clear();
		buffer_irc_raw.insert(0, buffer_irc_recv, pos_raw_start, pos_raw_end - pos_raw_start + 1);

		// przyjmij, że kolejny wiersz jest za kodem \n, a jeśli to koniec bufora, wykryte to będzie na końcu pętli do{} while()
		pos_raw_start = pos_raw_end + 1;

		// wykryj pokój z rawa (jeśli jest)
		f_channel.clear();
		f_channel_start = buffer_irc_raw.find("#");
		if(f_channel_start != std::string::npos)
		{
			f_channel_end = buffer_irc_raw.find(" ", f_channel_start);

			if(f_channel_end == std::string::npos)
			{
				f_channel_end = buffer_irc_raw.find("\n", f_channel_start);

				if(f_channel_end == std::string::npos)
				{
					chan_parm[chan_nr]->win_buf += get_time() + xRED + "# Błąd w parsowaniu kanału z bufora RAW!";
				}
			}

			f_channel.insert(0, buffer_irc_raw, f_channel_start, f_channel_end - f_channel_start);
		}

		// odpowiedz na PING
		if(find_value_irc(buffer_irc_raw, "PING :", "\n", f_value) == 0)
		{
			msg_irc = "PONG :" + f_value;
		}

		else if(find_value_irc(buffer_irc_raw, "JOIN " + f_channel + " :", "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			std::string nick_on_irc_zuo_ip;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);	// pobierz nick wchodzący
			find_value_irc(buffer_irc_raw, "!", " ", nick_on_irc_zuo_ip);	// pobierz ZUO oraz zakodowane IP

			for(int i = 0; i < 2; ++i)
			{
				if(chan_parm[i]->channel == f_channel)
				{
					chan_parm[i]->win_buf += get_time() + xGREEN + "* " + nick_on_irc
								 + " [" + nick_on_irc_zuo_ip + "] wchodzi do pokoju.";
				}
			}
		}

		else if(find_value_irc(buffer_irc_raw, "PART " + f_channel, "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			std::string nick_on_irc_zuo_ip;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);	// pobierz nick wychodzący z pokoju
			find_value_irc(buffer_irc_raw, "!", " ", nick_on_irc_zuo_ip);	// pobierz ZUO oraz zakodowane IP

			for(int i = 0; i < 2; ++i)
			{
				if(chan_parm[i]->channel == f_channel)
				{
					chan_parm[i]->win_buf += get_time() + xCYAN + "* " + nick_on_irc
								 + " [" + nick_on_irc_zuo_ip + "] wychodzi z pokoju";

					// jeśli jest komunikat w PART, dodaj go
					if(f_value.size() > 0)
					{
						chan_parm[i]->win_buf += " [" + f_value.erase(0, 2) +"]";	// erase(0, 2) - usuń ' :'
					}
					chan_parm[i]->win_buf += ".";
				}
			}
		}

		else if(find_value_irc(buffer_irc_raw, "QUIT :", "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			std::string nick_on_irc_zuo_ip;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);	// pobierz nick wychodzący z czata
			find_value_irc(buffer_irc_raw, "!", " ", nick_on_irc_zuo_ip);	// pobierz ZUO oraz zakodowane IP

//			for(int i = 0; i < 2; ++i)
//			{
//				if(chan_parm[i]->channel == f_channel)
//				{
					chan_parm[chan_nr]->win_buf += get_time() + xYELLOW + "* " + nick_on_irc
								 + " [" + nick_on_irc_zuo_ip + "] wychodzi z czata [" + f_value + "].";
//				}
//			}
		}

		else if(find_value_irc(buffer_irc_raw, "PRIVMSG " + f_channel + " :", "\n", f_value) == 0)
//		else if(buffer_irc_raw.find("PRIVMSG") != std::string::npos)
		{
			std::string nick_on_irc;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);

			for(int i = 0; i < 2; ++i)
			{
				if(chan_parm[i]->channel == f_channel)
				{
					find_value_irc(buffer_irc_raw, " :", "\n", f_value);
					chan_parm[i]->win_buf += get_time() + "<" + nick_on_irc + "> " + f_value;
				}
			}
		}

		// wykryj, gdy serwer odpowie ERROR
		else if(find_value_irc(buffer_irc_raw, "ERROR :", "\n", f_value) == 0)
		{
			irc_ok = false;

			// pokaż też komunikat serwera
			chan_parm[chan_nr]->win_buf += get_time() + xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1);
		}

		// nieznane lub jeszcze niezaimplementowane rawy wyświetl bez zmian
		else
		{
			// jeśli raw dotyczy konkretnego pokoju, to go tam wrzuć
			if(f_channel.size() > 0 && buffer_irc_raw.find(f_channel) != std::string::npos)
			{
				for(int i = 0; i < 2; ++i)
				{
					if(chan_parm[i]->channel == f_channel)
					{
						chan_parm[i]->win_buf += get_time() + xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1);
					}
				}
			}

			else
			{
				chan_parm[chan_nr]->win_buf += get_time() + xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1);
			}
		}

	} while(pos_raw_start < buffer_irc_recv.size());

}
