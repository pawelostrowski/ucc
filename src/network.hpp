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


#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <vector>		// std::vector

// wielkość bufora tymczasowego używanego podczas pobierania danych z serwera
#define RECV_BUF_TMP_SIZE (1500 + 1 * sizeof(char))

// numer wersji przeglądarki, wysyłany w ciągu User Agent podczas autoryzacji HTTP
#define BROWSER_VER "35.0"

// User Agent przeglądarki, wysyłany podczas autoryzacji HTTP(S)
#define BROWSER_USER_AGENT "Mozilla/5.0 (X11; Linux x86_64; rv:" BROWSER_VER ") Gecko/20100101 Firefox/" BROWSER_VER

int socket_init(std::string host, uint16_t port, std::string &msg_err);

bool http_get_cookies(std::string http_recv_buf_str, std::vector<std::string> &cookies, std::string &msg_err);

char *http_get_data(struct global_args &ga, struct channel_irc *ci[], std::string method, std::string host, uint16_t port, std::string stock,
	std::string content, bool get_cookies, int &bytes_recv_all, std::string dbg_http_msg);

void irc_send(struct global_args &ga, struct channel_irc *ci[], std::string irc_send_buf, std::string dbg_irc_msg = "");

void irc_recv(struct global_args &ga, struct channel_irc *ci[], std::string &irc_recv_buf, std::string dbg_irc_msg = "");

#endif		// NETWORK_HPP
