#ifndef SOCKETS_HPP
#define SOCKETS_HPP
#define SOCKETS_HPP_NAME "sockets"

int socket_http(std::string host, std::string data_send, char *c_buffer, long &offset_recv, bool useirc = false);

int asyn_socket_send(std::string &data_send, int &socketfd);

int asyn_socket_recv(char *c_buffer, int bytes_recv, int &socketfd);

int socket_irc(std::string &zuousername, std::string &uokey);

#endif      // SOCKETS_HPP
