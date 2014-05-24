#ifndef WINDOW_UTILS_HPP
#define WINDOW_UTILS_HPP

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattron_color(WINDOW *win_chat, bool use_colors, short color_p);

std::string get_time();

std::string key_utf2iso(int key_code);

void kbd_buf_show(std::string kbd_buf, std::string &zuousername, int term_y, int term_x, int kbd_buf_pos);

void win_buf_common(struct global_args &ga, std::string &buffer_str, int pos_buf_str_start);

void win_buf_refresh(struct global_args &ga, std::string &buffer_str);

void add_show_win_buf(struct global_args &ga, struct channel_irc *chan_parm[], std::string buffer_str, bool show_buf = true);

void add_act_chan(struct channel_irc *chan_parm[], std::string chan_name, int act_type);

void new_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, bool chan_ok = false);

void del_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[]);

void del_all_chan(struct channel_irc *chan_parm[]);

void add_show_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string buffer_str);

void destroy_my_password(struct global_args &ga);

#endif		// WINDOW_UTILS_HPP
