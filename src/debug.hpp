#ifndef DEBUG_HPP
#define DEBUG_HPP

void http_dbg_to_file(struct global_args &ga, std::string dbg_sent, std::string dbg_recv, std::string &host, short port, std::string &stock,
			std::string &msg_dbg_http);

void irc_sent_dbg_to_file(struct global_args &ga, std::string dbg_sent);

void irc_recv_dbg_to_file(struct global_args &ga, std::string &dbg_recv);

#endif		// DEBUG_HPP
