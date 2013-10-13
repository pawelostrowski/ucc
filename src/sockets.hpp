#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <unistd.h>         // close() - socket

bool socket_init(int &socketfd, std::string host, short port, std::string &msg_err);

char *http_get_data(std::string method, std::string host, short port, std::string stock, std::string content, std::string &cookies, bool get_cookies, long &offset_recv, std::string &msg_err);

bool find_cookies(char *buffer_recv, std::string &cookies, std::string &msg_err);

bool irc_send(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_send, std::string &msg_err);

bool irc_recv(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err);

#endif      // SOCKETS_HPP
