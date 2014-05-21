#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

std::string get_value_raw(std::string &buffer_irc_raw, std::string expr_before, std::string expr_after);

void irc_parser(struct global_args &ga, struct channel_irc *chan_parm[]);

void irc_parser(std::string &buffer_irc_recv, std::string &msg_irc, bool &irc_ok, struct channel_irc *chan_parm[], int &chan_nr);

/*
	Poniżej funkcje do obsługi RAW.
*/

void raw_error(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_ping(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_invite(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_mode(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_privmsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

#endif		// IRC_PARSER_HPP
