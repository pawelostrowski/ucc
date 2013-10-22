#include <iostream>         // std::cerr, std::endl
#include <cstdio>           // perror()

#include "main_window.hpp"


int main(int argc, char *argv[])
{
    bool use_colors = true;     // domyślnie używaj kolorów w terminalu (jeśli terminal je obsługuje)
    bool ucc_dbg_irc = true;    // domyślnie pracuj w trybie debugowania IRC (potem zamienić to na parametr pobierany z argv[])
    int window_status;

    window_status = main_window(use_colors, ucc_dbg_irc);

    if(window_status == 1)
        perror("freopen()");

    else if(window_status == 2)
        std::cerr << "Nie udało się zainicjalizować biblioteki ncursesw!" << std::endl;

    else if(window_status == 3)
        perror("select()");

    else if(window_status != 0)
        std::cerr << "Wystąpił błąd numer: " << window_status << std::endl;

    return 0;
}
