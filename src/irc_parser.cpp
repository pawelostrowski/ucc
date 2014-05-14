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


void irc_parser(std::string &buffer_irc_recv, std::string &msg_scr, std::string &msg_irc, std::string &channel, bool &irc_ok)
{
	std::string f_value;

	std::string buffer_irc_raw;
	size_t pos_raw_start = 0, pos_raw_end = 0;

	// zacznij od wyczyszczenia bufora komunikatu oraz bufora IRC
	msg_scr.clear();
	msg_irc.clear();

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
			msg_scr = get_time() + xRED + "# Błąd w buforze IRC!";
			return;
		}

		// wstaw aktualnie obsługiwany wiersz (raw)
		buffer_irc_raw.clear();
		buffer_irc_raw.insert(0, buffer_irc_recv, pos_raw_start, pos_raw_end - pos_raw_start + 1);

		// przyjmij, że kolejny wiersz jest za kodem \n, a jeśli to koniec bufora, wykryte to będzie na końcu pętli do{} while()
		pos_raw_start = pos_raw_end + 1;

		// odpowiedz na PING
		if(find_value_irc(buffer_irc_raw, "PING :", "\n", f_value) == 0)
		{
			msg_irc = "PONG :" + f_value;
		}

		else if(find_value_irc(buffer_irc_raw, "JOIN " + channel + " :", "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			std::string nick_on_irc_zuo_ip;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);	// pobierz nick wchodzący
			find_value_irc(buffer_irc_raw, "!", " ", nick_on_irc_zuo_ip);	// pobierz ZUO oraz zakodowane IP
			msg_scr += get_time() + xGREEN + "* " + nick_on_irc + " [" + nick_on_irc_zuo_ip + "] wchodzi do pokoju.";
		}

		else if(find_value_irc(buffer_irc_raw, "PART " + channel, "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			std::string nick_on_irc_zuo_ip;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);	// pobierz nick wychodzący z pokoju
			find_value_irc(buffer_irc_raw, "!", " ", nick_on_irc_zuo_ip);	// pobierz ZUO oraz zakodowane IP
			msg_scr += get_time() + xCYAN + "* " + nick_on_irc + " [" + nick_on_irc_zuo_ip + "] wychodzi z pokoju";
			// jeśli jest komunikat w PART, dodaj go
			if(f_value.size() > 0)
			{
				msg_scr += " [" + f_value.erase(0, 2) +"]";	// erase(0, 2) - usuń ' :'
			}
			msg_scr += ".";
		}

		else if(find_value_irc(buffer_irc_raw, "QUIT :", "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			std::string nick_on_irc_zuo_ip;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);	// pobierz nick wychodzący z czata
			find_value_irc(buffer_irc_raw, "!", " ", nick_on_irc_zuo_ip);	// pobierz ZUO oraz zakodowane IP
			msg_scr += get_time() + xYELLOW + "* " + nick_on_irc + " [" + nick_on_irc_zuo_ip + "] wychodzi z czata [" + f_value + "].";
		}

		// nieeleganckie na razie wycinanie z tekstu (z założeniem, że chodzi o 1 pokój), aby pokazać komunikat usera
		else if(find_value_irc(buffer_irc_raw, "PRIVMSG " + channel + " :", "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			find_value_irc(buffer_irc_raw, ":", "!", nick_on_irc);
			msg_scr += get_time() + "<" + nick_on_irc + "> " + f_value;
		}

		// wykryj, gdy serwer odpowie ERROR
		else if(find_value_irc(buffer_irc_raw, "ERROR :", "\n", f_value) == 0)
		{
			irc_ok = false;
			msg_scr += get_time() + buffer_irc_raw;		// pokaż też komunikat serwera
		}

		// nieznane lub jeszcze niezaimplementowane rawy wyświetl bez zmian
		else
		{
			msg_scr += get_time() + xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1);
		}

	} while(pos_raw_start < buffer_irc_recv.size());

}
