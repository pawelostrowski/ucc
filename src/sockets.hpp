#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#define SOCKETS_HPP_NAME "sockets"


int socket_a(std::string &host, std::string &port, std::string &data_send, char *c_buffer, long &offset_recv);

int find_cookies(char *c_buffer, std::string &cookies);

int find_value(char *c_buffer, std::string &expr_before, std::string &expr_after, std::string &f_value);	// szuka wartości pomiędzy wyrażeniem początkowym i końcowym

int http_1(std::string &cookies);

int http_2(std::string &cookies);

int http_3(std::string &cookies, std::string &captcha_code, std::string &err_code);

int http_4(std::string &cookies, std::string &nick, std::string &uokey, std::string &zuousername, std::string &err_code);

#endif // SOCKETS_HPP
