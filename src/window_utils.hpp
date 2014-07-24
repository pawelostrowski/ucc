#ifndef WINDOW_UTILS_HPP
#define WINDOW_UTILS_HPP

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattron_color(WINDOW *win, bool use_colors, short color_p);

std::string get_time();

std::string time_unixtimestamp2local(std::string &time_unixtimestamp);

std::string time_unixtimestamp2local_full(std::string &time_unixtimestamp);

std::string time_sec2time(std::string &sec);

int line_size(struct global_args &ga, struct channel_irc *chan_parm[], int line_number);

std::string key_utf2iso(int key_code);

void kbd_buf_show(std::string kbd_buf, std::string &zuousername, int term_y, int term_x, int kbd_buf_pos);

void win_buf_show(struct global_args &ga, struct channel_irc *chan_parm[], int line_number);

void win_buf_refresh(struct global_args &ga, struct channel_irc *chan_parm[]);

void win_buf_add_str(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string in_buf, int act_type = 1,
			bool add_time = true, bool only_chan_normal = true);

void win_buf_all_chan_msg(struct global_args &ga, struct channel_irc *chan_parm[], std::string msg);

void nicklist_on(struct global_args &ga);

std::string get_flags_nick(struct global_args &ga, struct channel_irc *chan_parm[], std::string nick_key);

void nicklist_refresh(struct global_args &ga, struct channel_irc *chan_parm[]);

void nicklist_off(struct global_args &ga);

#endif		// WINDOW_UTILS_HPP
