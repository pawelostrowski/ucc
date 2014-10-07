/*
	Ucieszony Chat Client
	Copyright (C) 2013, 2014 Paweł Ostrowski

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


#ifndef CHAT_UTILS_HPP
#define CHAT_UTILS_HPP

void new_chan_status(struct global_args &ga, struct channel_irc *ci[]);

void new_chan_debug_irc(struct global_args &ga, struct channel_irc *ci[]);

void new_chan_raw_unknown(struct global_args &ga, struct channel_irc *ci[]);

bool new_chan_chat(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, bool active);

void del_chan_chat(struct global_args &ga, struct channel_irc *ci[], std::string chan_name);

void del_all_chan(struct channel_irc *ci[]);

void new_or_update_nick_chan(struct global_args &ga, struct channel_irc *ci[], std::string &channel, std::string &nick, std::string zuo_ip,
	struct nick_flags flags);

void del_nick_chan(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, std::string nick);

#endif		// CHAT_UTILS_HPP
