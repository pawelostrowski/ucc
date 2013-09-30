#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP
#define KBD_PARSER_HPP_NAME "kbd_parser"

#include "msg_window.hpp"

void kbd_parser(WINDOW *active_window, bool use_colors, std::string kbd_buf, std::string &cookies, std::string &nick, std::string &zuousername,
                std::string room, bool &captcha_ok, bool &irc_ok, int socketfd_irc, bool &ucc_quit);

int find_command(std::string kbd_buf, std::string &f_command, size_t &arg_start);

void find_arg(std::string kbd_buf, std::string &f_arg, size_t &arg_start, bool lower2upper = true);

#endif      // KBD_PARSER_HPP
