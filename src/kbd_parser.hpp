#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP

std::string get_arg(std::string &kbd_buf, size_t &pos_arg_start, bool lower2upper = false);

std::string get_rest_args(std::string &kbd_buf, size_t pos_arg_start);

void msg_err_first_login(struct global_args &ga, struct channel_irc *chan_parm[]);

void msg_err_not_active_chan(struct global_args &ga, struct channel_irc *chan_parm[]);

void msg_err_disconnect(struct global_args &ga, struct channel_irc *chan_parm[]);

void kbd_parser(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf);

/*
	Poniżej są funkcje do obsługi poleceń wpisywanych w programie.
*/

void command_away(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_busy(struct global_args &ga, struct channel_irc *chan_parm[]);

void command_captcha(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_card(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_connect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_disconnect(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_help(struct global_args &ga, struct channel_irc *chan_parm[]);

void command_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_list(struct global_args &ga, struct channel_irc *chan_parm[]);

void command_me(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_names(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_priv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_raw(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_time(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_vhost(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_whois(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

void command_whowas(struct global_args &ga, struct channel_irc *chan_parm[], std::string &kbd_buf, size_t pos_arg_start);

#endif		// KBD_PARSER_HPP
