#include <iostream>         // std::cerr, std::endl
#include <cerrno>
//#include <iconv.h>          // konwersja kodowania znaków
#include "main_window.hpp"

int main(int argc, char *argv[])
{
    bool use_colors = true;     // domyślnie używaj kolorów w terminalu (jeśli terminal je obsługuje)
	int window_status;

    window_status = main_window(use_colors);
    if(window_status != 0)
    {
        if(window_status == 1)
            std::cerr << "Nie udało się zainicjalizować biblioteki ncursesw!" << std::endl;

        else if(window_status == 2)
            perror("Błąd w funkcji select()");

        else
            std::cerr << "Wystąpił błąd nr " << window_status << std::endl;

        std::cerr << "Kończenie." << std::endl;
        return window_status;
    }

    return 0;
}
