//#include <cstring>      // strlen()
#include <string>       // std::string, setlocale()
#include <sys/select.h> // select()
#include "ncursesw/ncurses.h"   // wersja ncurses ze wsparciem dla UTF-8
#include "main_window.hpp"


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
        background_color = -1;
        font_color = -1;
    }

    init_pair(1, COLOR_RED, background_color);
    init_pair(2, COLOR_GREEN, background_color);
    init_pair(3, COLOR_YELLOW, background_color);
    init_pair(4, COLOR_BLUE, background_color);
    init_pair(5, COLOR_MAGENTA, background_color);
    init_pair(6, COLOR_CYAN, background_color);
    init_pair(7, font_color, background_color);

    return true;
}


void txt_color(bool use_colors, short color_font)
{
    if(use_colors)
        attrset(COLOR_PAIR(color_font));    // attrset() nadpisuje atrybuty, attron() dodaje atrybuty do istniejących
}


int main_window()
{
    setlocale(LC_ALL, "");  // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    if(! initscr())         // inicjalizacja ncurses
        return 1;

    bool use_colors;
    bool ucc_quit = false;  // aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
    int term_y, term_x;     // wymiary terminala
    int kbd_buf_pos = 0;    // początkowa pozycja bufora klawiatury (istotne podczas używania strzałek oraz Home i End)
    int kbd_buf_max = 0;    // początkowy maksymalny rozmiar bufora klawiatury
    int key_code;           // kod ostatnio wciśniętego klawisza
    std::string kbd_buf;    // bufor odczytanych znaków z klawiatury
    std::string key_code_tmp;   // tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)

    fd_set readfds;         // deskryptor dla select()
    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);    // klawiatura

    raw();                  // zablokuj Ctrl-C i Ctrl-Z
    keypad(stdscr, TRUE);   // klawisze funkcyjne będą obsługiwane
    noecho();               // nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)

    use_colors = check_colors();    // sprawdź, czy terminal obsługuje kolory, jeśli tak, włącz kolory oraz zainicjalizuj podstawową parę kolorów

    getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala


    if(use_colors)
        attron(COLOR_PAIR(4) | A_REVERSE);
    for(int i = 0; i < term_x; i++)
        printw(" ");

    move(term_y - 3, 0);
    for(int i = 0; i < term_x; i++)
        printw(" ");


    move(1, 0);
    txt_color(use_colors, 2);
    printw("Ucieszony Chat Client\n");

//    txt_color(use_colors, 7);
    attrset(COLOR_PAIR(7));

//    printw("Podaj nick tymczasowy:");

    move(term_y - 1, 0);

    refresh();

    do
    {
        getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala

        move(3, 0);
        printw("Wielkość bufora: %d, pozycja kursora: %d", kbd_buf_max, kbd_buf_pos);
        clrtoeol();

        // wyczyść przedostatni wiersz, aby przy zmianie rozmiaru okna terminala nie było śmieci
        move(term_y - 2, 0);
        clrtoeol();

        move(term_y - 1, 0);
        printw(">%s", kbd_buf.c_str());
        clrtoeol();
        move(term_y - 1, kbd_buf_pos + 1);      // + 1, bo na początku jest znak >

        refresh();

        select(STDIN + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(STDIN, &readfds))
        {
            refresh();

            key_code = getch();

            if(key_code == KEY_LEFT)
            {
                if(kbd_buf_pos > 0)
                    --kbd_buf_pos;
            }
            else if(key_code == KEY_RIGHT)
            {
                if(kbd_buf_pos < kbd_buf_max)
                    ++kbd_buf_pos;
            }
            else if(key_code == KEY_BACKSPACE)
            {
                if(kbd_buf_pos > 0)
                {
                    --kbd_buf_pos;
                    --kbd_buf_max;
                    kbd_buf.erase(kbd_buf_pos, 1);
                }
            }
            else if(key_code == KEY_DC)     // Delete
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
//
            else if(key_code == '\n')
            {
                move(2, 0);
                if(kbd_buf.size() != 0)
                {
                    printw("Wpisałeś: %s", kbd_buf.c_str());
                    if(kbd_buf == "/quit")
                    ucc_quit = true;
                    clrtoeol();
                    kbd_buf.clear();
                    kbd_buf_pos = 0;
                    kbd_buf_max = 0;
                }
                refresh();
            }
//
            else if(key_code >= 32 && key_code <= 255)   // do bufora odczytanych znaków wpisuj tylko te z zakresu 32...255
            {
                if(kbd_buf_max < 256)       // ogranicz pojemność bufora wejściowego
                {
                    key_code_tmp = key_code;
                    kbd_buf.insert(kbd_buf_pos, key_code_tmp);
                    ++kbd_buf_pos;
                    ++kbd_buf_max;
                }
            }

            move(4, 0);
            printw("Kod klawisza: %d", key_code);
            clrtoeol();

        }

    } while(! ucc_quit);

    endwin();       // zakończ tryb ncurses

    return 0;
}
