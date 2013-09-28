#ifndef SOCKETS_HPP
#define SOCKETS_HPP
#define SOCKETS_HPP_NAME "sockets"

#include <netdb.h>      // getaddrinfo(), freeaddrinfo(), socket()
//#include <fcntl.h>      // fcntl()
#include <unistd.h>     // close() - socket
//#include <sys/select.h> // select()
#include <ncursesw/ncurses.h>

int socket_http(std::string host, std::string data_send, char *buffer_recv, long &offset_recv);

void show_buffer_send(std::string data_send, WINDOW *active_room);

void show_buffer_recv(char *buffer_recv, WINDOW *active_room);

int asyn_socket_send(std::string data_send, int socketfd, WINDOW *active_room);

int asyn_socket_recv(char *buffer_recv, int bytes_recv, int socketfd);

int socket_irc(std::string &zuousername, std::string &uokey, int &socketfd_irc, WINDOW *active_room);

#endif      // SOCKETS_HPP
