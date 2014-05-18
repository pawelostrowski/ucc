#ifndef WINDOW_UTILS_HPP
#define WINDOW_UTILS_HPP

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattron_color(WINDOW *win_chat, bool use_colors, short color_p);

std::string get_time();

std::string buf_utf2iso(std::string &buffer_str);

std::string buf_iso2utf(std::string &buffer_str);

void wprintw_buffer(WINDOW *win_chat, bool use_colors, std::string &buffer_str);

//void add_buffer();

std::string kbd_utf2iso(int key_code);

void kbd_buf_show(std::string kbd_buf, std::string &zuousername, int term_y, int term_x, int kbd_buf_pos);

std::string form_from_chat(std::string &buffer_irc_recv);

std::string onet_color_conv(std::string &onet_color);

void del_all_chan(struct channel_irc *chan_parm[]);

#endif		// WINDOW_UTILS_HPP
