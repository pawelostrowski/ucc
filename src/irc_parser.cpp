#include <string>		// std::string
#include <cstring>		// strdup()

#include "irc_parser.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


void irc_parser(std::string &buffer_irc_recv, std::string &msg, std::string &msg_irc, std::string &channel, bool &irc_ok)
{
	std::string f_value;

	std::string buffer_irc_raw;
	size_t pos_raw_start = 0, pos_raw_end = 0;

	// zacznij od wyczyszczenia bufora powrotnego oraz bufora IRC
	msg.clear();
	msg_irc.clear();

	do
	{
		// znajdź koniec wiersza
		pos_raw_end = buffer_irc_recv.find("\n", pos_raw_start);

		// nie może dojść do sytuacji, że na końcu wiersza nie ma \n
		if(pos_raw_end == std::string::npos)
		{
			pos_raw_start = 0;
			pos_raw_end = 0;
			msg = "# Błąd w buforze IRC!";
			return;
		}

		// wstaw aktualnie obsługiwany wiersz (raw)
		buffer_irc_raw.clear();
		buffer_irc_raw.insert(0, buffer_irc_recv, pos_raw_start, pos_raw_end - pos_raw_start + 1);

		// przyjmij, że kolejny wiersz jest za kodem \n, a jeśli to koniec bufora, wykryte to będzie na końcu pętli do{} while()
		pos_raw_start = pos_raw_end + 1;

		// odpowiedz na PING
		if(find_value(strdup(buffer_irc_raw.c_str()), "PING :", "\n", f_value) == 0)
		{
			msg_irc = "PONG :" + f_value;
		}

		// nieeleganckie na razie wycinanie z tekstu (z założeniem, że chodzi o 1 pokój), aby pokazać komunikat usera
		else if(find_value(strdup(buffer_irc_raw.c_str()), "PRIVMSG " + channel + " :", "\n", f_value) == 0)
		{
			std::string nick_on_irc;
			find_value(strdup(buffer_irc_raw.c_str()), ":", "!", nick_on_irc);
			msg += "<" + nick_on_irc + "> " + f_value + "\n";
		}

		// wykryj, gdy serwer odpowie ERROR
		else if(find_value(strdup(buffer_irc_raw.c_str()), "ERROR :", "\n", f_value) == 0)
		{
			irc_ok = false;
			msg += buffer_irc_raw;		// pokaż też komunikat serwera
		}

		// nieznane lub jeszcze niezaimplementowane rawy wyświetl bez zmian
		else
		{
			msg += buffer_irc_raw;
		}

	} while(pos_raw_start < buffer_irc_recv.size());

}
