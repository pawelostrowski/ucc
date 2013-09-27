#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP
#define KBD_PARSER_HPP_NAME "kbd_parser"

#include <ncursesw/ncurses.h>

void kbd_parser(std::string kbd_buf, WINDOW *win_diag, int socketfd, bool &ucc_quit);

int find_command(std::string kbd_buf, std::string &f_command);

#endif      // KBD_PARSER_HPP
