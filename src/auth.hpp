#ifndef AUTH_HPP
#define AUTH_HPP

#define CAPTCHA_FILE "/tmp/ucc_captcha.gif"

// wersja appletu (trzeba ją wysłać podczas autoryzacji), od czasu do czasu można zmodyfikować na nowszą wersję
#define AP_VER "1.1(20130621-0052 - R)"

std::string auth_code(std::string &authkey);

bool http_auth_init(struct global_args &ga, std::string &msg);

bool http_auth_getcaptcha(struct global_args &ga, std::string &msg);

bool http_auth_getsk(struct global_args &ga, std::string &msg);

bool http_auth_checkcode(struct global_args &ga, std::string &captcha, std::string &msg);

bool http_auth_mlogin(struct global_args &ga, std::string &msg);

bool http_auth_useroverride(struct global_args &ga, std::string &msg);

bool http_auth_getuokey(struct global_args &ga, std::string &msg);

void irc_auth_all(struct global_args &ga, struct channel_irc *chan_parm[]);

#endif		// AUTH_HPP
