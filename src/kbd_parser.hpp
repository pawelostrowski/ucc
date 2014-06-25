#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP

int get_command(std::string &kbd_buf, std::string &g_command, std::string &g_command_org, size_t &pos_arg_start);

std::string get_arg(std::string &kbd_buf, size_t &pos_arg_start, bool lower2upper);

std::string get_rest_args(std::string &kbd_buf, size_t pos_arg_start);

void kbd_parser(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf);

void msg_connect_irc_err(struct global_args &ga, struct channel_irc *chan_parm[]);

/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie.
*/

void kbd_command_away(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_busy(struct global_args &ga, struct channel_irc *chan_parm[]);

void kbd_command_captcha(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_card(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_connect(struct global_args &ga, struct channel_irc *chan_parm[]);

void kbd_command_disconnect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_help(struct global_args &ga, struct channel_irc *chan_parm[]);

void kbd_command_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_me(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_names(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_priv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_raw(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_time(struct global_args &ga, struct channel_irc *chan_parm[]);

void kbd_command_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_vhost(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_whois(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

void kbd_command_whowas(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t &pos_arg_start);

#endif		// KBD_PARSER_HPP
