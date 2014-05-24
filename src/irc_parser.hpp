#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

std::string get_value_from_buf(std::string buffer_str, std::string expr_before, std::string expr_after);

void irc_parser(struct global_args &ga, struct channel_irc *chan_parm[]);

void get_raw_parm(std::string &buffer_irc_raw, std::string *raw_parm);

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

void raw_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_001();

void raw_002();

void raw_003();

void raw_004();

void raw_005();

void raw_251();

void raw_252();

void raw_253();

void raw_254();

void raw_255();

void raw_265();

void raw_266();

void raw_332(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_366();

void raw_372(struct global_args &ga, std::string &buffer_irc_raw);

void raw_375(struct global_args &ga);

void raw_376(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_801(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_auth(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

#endif		// IRC_PARSER_HPP
