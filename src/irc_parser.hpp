#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP
#define IRC_PARSER_HPP_NAME "irc_parser"

int irc_parser(char *c_buffer, std::string &data_send, int &socketfd, bool &connect_status);

#endif      // IRC_PARSER_HPP
