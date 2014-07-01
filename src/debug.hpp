#ifndef DEBUG_HPP
#define DEBUG_HPP

void dbg_http_to_file(struct global_args &ga, std::string dbg_sent, std::string dbg_recv, std::string &host, short port, std::string &stock,
			std::string &msg_dbg_http);

void dbg_irc_sent_to_file(struct global_args &ga, std::string dbg_sent);

void dbg_irc_recv_to_file(struct global_args &ga, std::string &dbg_recv);

#endif		// DEBUG_HPP
