#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#define SOCKETS_HPP_NAME "sockets"


int socket_a(std::string &host, std::string &port, std::string &data_send, char *c_buffer, long &offset_recv, bool useirc = false);

int http_1(std::string &cookies);

int http_2(std::string &cookies);

int http_3(std::string &cookies, std::string &captcha_code, std::string &err_code);

int http_4(std::string &cookies, std::string &nick, std::string &zuousername, std::string &uokey, std::string &err_code);

int irc(std::string &zuousername, std::string &uokey);

#endif // SOCKETS_HPP
