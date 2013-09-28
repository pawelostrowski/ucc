#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP
#define KBD_PARSER_HPP_NAME "kbd_parser"

#include <ncursesw/ncurses.h>

void kbd_parser(WINDOW *active_room, bool use_colors, int &socketfd_irc, fd_set &readfds, std::string kbd_buf, std::string &cookies,
                std::string &nick, std::string &zuousername, std::string room, bool &captcha_ok, bool &irc_ok, bool &ucc_quit);

int find_command(std::string kbd_buf, std::string &f_command, size_t &arg_start);

void find_arg(std::string kbd_buf, std::string &f_arg, size_t &arg_start, bool lower2upper = true);

#endif      // KBD_PARSER_HPP
