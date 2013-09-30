#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP
#define IRC_PARSER_HPP_NAME "irc_parser"

#include <ncursesw/ncurses.h>

int irc_parser(WINDOW *active_room, char *buffer_recv, int socketfd);

#endif      // IRC_PARSER_HPP
