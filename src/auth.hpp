#ifndef AUTH_HPP
#define AUTH_HPP
#define AUTH_HPP_NAME "auth"

#define FILE_GIF "/tmp/onetcaptcha.gif"

bool auth_code(std::string &authkey);

int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value);     // szuka wartości pomiędzy wyrażeniem początkowym i końcowym

bool http_auth_init(std::string &cookies, std::string &msg_err);

bool http_auth_getcaptcha(std::string &cookies, std::string &msg_err);

bool http_auth_getsk(std::string &cookies, std::string &msg_err);

bool http_auth_sendcaptcha(std::string &cookies, std::string &captcha, std::string &msg_err);

bool http_auth_sendnickpasswd(std::string &cookies, std::string my_nick, std::string my_password, std::string &msg_err);

bool http_auth_getuo(std::string &cookies, std::string my_nick, std::string my_password, std::string &zuousername, std::string &uokey, std::string &msg_err);

bool irc_auth_1(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, struct sockaddr_in &irc_info);

bool irc_auth_2(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &zuousername);

bool irc_auth_3(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent);

bool irc_auth_4(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent);

bool irc_auth_5(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_sent, std::string &zuousername, std::string &uokey);

#endif      // AUTH_HPP
