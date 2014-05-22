#ifndef AUTH_HPP
#define AUTH_HPP

void auth_code(std::string &authkey);

// szuka wartości pomiędzy wyrażeniem początkowym i końcowym
int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value);

bool http_auth_init(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_getcaptcha(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_getsk(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_checkcode(struct global_args &ga, struct channel_irc *chan_parm[], std::string &captcha);

bool http_auth_mlogin(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_useroverride(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_getuokey(struct global_args &ga, struct channel_irc *chan_parm[]);

bool irc_auth_1(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err);

bool irc_auth_2(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &zuousername, std::string &msg_err);

bool irc_auth_3(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &msg_err);

bool irc_auth_4(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &msg_err);

bool irc_auth_5(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_sent, std::string &zuousername, std::string &uokey, std::string &msg_err);

void irc_auth_all(struct global_args &ga, struct channel_irc *chan_parm[]);

#endif		// AUTH_HPP
