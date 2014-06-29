#ifndef WINDOW_UTILS_HPP
#define WINDOW_UTILS_HPP

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattron_color(WINDOW *win, bool use_colors, short color_p);

std::string get_time();

std::string time_unixtimestamp2local(std::string &time_unixtimestamp);

std::string time_unixtimestamp2local_full(std::string &time_unixtimestamp);

std::string time_sec2time(std::string &sec);

std::string key_utf2iso(int key_code);

void kbd_buf_show(std::string kbd_buf, std::string &zuousername, int term_y, int term_x, int kbd_buf_pos);

void win_buf_common(struct global_args &ga, std::string &win_buf, int pos_win_buf_start);

void win_buf_refresh(struct global_args &ga, struct channel_irc *chan_parm[]);

void win_buf_add_str(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string buffer_str, int act_type = 1,
			bool add_time = true);

void nicklist_on(struct global_args &ga);

std::string get_flags_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string nick);

void nicklist_refresh(struct global_args &ga, struct channel_irc *chan_parm[]);

void nicklist_off(struct global_args &ga);

void new_chan_status(struct global_args &ga, struct channel_irc *chan_parm[]);

void new_chan_debug_irc(struct global_args &ga, struct channel_irc *chan_parm[]);

bool new_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, bool active = true);

void del_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name);

void del_all_chan(struct channel_irc *chan_parm[]);

void new_or_update_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &chan_name, std::string nick, std::string zuo);

void update_nick_flags_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &chan_name, std::string nick, struct nick_flags flags);

void del_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string nick);

void erase_passwd_nick(std::string &kbd_buf, std::string &hist_buf, std::string &hist_ignore);

void destroy_my_password(struct global_args &ga);

#endif		// WINDOW_UTILS_HPP
