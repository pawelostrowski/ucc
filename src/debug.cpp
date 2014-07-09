#include <string>		// std::string

#include "debug.hpp"
#include "enc_str.hpp"
#include "window_utils.hpp"
#include "ucc_global.hpp"


void http_dbg_to_file(struct global_args &ga, std::string dbg_sent, std::string dbg_recv, std::string &host, short port, std::string &stock,
			std::string &msg_dbg_http)
{
	if(ga.f_dbg_http.good())
	{
		// jeśli wysyłane było hasło, ukryj je oraz długość zapytania (zdradza długość hasła)
		size_t exp_start, exp_end;
		const std::string pwd_str = "haslo=";
		const std::string con_str = "Content-Length: ";

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
		const std::string gif_str = "GIF";

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

		// dodaj datę i czas
		time_t time_g;

		time(&time_g);

		std::string time_str = std::to_string(time_g);

		// zapisz dane
		std::string s;

		if(port == 443)
		{
			s = "s";
		}

		ga.f_dbg_http << "================================================================================\n";

		ga.f_dbg_http << msg_dbg_http + " (" + time_unixtimestamp2local_full(time_str) + "):\n\n\n";

		ga.f_dbg_http << "--> SENT (http" + s + "://" + host + stock + "):\n\n";

		ga.f_dbg_http << dbg_sent + "\n";

		if(dbg_sent[dbg_sent.size() - 1] != '\n')
		{
			ga.f_dbg_http << "\n\n";
		}

		ga.f_dbg_http << "--> RECV:\n\n";

		ga.f_dbg_http << buf_iso2utf(dbg_recv) + "\n";

		if(dbg_recv[dbg_recv.size() - 1] != '\n')
		{
			ga.f_dbg_http << "\n\n";
		}

		ga.f_dbg_http.flush();
	}
}


void irc_sent_dbg_to_file(struct global_args &ga, std::string dbg_sent)
{
	if(ga.f_dbg_irc.good())
	{
		dbg_sent = buf_iso2utf(dbg_sent);	// zapis w UTF-8

		while(dbg_sent.find("\r") != std::string::npos)
		{
			dbg_sent.erase(dbg_sent.find("\r"), 1);
		}

		dbg_sent.insert(0, "--> ");

		size_t next_n = dbg_sent.find("\n");

		while(next_n != std::string::npos && next_n < dbg_sent.size() - 1)
		{
			dbg_sent.insert(next_n + 1, "--> ");
			next_n = dbg_sent.find("\n", next_n + 1);
		}

		ga.f_dbg_irc << dbg_sent;

		ga.f_dbg_irc.flush();
	}
}


void irc_recv_dbg_to_file(struct global_args &ga, std::string &dbg_recv)
{
	if(ga.f_dbg_irc.good())
	{
		ga.f_dbg_irc << dbg_recv;

		ga.f_dbg_irc.flush();
	}
}
