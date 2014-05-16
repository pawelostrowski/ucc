#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

int find_value_irc(std::string buffer_irc_raw, std::string expr_before, std::string expr_after, std::string &f_value);

void irc_parser(std::string &buffer_irc_recv, std::string &msg_irc, bool &irc_ok, struct channel_irc *chan_parm[], int &chan_nr);

#endif		// IRC_PARSER_HPP
