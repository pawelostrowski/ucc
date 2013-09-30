#ifndef MSG_WINDOW_HPP
#define MSG_WINDOW_HPP
#define MSG_WINDOW_HPP_NAME "msg_window"

// przypisz własne nazwy kolorów dla zainicjalizowanych par kolorów
#define UCC_RED         1
#define UCC_GREEN       2
#define UCC_YELLOW      3
#define UCC_BLUE        4
#define UCC_MAGENTA     5
#define UCC_CYAN        6
#define UCC_WHITE       7
#define UCC_TERM        8   // kolor zależny od ustawień terminala
#define UCC_BLUE_WHITE  9   // niebieski font, białe tło

#include <ncursesw/ncurses.h>   // wersja ncurses ze wsparciem dla UTF-8

bool check_colors();

void wattrset_color(WINDOW *active_window, bool use_color, short color_p);

void show_buffer_1(WINDOW *active_window, std::string &data_buf);

void show_buffer_2(WINDOW *active_window, char *data_buf);

#endif      // MSG_WINDOW_HPP
