#ifndef SOCKET_IRC_HPP
#define SOCKET_IRC_HPP
#define SOCKET_IRC_HPP_NAME "socket_irc"

int socket_irc_init(int &socketfd_irc);

int socket_irc_send(std::string data_send, int &socketfd_irc, bool irc_ok);

int socket_irc_recv(char *buffer_recv, int &socketfd_irc, bool irc_ok);

#endif      // SOCKET_IRC_HPP
