#ifndef WINDOW_UTILS_HPP
#define WINDOW_UTILS_HPP

#include <ncursesw/ncurses.h>   // wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattrset_color(WINDOW *active_window, bool use_colors, short color_p);

void get_time(char *time_hms);

void kbd_utf2iso(int &key_code);

void kbd_buf_show(std::string kbd_buf, std::string zuousername, int term_y, int term_x, int kbd_buf_pos);

void wprintw_utf(WINDOW *active_window, bool use_colors, short color_p, std::string buffer_str);

void wprintw_iso2utf(WINDOW *active_window, bool use_colors, short color_p, std::string buffer_str, bool bold_my_nick = false);

short onet_color_conv(std::string onet_color);

#endif      // WINDOW_UTILS_HPP