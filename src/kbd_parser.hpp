#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP
#define KBD_PARSER_HPP_NAME "kbd_parser"

#include "auth.hpp"

void kbd_parser(std::string &kbd_buf, std::string &msg, short &msg_color, std::string &msg_irc, std::string &my_nick, std::string &my_password, std::string &zuousername, std::string &cookies,
                std::string &uokey, bool &command_ok, bool &captcha_ready, bool &irc_ready, bool irc_ok, std::string &room, bool &room_ok, bool &command_me, bool &ucc_quit);

void msg_connect_irc(std::string &msg);

int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start);

void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper);

bool rest_args(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest);

#endif      // KBD_PARSER_HPP
