#ifndef SOCKET_IRC_HPP
#define SOCKET_IRC_HPP
#define SOCKET_IRC_HPP_NAME "socket_irc"

#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>         // close() - socket

int socket_irc_init(int &socketfd_irc, struct sockaddr_in &www);

void socket_irc_connect(int &socketfd_irc, struct sockaddr_in &www, std::string &msg, short &msg_color, char *buffer_irc_recv);

int socket_irc_send(int &socketfd_irc, bool &irc_ok, std::string data_send, std::string &data_sent);

int socket_irc_recv(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv);

#endif      // SOCKET_IRC_HPP
