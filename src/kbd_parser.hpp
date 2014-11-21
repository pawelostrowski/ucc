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


#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP

void msg_err_first_login(struct global_args &ga, struct channel_irc *ci[]);

void msg_err_already_logged_in(struct global_args &ga, struct channel_irc *ci[]);

void msg_err_not_specified_nick(struct global_args &ga, struct channel_irc *ci[]);

void msg_err_not_active_chan(struct global_args &ga, struct channel_irc *ci[]);

void msg_err_disconnect(struct global_args &ga, struct channel_irc *ci[]);

std::string get_arg(std::string &kbd_buf, size_t &pos_arg_start, bool lower2upper = false);

std::string get_rest_args(std::string &kbd_buf, size_t pos_arg_start);

void kbd_parser(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf);

/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie.
*/

void command_away(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_ban_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &ban_type);

void command_busy(struct global_args &ga, struct channel_irc *ci[]);

void command_captcha(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_card(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_connect(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_disconnect(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_favourites_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, bool add_del);

void command_friends_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, bool add_del);

void command_help(struct global_args &ga, struct channel_irc *ci[]);

void command_ignore_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, bool add_del);

void command_invite(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_join(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_kban_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &kban_type);

void command_kick(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_list(struct global_args &ga, struct channel_irc *ci[]);

void command_lusers(struct global_args &ga, struct channel_irc *ci[]);

void command_me(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_names(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_nick(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_op_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &op_type);

void command_part(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_priv(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_quit(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_raw(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_set(struct global_args &ga, std::string &kbd_buf, size_t pos_arg_start);

void command_time(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_topic(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_vhost(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_vip_common(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start, std::string &vip_type);

void command_whois(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

void command_whowas(struct global_args &ga, struct channel_irc *ci[], std::string &kbd_buf, size_t pos_arg_start);

#endif		// KBD_PARSER_HPP
