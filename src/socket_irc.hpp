#ifndef SOCKET_IRC_HPP
#define SOCKET_IRC_HPP
#define SOCKET_IRC_HPP_NAME "socket_irc"

#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>         // close() - socket

int socket_irc_init(int &socketfd_irc, struct sockaddr_in &www);

bool socket_irc_connect(int &socketfd_irc, struct sockaddr_in &www);

bool socket_irc_send(int &socketfd_irc, bool &irc_ok, std::string &msg_sock, std::string &buffer_irc_send);

bool socket_irc_recv(int &socketfd_irc, bool &irc_ok, std::string &msg_sock, std::string &buffer_irc_recv);

#endif      // SOCKET_IRC_HPP
