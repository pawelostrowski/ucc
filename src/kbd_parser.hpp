#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP
#define KBD_PARSER_HPP_NAME "kbd_parser"

#include "auth.hpp"

void kbd_parser(std::string &kbd_buf, std::string &msg, short &msg_color, std::string &cookies, std::string &nick, std::string &zuousername,
                std::string &uokey, bool &captcha_ok, bool &irc_ready, bool irc_ok, std::string &room, bool &room_ok, bool &send_irc, bool &ucc_quit);

int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start);

void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper);

bool insert_rest(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest);

#endif      // KBD_PARSER_HPP
