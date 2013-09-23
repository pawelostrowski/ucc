#ifndef AUTH_HTTP_HPP
#define AUTH_HTTP_HPP
#define AUTH_HTTP_HPP_NAME "auth_http"
#define GIF_FILE "/tmp/onetcaptcha.gif"     // plik z kodem captcha

int http_1(std::string &cookies);

int http_2(std::string &cookies);

int http_3(std::string &cookies, std::string captcha_code, std::string &err_code);

int http_4(std::string &cookies, std::string &nick, std::string &zuousername, std::string &uokey, std::string &err_code);

#endif      // AUTH_HTTP_HPP
