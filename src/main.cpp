#include <iostream>		// std::cerr, std::endl
//#include <iconv.h>	// konwersja kodowania znaków
#include "main_window.hpp"


int main(int argc, char *argv[])
{
	int window_status;

    window_status = main_window();
    if(window_status != 0)
    {
        if(window_status == 1)
            std::cerr << "Nie udało się zainicjalizować biblioteki ncursesw!" << std::endl;

        else if(window_status == 2)
            std::cerr << "Błąd w funkcji select()" << std::endl;

        else
            std::cerr << "Wystąpił błąd nr " << window_status << std::endl;

        std::cerr << "Kończenie." << std::endl;
        return window_status;
    }

    return 0;
}
