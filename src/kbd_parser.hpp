#ifndef KBD_PARSER_HPP
#define KBD_PARSER_HPP

int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start);

void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper);

bool rest_args(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest);

std::string msg_connect_irc_err();

void kbd_parser(std::string &kbd_buf, std::string &msg_scr, std::string &msg_irc, std::string &my_nick, std::string &my_password, std::string &cookies,
		std::string &zuousername, std::string &uokey, bool &captcha_ready, bool &irc_ready, bool &irc_ok, bool &channel_ok, std::string &channel,
		bool &ucc_quit);

#endif		// KBD_PARSER_HPP
