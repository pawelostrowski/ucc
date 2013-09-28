#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP
#define IRC_PARSER_HPP_NAME "irc_parser"

#include <ncursesw/ncurses.h>

int irc_parser(char *buffer_recv, int socketfd, WINDOW *active_room);

#endif      // IRC_PARSER_HPP
