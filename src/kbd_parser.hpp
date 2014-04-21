#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP

#include "ucc_global.hpp"

bool kbd_parser(ucc_global_args *ucc_ga);

void msg_connect_irc_err(std::string &msg);

int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start);

void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper);

bool rest_args(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest);

#endif		// KBD_PARSER_HPP
