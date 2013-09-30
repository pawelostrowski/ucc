#include <string>           // std::string
#include <cstring>          // strlen()
#include "msg_window.hpp"


bool check_colors()
{
    if(has_colors() == FALSE)       // gdy nie da się używać kolorów, nie kończ programu, po prostu używaj czarno-białego terminala
        return false;

    if(start_color() == ERR)
        return false;

    short font_color = COLOR_WHITE;
    short background_color = COLOR_BLACK;

    if(use_default_colors() == OK)       // jeśli się da, dopasuj kolory do ustawień terminala
    {
        font_color = -1;
        background_color = -1;
    }

    init_pair(1, COLOR_RED, background_color);
    init_pair(2, COLOR_GREEN, background_color);
    init_pair(3, COLOR_YELLOW, background_color);
    init_pair(4, COLOR_BLUE, background_color);
    init_pair(5, COLOR_MAGENTA, background_color);
    init_pair(6, COLOR_CYAN, background_color);
    init_pair(7, COLOR_WHITE, background_color);
    init_pair(8, font_color, background_color);
    init_pair(9, COLOR_BLUE, COLOR_WHITE);

    return true;
}


void wattrset_color(WINDOW *active_window, bool use_color, short color_p)
{
    if(use_color)
        wattrset(active_window, COLOR_PAIR(color_p));    // wattrset() nadpisuje atrybuty, wattron() dodaje atrybuty do istniejących
    else
        wattrset(active_window, A_NORMAL);
}


void show_buffer_1(WINDOW *active_window, std::string &data_buf)
{
    int data_buf_len = data_buf.size();

    for(int i = 0; i < data_buf_len; ++i)
    {
        if(data_buf[i] != '\r')
            wprintw(active_window, "%c", data_buf[i]);
    }
}


void show_buffer_2(WINDOW *active_window, char *data_buf)
{
    int data_buf_len = strlen(data_buf);

    for(int i = 0; i < data_buf_len; ++i)
    {
        if(data_buf[i] != '\r')
            wprintw(active_window, "%c", data_buf[i]);
    }
}
