/*
	Ucieszony Chat Client
	Copyright (C) 2013, 2014 Paweł Ostrowski

	This file is part of Ucieszony Chat Client.

	Ucieszony Chat Client is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	Ucieszony Chat Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Ucieszony Chat Client (in the file LICENSE); if not,
	see <http://www.gnu.org/licenses/gpl-2.0.html>.
*/


#include <iostream>			// std::string, std::cout, std::cerr, std::endl

// -std=c++11 - std::perror()

#include "main_window.hpp"
#include "ucc_global.hpp"


void ucc_help()
{
	std::cout << std::endl << "Opcje:" << std::endl;
	std::cout << "  -n, --no-colors           Wyłącza kolory w programie" << std::endl;
	std::cout << "  -d, --debug-irc           Włącza debugowanie IRC w oknie [DebugIRC]" << std::endl;
	std::cout << "  -h, --help                Wyświetla ten tekst pomocy i kończy działanie programu" << std::endl;
	std::cout << "  -v, --version             Wyświetla informację o wersji i kończy działanie programu" << std::endl;
}


void ucc_version()
{
	std::cout << UCC_NAME " " UCC_VER << std::endl;
	std::cout << "Copyright (C) 2013, 2014 Paweł Ostrowski" << std::endl;
	std::cout << "Licencja: GNU General Public License v2.0 lub późniejsze wersje" << std::endl;
}


int main(int argc, char *argv[])
{
	// wartości domyślne (zostaną zmienione, jeśli w wierszu poleceń wpiszemy odpowiednie parametry)
	bool _use_colors = true;		// domyślnie używaj kolorów w terminalu
	bool _debug_irc = false;		// domyślnie nie pracuj w trybie debugowania IRC

	std::string args[argc];

	// wczytaj wszystkie argumenty łącznie ze ścieżką i nazwą programu
	for(int i = 0; i < argc; ++i)
	{
		args[i] = std::string(argv[i]);
	}

	for(int i = 1; i < argc; ++i)
	{
		if(args[i] == "-n" || args[i] == "--no-colors")
		{
			_use_colors = false;
		}

		else if(args[i] == "-d" || args[i] == "--debug-irc")
		{
			_debug_irc = true;
		}

		else if(args[i] == "-h" || args[i] == "--help")
		{
			ucc_version();
			ucc_help();

			return 0;
		}

		else if(args[i] == "-v" || args[i] == "--version")
		{
			ucc_version();

			return 0;
		}

		else
		{
			ucc_version();
			std::cout << std::endl << "Nieznana opcja: \'" << args[i] << "\'" << std::endl;
			ucc_help();

			return 10;
		}
	}

	int window_status = main_window(_use_colors, _debug_irc);

	if(window_status == 1)
	{
		std::perror("freopen()");
	}

	else if(window_status == 2)
	{
		std::cerr << "Nie udało się zainicjalizować biblioteki ncursesw!" << std::endl;
	}

	else if(window_status == 3)
	{
		std::perror("select()");
	}

	return window_status;
}
