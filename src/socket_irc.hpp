#ifndef SOCKET_IRC_HPP
#define SOCKET_IRC_HPP

#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <fcntl.h>          // fnctl()
#include <unistd.h>         // close() - socket

int socket_irc_init(int &socketfd_irc, struct sockaddr_in &irc_info);

bool socket_irc_connect(int &socketfd_irc, struct sockaddr_in &irc_info);

bool socket_irc_send(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_send, std::string &msg_err);

bool socket_irc_recv(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err);

#endif      // SOCKET_IRC_HPP
