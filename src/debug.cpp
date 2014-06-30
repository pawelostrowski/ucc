#include <string>		// std::string

#include "enc_str.hpp"
#include "ucc_global.hpp"


void dbg_http_to_file(std::string dbg_sent, std::string dbg_recv, std::string &host, short port, std::string &stock, std::string &msg_dbg_http)
{
	static bool dbg_first_save = true;	// pierwszy zapis nadpisuje starą zawartość pliku

	std::ofstream file_dbg;

	if(dbg_first_save)
	{
		file_dbg.open(FILE_DBG_HTTP, std::ios::out);
	}

	else
	{
		file_dbg.open(FILE_DBG_HTTP, std::ios::app | std::ios::out);
	}

	dbg_first_save = false;			// kolejne zapisy dopisują dane do pliku

	if(file_dbg.good())
	{
		// jeśli wysyłane było hasło, ukryj je oraz długość zapytania (zdradza długość hasła)
		size_t exp_start, exp_end;
		std::string pwd_str = "haslo=";
		std::string con_str = "Content-Length: ";

		exp_start = dbg_sent.find(pwd_str);

		if(exp_start != std::string::npos)
		{
			exp_end = dbg_sent.find("&", exp_start);

			if(exp_end != std::string::npos)
			{
				dbg_sent.erase(exp_start + pwd_str.size(), exp_end - exp_start - pwd_str.size());
				dbg_sent.insert(exp_start + pwd_str.size(), "[hidden]");

				// było hasło, więc ukryj długość zapytania
				exp_start = dbg_sent.find(con_str);

				if(exp_start != std::string::npos)
				{
					exp_end = dbg_sent.find("\r\n", exp_start);

					if(exp_end != std::string::npos)
					{
						dbg_sent.erase(exp_start + con_str.size(), exp_end - exp_start - con_str.size());
						dbg_sent.insert(exp_start + con_str.size(), "[hidden]");
					}
				}
			}
		}

		// jeśli pobrano obrazek, zakończ string za wyrażeniem GIFxxx
		std::string gif_str = "GIF";

		exp_start = dbg_recv.find(gif_str);

		if(exp_start != std::string::npos)
		{
			dbg_recv.erase(exp_start + gif_str.size() + 3, dbg_recv.size() - exp_start - gif_str.size() - 3);
		}

		// usuń \r z buforów, aby w pliku nie było go
		while(dbg_sent.find("\r") != std::string::npos)
		{
			dbg_sent.erase(dbg_sent.find("\r"), 1);
		}

		while(dbg_recv.find("\r") != std::string::npos)
		{
			dbg_recv.erase(dbg_recv.find("\r"), 1);
		}

		// zapisz dane
		std::string s;

		if(port == 443)
		{
			s = "s";
		}

		file_dbg << "================================================================================\n";

		file_dbg << msg_dbg_http + "\n\n\n";

		file_dbg << "--> SENT (http" + s + "://" + host + stock + "):\n\n";

		file_dbg << dbg_sent + "\n";

		if(dbg_sent[dbg_sent.size() - 1] != '\n')
		{
			file_dbg << "\n\n";
		}

		file_dbg << "--> RECV:\n\n";

		file_dbg << buf_iso2utf(dbg_recv) + "\n";

		if(dbg_recv[dbg_recv.size() - 1] != '\n')
		{
			file_dbg << "\n\n";
		}

		file_dbg.close();
	}
}
