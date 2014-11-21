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


#ifndef WINDOW_UTILS_HPP
#define WINDOW_UTILS_HPP

bool check_colors();

std::string get_time();

std::string time_utimestamp_to_local(std::string &time_unixtimestamp);

std::string get_time_full();

std::string time_utimestamp_to_local_full(std::string &time_unixtimestamp);

std::string time_sec2time(std::string &sec_str);

std::string buf_lower_to_upper(std::string buf);

int buf_chars(std::string &in_buf);

int how_lines(std::string &in_buf, int wterm_x, int time_len);

void kbd_buf_show(std::string &kbd_buf, int term_y, int term_x, int kbd_cur_pos, int &kbd_cur_pos_offset);

void win_buf_show(struct global_args &ga, struct channel_irc *ci[]);

void win_buf_refresh(struct global_args &ga, struct channel_irc *ci[]);

void win_buf_add_str(struct global_args &ga, struct channel_irc *ci[], std::string chan_name, std::string in_buf, bool save_log = true, int act_type = 1,
	bool add_time = true, bool only_chan_normal = true);

void win_buf_all_chan_msg(struct global_args &ga, struct channel_irc *ci[], std::string msg, bool save_log = true);

std::string get_flags_nick(struct global_args &ga, struct channel_irc *ci[], int chan_index, std::string nick_key);

void nicklist_refresh(struct global_args &ga, struct channel_irc *ci[]);

#endif		// WINDOW_UTILS_HPP
