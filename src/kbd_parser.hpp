#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP
#define KBD_PARSER_HPP_NAME "kbd_parser"

void kbd_parser(std::string &kbd_buf, std::string &msg, short &msg_color, std::string &cookies,
                std::string &nick, std::string &zuousername, bool &captcha_ok, bool &irc_ok, int socketfd_irc, bool &ucc_quit);

int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &arg_start);

void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &arg_start, bool lower2upper);

#endif      // KBD_PARSER_HPP
