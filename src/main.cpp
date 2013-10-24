#include <iostream>         // std::cerr, std::endl
#include <cstdio>           // perror()

//#include <sys/stat.h>

#include "main_window.hpp"


int main(int argc, char *argv[])
{
    int window_status;
    bool use_colors = true;     // domyślnie używaj kolorów w terminalu (jeśli terminal je obsługuje)
    bool ucc_dbg_irc = true;    // domyślnie pracuj w trybie debugowania IRC (potem zamienić to na parametr pobierany z argv[])

    window_status = main_window(use_colors, ucc_dbg_irc);

    if(window_status == 1)
        perror("freopen()");

    else if(window_status == 2)
        std::cerr << "Nie udało się zainicjalizować biblioteki ncursesw!" << std::endl;

    else if(window_status == 3)
        perror("select()");

    else if(window_status != 0)
        std::cerr << "Wystąpił błąd numer: " << window_status << std::endl;

//    std::string ucc_dir = std::string(getenv("HOME"));
//    char ucc_dir[] = "/.ucc";
//    char full_dir[120];
//    memcpy(full_dir, home_dir, strlen(home_dir));
//    memcpy(full_dir + strlen(home_dir), ucc_dir, strlen(ucc_dir));
//    full_dir[strlen(home_dir) + strlen(ucc_dir)] = '\0';
//    int st = mkdir(full_dir, 0755);
//    perror("mkdir");
//    std::cout << ucc_dir << std::endl;

    return window_status;
}
