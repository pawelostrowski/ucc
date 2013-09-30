#ifndef AUTH_HPP
#define AUTH_HPP
#define AUTH_HPP_NAME "auth"

#define COOKIE_STRING "Set-Cookie:"
#define FILE_GIF "/tmp/onetcaptcha.gif"

bool auth_code(std::string &authkey);

void header_get(std::string host, std::string data_get, std::string &cookies, std::string &data_send, bool add_cookies = false);

void header_post(std::string &cookies, std::string &api_function, std::string &data_send);

int find_cookies(char *buffer_recv, std::string &cookies);

int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value);     // szuka wartości pomiędzy wyrażeniem początkowym i końcowym

int http_1(std::string &cookies);

int http_2(std::string &cookies);

int http_3(std::string &cookies, std::string &captcha, std::string &err_code);

int http_4(std::string &cookies, std::string &nick, std::string &zuousername, std::string &uokey, std::string &err_code);

#endif      // AUTH_HPP
