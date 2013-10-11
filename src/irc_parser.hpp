#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

void irc_parser(std::string &buffer_irc_recv, std::string &buffer_irc_recv_tmp, std::string &msg, short &msg_color, std::string &room, bool &send_irc, bool &irc_ok);

#endif      // IRC_PARSER_HPP
