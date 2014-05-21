#ifndef NETWORK_HPP
#define NETWORK_HPP

// debugowanie HTTP (1 - zapisuje plik na dysku)
#define DBG_HTTP 1

bool find_cookies(char *buffer_recv, std::string &cookies, std::string &msg_err);

int socket_init(std::string host, short port, std::string &msg_err);

char *http_get_data(std::string method, std::string host, short port, std::string stock, std::string content, std::string &cookies, bool get_cookies,
		    long &offset_recv, std::string &msg_err_pre, std::string &msg_err);

bool irc_send(int &socketfd_irc, bool &irc_ok, std::string buffer_irc_send, std::string &msg_err);

bool irc_recv(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err);

#endif		// NETWORK_HPP
