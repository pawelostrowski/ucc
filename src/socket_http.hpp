#ifndef SOCKET_HTTP_HPP
#define SOCKET_HTTP_HPP
#define SOCKET_HTTP_HPP_NAME "socket_http"

int socket_http(std::string method, std::string host, std::string stock, std::string content, std::string &cookies, char *buffer_recv, long &offset_recv);

int find_cookies(char *buffer_recv, std::string &cookies);

#endif      // SOCKET_HTTP_HPP
