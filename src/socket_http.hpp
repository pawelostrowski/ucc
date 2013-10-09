#ifndef SOCKET_HTTP_HPP
#define SOCKET_HTTP_HPP
#define SOCKET_HTTP_HPP_NAME "socket_http"

bool socket_http(std::string method, std::string host, std::string stock, std::string content, std::string &cookies, bool get_cookies,
                 char *buffer_recv, long &offset_recv, std::string &msg_err);

bool find_cookies(char *buffer_recv, std::string &cookies, std::string &msg_err);

#endif      // SOCKET_HTTP_HPP
