#ifndef AUTH_HPP
#define AUTH_HPP

void auth_code(std::string &authkey);

// szuka wartości pomiędzy wyrażeniem początkowym i końcowym
int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value);

bool http_auth_init(std::string &cookies, std::string &msg_err);

bool http_auth_getcaptcha(std::string &cookies, std::string &msg_err);

bool http_auth_getsk(std::string &cookies, std::string &msg_err);

bool http_auth_checkcode(std::string &cookies, std::string &captcha, std::string &msg_err);

bool http_auth_mlogin(std::string &cookies, std::string my_nick, std::string my_password, std::string &msg_err);

bool http_auth_useroverride(std::string &cookies, std::string my_nick, std::string &msg_err);

bool http_auth_getuo(std::string &cookies, std::string my_nick, std::string my_password, std::string &zuousername, std::string &uokey, std::string &msg_err);

bool irc_auth_1(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err);

bool irc_auth_2(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &zuousername, std::string &msg_err);

bool irc_auth_3(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &msg_err);

bool irc_auth_4(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &msg_err);

bool irc_auth_5(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_sent, std::string &zuousername, std::string &uokey, std::string &msg_err);

void irc_auth_all(struct global_args &ga, struct channel_irc *chan_parm[]);

#endif		// AUTH_HPP
