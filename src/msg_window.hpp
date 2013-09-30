#ifndef MSG_WINDOW_HPP
#define MSG_WINDOW_HPP
#define MSG_WINDOW_HPP_NAME "msg_window"

#include <ncursesw/ncurses.h>   // wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattrset_color(bool use_color, WINDOW *active_window, short color_p);

void show_buffer_1(std::string data_buf, WINDOW *active_window);

void show_buffer_2(char *data_buf, WINDOW *active_window);

#endif      // MSG_WINDOW_HPP
