#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP_NAME "main_window"

//#undef FD_SETSIZE
//#define FD_SETSIZE      10240

#include <ncursesw/ncurses.h>   // wersja ncurses ze wsparciem dla UTF-8

int main_window(bool use_colors);

bool check_colors();

void wattrset_color(WINDOW *active_window, bool use_color, short color_p);

void show_buffer_1(WINDOW *active_window, std::string &data_buf, bool use_colors);

void show_buffer_2(WINDOW *active_window, char *data_buf, bool use_colors);

#endif      // MAIN_WINDOW_HPP
