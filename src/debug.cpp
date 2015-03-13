/*
	Ucieszony Chat Client
	Copyright (C) 2013-2015 Paweł Ostrowski

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

#include "debug.hpp"
#include "enc_str.hpp"
#include "window_utils.hpp"
#include "ucc_global.hpp"


void http_dbg_to_file_header(struct global_args &ga, std::string dbg_header)
{
	if(ga.debug_http_f.good())
	{
		ga.debug_http_f << "================================================================================";

#ifndef __CYGWIN__
		ga.debug_http_f << "\n";
#else
		ga.debug_http_f << "\r\n";
#endif // __CYGWIN__

		ga.debug_http_f << dbg_header << " (" << get_time_full() << "):";

#ifndef __CYGWIN__
		ga.debug_http_f << "\n\n\n";
#else
		ga.debug_http_f << "\r\n\r\n\r\n";
#endif // __CYGWIN__

		ga.debug_http_f.flush();
	}
}


void http_dbg_to_file(struct global_args &ga, std::string dbg_sent, std::string dbg_recv, std::string &host, uint16_t port, std::string &stock)
{
	if(ga.debug_http_f.good())
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

#ifndef __CYGWIN__
		// usuń \r z buforów
		size_t code_erase = dbg_sent.find("\r");

		while(code_erase != std::string::npos)
		{
			dbg_sent.erase(dbg_sent.find("\r"), 1);
			code_erase = dbg_sent.find("\r");
		}

		code_erase = dbg_recv.find("\r");

		while(code_erase != std::string::npos)
		{
			dbg_recv.erase(dbg_recv.find("\r"), 1);
			code_erase = dbg_recv.find("\r");
		}
#endif // __CYGWIN__

		// zapisz dane do pliku
		ga.debug_http_f << "--> SENT (http" << (port == 443 ? "s" : "") << "://" << host << stock << "):";

#ifndef __CYGWIN__
		ga.debug_http_f << "\n\n";
#else
		ga.debug_http_f << "\r\n\r\n";
#endif // __CYGWIN__

		ga.debug_http_f << dbg_sent;

#ifndef __CYGWIN__
		ga.debug_http_f << "\n";
#else
		ga.debug_http_f << "\r\n";
#endif // __CYGWIN__

		if(dbg_sent.size() > 0 && dbg_sent[dbg_sent.size() - 1] != '\n')
		{
#ifndef __CYGWIN__
			ga.debug_http_f << "\n\n";
#else
			ga.debug_http_f << "\r\n\r\n";
#endif // __CYGWIN__
		}

		ga.debug_http_f << "<-- RECV:";

#ifndef __CYGWIN__
		ga.debug_http_f << "\n\n";
#else
		ga.debug_http_f << "\r\n\r\n";
#endif // __CYGWIN__

		ga.debug_http_f << buf_iso2utf(dbg_recv);

#ifndef __CYGWIN__
		ga.debug_http_f << "\n";
#else
		ga.debug_http_f << "\r\n";
#endif // __CYGWIN__

		if(dbg_sent.size() > 0 && dbg_recv[dbg_recv.size() - 1] != '\n')
		{
#ifndef __CYGWIN__
			ga.debug_http_f << "\n\n";
#else
			ga.debug_http_f << "\r\n\r\n";
#endif // __CYGWIN__
		}

		ga.debug_http_f.flush();
	}
}
