#include <iostream>			// std::string, std::cout, std::cerr, std::endl

// -std=gnu++11 - perror()

#include "main_window.hpp"
#include "ucc_global.hpp"


int main(int argc, char *argv[])
{
	int window_status;

	// wartości domyślne (zostaną zmienione, jeśli w wierszu poleceń wpiszemy odpowiednie parametry)
	bool use_colors_main = true;	// domyślnie używaj kolorów w terminalu
	bool ucc_dbg_irc_main = false;	// domyślnie nie pracuj w trybie debugowania IRC

	std::string args[argc];

	// wczytaj wszystkie argumenty łącznie z ze ścieżką i nazwą programu
	for(int i = 0; i < argc; ++i)
	{
		args[i] = std::string(argv[i]);
	}

	// poprawić obsługę o wykrywanie błędnych argumentów
	for(int i = 0; i < argc; ++i)
	{
		// --color=on
		if(args[i] == "--colors=on")
		{
			// w zasadzie może tego w ogóle nie być, bo kolory są domyślnie włączone
		}

		else if(args[i] == "--colors=off")
		{
			use_colors_main = false;
		}

		// --dbg-irc
		else if(args[i] == "--dbg-irc")
		{
			ucc_dbg_irc_main = true;
		}

		// --help
		else if(args[i] == "--help")
		{
			std::cout << UCC_NAME " " UCC_VERSION << std::endl << std::endl;
			std::cout << "Opcje:" << std::endl;
			std::cout << "  --colors=on/off\twłącza/wyłącza kolory w programie (domyślnie włączone)" << std::endl;
			std::cout << "  --dbg-irc\t\twłącza debugowanie IRC w oknie \"Debug\"" << std::endl;
			std::cout << "  --help\t\twyświetla ten tekst pomocy i kończy działanie programu" << std::endl;

			return 0;
		}
	}

	window_status = main_window(use_colors_main, ucc_dbg_irc_main);

	if(window_status == 1)
	{
		perror("freopen()");
	}

	else if(window_status == 2)
	{
		std::cerr << "Nie udało się zainicjalizować biblioteki ncursesw!" << std::endl;
	}

	else if(window_status == 3)
	{
		perror("select()");
	}

	else if(window_status != 0)
	{
		std::cerr << "Wystąpił błąd numer: " << window_status << std::endl;
	}

	return window_status;
}
