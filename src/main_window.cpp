#include <iostream>
#include <cstring>      // strlen()
#include <sstream>
#include <sys/select.h> // select()
#include "ncursesw/ncurses.h"
#include "main_window.hpp"

#define COLOR_BACKGROUND COLOR_BLACK
#define STDIN 0


bool check_colors()
{
    if(! has_colors())
        return false;

    start_color();

    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_RED, COLOR_BACKGROUND);
    init_pair(3, COLOR_GREEN, COLOR_BACKGROUND);
    init_pair(4, COLOR_YELLOW, COLOR_BACKGROUND);
    init_pair(5, COLOR_BLUE, COLOR_BACKGROUND);
    init_pair(6, COLOR_MAGENTA, COLOR_BACKGROUND);
    init_pair(7, COLOR_CYAN, COLOR_BACKGROUND);
    init_pair(8, COLOR_WHITE, COLOR_BACKGROUND);

    return true;
}


void txt_color(bool use_colors, short color_font)
{
    if(use_colors)
        attron(COLOR_PAIR(color_font));
}


int main_window()
{
    setlocale(LC_ALL, "");  // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    if(! initscr())
    {
        std::cerr << "Nie udało się zainicjalizować biblioteki ncurses!" << std::endl;
        return 1;
    }

    bool use_colors;
    int term_y, term_x;     // wymiary terminala

    raw();
    keypad(stdscr, TRUE);
    noecho();

    use_colors = check_colors();

    getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala

    txt_color(use_colors, 4);
    printw("Ucieszony Chat Client\n");

    printw("Podaj nick tymczasowy:");

    move(term_y - 1, 0);

    refresh();


    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);    // klawiatura

    int kbd_buf_pos = 0, kbd_buf_max = 0;
    int key_code;
    std::string kbd_buf, key_code_tmp;

    do
    {
        getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala

        move(6, 0);
        printw("Pozycja kursora: %d", kbd_buf_pos);
        clrtoeol();

        // wyczyść przedostatni wiersz, aby przy zmianie rozmiaru okna terminala nie było śmieci
        move(term_y - 2, 0);
        clrtoeol();

        move(term_y - 1, 0);
        printw("%s", kbd_buf.c_str());
        clrtoeol();
        move(term_y - 1, kbd_buf_pos);

        refresh();

        select(STDIN + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(STDIN, &readfds))
        {
            refresh();

            key_code = getch();

            if(key_code == KEY_BACKSPACE)
            {
                if(kbd_buf_pos > 0)
                {
                    --kbd_buf_pos;
                    --kbd_buf_max;
                    kbd_buf.erase(kbd_buf_pos, 1);
                }
            }
            else if(key_code == KEY_DC)     // klawisz Delete
            {
                if(kbd_buf_pos < kbd_buf_max)
                {
                    --kbd_buf_max;
                    kbd_buf.erase(kbd_buf_pos, 1);
                }
            }
            else if(key_code == KEY_HOME)
            {
                kbd_buf_pos = 0;
            }
            else if(key_code == KEY_END)
            {
                kbd_buf_pos = kbd_buf_max;
            }
            else if(key_code == KEY_LEFT)
            {
                if(kbd_buf_pos > 0)
                    --kbd_buf_pos;
            }
            else if(key_code == KEY_RIGHT)
            {
                if(kbd_buf_pos < kbd_buf_max)
                    ++kbd_buf_pos;
            }
//
            else if(key_code == '\n')
            {
                move(10, 0);
                printw("Wpisałeś: %s", kbd_buf.c_str());
                    if(kbd_buf == "quit")
                    {
                        endwin();
                        return 0;
                    }
                clrtoeol();
                kbd_buf.clear();
                kbd_buf_pos = 0;
                kbd_buf_max = 0;
                refresh();
            }
//
            else if(key_code < 256)
            {
                if(key_code >= 32)
                {
                    key_code_tmp = key_code;
                    kbd_buf.insert(kbd_buf_pos, key_code_tmp);
                    ++kbd_buf_pos;
                    ++kbd_buf_max;
                }
            }

            move(7, 0);
            printw("Kod klawisza: %d", key_code);
            clrtoeol();

        }

    } while(true);


    endwin();

    return 0;
}
