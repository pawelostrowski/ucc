#ifndef AUTH_HPP
#define AUTH_HPP
#define AUTH_HPP_NAME "auth"

#define FILE_GIF "/tmp/onetcaptcha.gif"

bool auth_code(std::string &authkey);

void header_get(std::string host, std::string data_get, std::string &data_send, std::string &cookies, bool add_cookies = false);

void header_post(std::string host, std::string data_post, std::string &data_send, std::string &cookies, std::string &api_function);

int find_cookies(char *buffer_recv, std::string &cookies);

int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value);     // szuka wartości pomiędzy wyrażeniem początkowym i końcowym

int http_auth_1(std::string &cookies);

int http_auth_2(std::string &cookies);

int http_auth_3(std::string &cookies, std::string &captcha, std::string &err_code);

int http_auth_4(std::string &cookies, std::string my_nick, std::string &zuousername, std::string &uokey, std::string &err_code);

bool irc_auth_1(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, struct sockaddr_in &irc_info);

bool irc_auth_2(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &zuousername);

bool irc_auth_3(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent);

bool irc_auth_4(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent);

bool irc_auth_5(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_sent, std::string &zuousername, std::string &uokey);

#endif      // AUTH_HPP
