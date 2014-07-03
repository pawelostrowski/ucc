#ifndef NETWORK_HPP
#define NETWORK_HPP

#define BUF_SIZE (1500 * sizeof(char))

#define FF_VER "30.0"

int socket_init(struct global_args &ga, struct channel_irc *chan_parm[], std::string host, short port, std::string msg_dbg);

bool http_get_cookies(struct global_args &ga, struct channel_irc *chan_parm[], char *buffer_recv, std::string msg_dbg_http);

char *http_get_data(struct global_args &ga, struct channel_irc *chan_parm[], std::string method, std::string host, short port, std::string stock,
		std::string content, std::string &cookies, bool get_cookies, long &offset_recv, std::string msg_dbg_http);

void irc_send(struct global_args &ga, struct channel_irc *chan_parm[], std::string buffer_irc_send, std::string msg_dbg_irc = "");

void irc_recv(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_recv, std::string msg_dbg_irc = "");

#endif		// NETWORK_HPP
