#ifndef SOCKETS_HPP
#define SOCKETS_HPP
#define SOCKETS_HPP_NAME "sockets"

int socket_http(std::string host, std::string port, std::string data_send, char *c_buffer, long &offset_recv, bool useirc = false);

int socket_irc(std::string &zuousername, std::string &uokey);

#endif      // SOCKETS_HPP
