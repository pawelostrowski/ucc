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


#ifndef AUTH_HPP
#define AUTH_HPP

// plik do zapisania CAPTCHA
#define CAPTCHA_FILE "/tmp/ucc_captcha.gif"

// wersja apletu (informacja dla serwera), od czasu do czasu można zmodyfikować na nowszą wersję
#define AP_VER "1.1(20140526-0005 - R)"

// wymuś całą autoryzację przez HTTPS, a nie tylko samo hasło, jeśli z jakiegoś powodu ta opcja będzie sprawiać problemy, można ją wyłączyć
#define ALL_AUTH_HTTPS true

void auth_code(std::string &authkey);

bool auth_http_init(struct global_args &ga, struct channel_irc *ci[]);

bool auth_http_getcaptcha(struct global_args &ga, struct channel_irc *ci[]);

bool auth_http_checkcode(struct global_args &ga, struct channel_irc *ci[], std::string &captcha);

bool auth_http_getsk(struct global_args &ga, struct channel_irc *ci[]);

bool auth_http_mlogin(struct global_args &ga, struct channel_irc *ci[]);

bool auth_http_useroverride(struct global_args &ga, struct channel_irc *ci[]);

bool auth_http_getuokey(struct global_args &ga, struct channel_irc *ci[]);

void auth_irc_all(struct global_args &ga, struct channel_irc *ci[]);

#endif		// AUTH_HPP
