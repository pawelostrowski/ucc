#ifndef AUTH_HPP
#define AUTH_HPP

#define CAPTCHA_FILE "/tmp/ucc_captcha.gif"

// wersja appletu (trzeba ją wysłać podczas autoryzacji), od czasu do czasu można zmodyfikować na nowszą wersję
#define AP_VER "1.1(20130621-0052 - R)"

std::string auth_code(std::string &authkey);

bool auth_http_init(struct global_args &ga, struct channel_irc *chan_parm[]);

bool auth_http_getcaptcha(struct global_args &ga, struct channel_irc *chan_parm[]);

bool auth_http_getsk(struct global_args &ga, struct channel_irc *chan_parm[]);

bool auth_http_checkcode(struct global_args &ga, struct channel_irc *chan_parm[], std::string &captcha);

bool auth_http_mlogin(struct global_args &ga, struct channel_irc *chan_parm[]);

bool auth_http_useroverride(struct global_args &ga, struct channel_irc *chan_parm[]);

bool auth_http_getuokey(struct global_args &ga, struct channel_irc *chan_parm[]);

void auth_irc_all(struct global_args &ga, struct channel_irc *chan_parm[]);

#endif		// AUTH_HPP
