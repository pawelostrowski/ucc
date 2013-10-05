#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP_NAME "main_window"

#include <ncursesw/ncurses.h>   // wersja ncurses ze wsparciem dla UTF-8

int main_window(bool use_colors);

bool check_colors();

void wattrset_color(WINDOW *active_window, bool use_colors, short color_p);

void display_buffer(WINDOW *active_window, bool use_colors, short color_p, std::string buffer_out);

void kbd_utf2iso(int &key_code);

void wprintw_iso2utf(WINDOW *active_window, bool use_colors, short color_p, int cur_y, int cur_x, std::string buffer_str);

#endif      // MAIN_WINDOW_HPP
