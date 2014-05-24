#ifndef AUTH_HPP
#define AUTH_HPP

#define FILE_GIF "/tmp/ucc_captcha.gif"

void auth_code(std::string &authkey);

bool http_auth_init(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_getcaptcha(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_getsk(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_checkcode(struct global_args &ga, struct channel_irc *chan_parm[], std::string &captcha);

bool http_auth_mlogin(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_useroverride(struct global_args &ga, struct channel_irc *chan_parm[]);

bool http_auth_getuokey(struct global_args &ga, struct channel_irc *chan_parm[]);

void irc_auth(struct global_args &ga, struct channel_irc *chan_parm[]);

#endif		// AUTH_HPP
