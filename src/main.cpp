/*
 * Opis dodam, jak kod się bardziej rozwinie.
 */

#include <iostream>		// std::cin, std::cout, std::string
#include <cstdlib>		// system()
//#include <iconv.h>	// konwersja kodowania znaków
#include "main_window.hpp"
#include "auth_http.hpp"
#include "sockets.hpp"

// Celowo nie używam 'using namespace std;', aby pokazać, gdzie używane są obiekty standardowych bibliotek C++


int main(int argc, char *argv[])
{
	std::string nick, cookies, captcha_code, err_code, zuousername, uokey;
	int window_status, http_status;

/*
//	std::cout << "Ucieszony Chat Client" << std::endl;
    window_status = main_window();
    if(! window_status)
        return window_status;

        return 0;
*/

	do
	{
		std::cout << "Podaj nick tymczasowy: ";
		std::cin.clear();		// zapobiega zapętleniu się programu przy kombinacji Ctrl-D
		getline(std::cin, nick);
		if(std::cin.eof() == 1)		// jeśli wciśnięto Ctrl-D, przejdź do nowego wiersza i ponownie wypisz komunikat
			std::cout << std::endl;
		while(nick.find(" ") != std::string::npos)
			nick.erase(nick.find(" "), 1);		// usuń spacje z nicka (nie może istnieć taki nick)
	} while(nick.size() == 0);

	std::cout << "Pobieranie obrazka z kodem do przepisania... " << std::endl;


	std::cout << "http_1()" << std::endl;
	http_status = http_1(cookies);
	if(http_status != 0)
	{
		std::cerr << "Błąd w http_1(), kod błędu " << http_status << std::endl;
		return 1;
	}

	std::cout << "http_2()" << std::endl;
	http_status = http_2(cookies);
	if(http_status != 0)
	{
		std::cerr << "Błąd w http_2(), kod błędu " << http_status << std::endl;
		return 1;
	}

	int eog_stat = system("/usr/bin/eog "FILE_GIF" 2>/dev/null &");	// to do poprawy, rozwiązanie tymczasowe!!!
	std::cout << eog_stat << std::endl;

	do
	{
		std::cout << "Przepisz kod z obrazka: ";
		std::cin.clear();		// zapobiega zapętleniu się programu przy kombinacji Ctrl-D
		getline(std::cin, captcha_code);
		if(std::cin.eof() == 1)		// jeśli wciśnięto Ctrl-D, przejdź do nowego wiersza i ponownie wypisz komunikat
			std::cout << std::endl;
		if(captcha_code.size() != 6)
			std::cout << "Kod musi mieć 6 znaków!" << std::endl;
	} while(captcha_code.size() != 6);

	std::cout << "http_3()" << std::endl;
	http_status = http_3(cookies, captcha_code, err_code);
	if(http_status != 0)
	{
		std::cerr << "Błąd w http_3(), kod błędu " << http_status << std::endl;
		return 1;
	}

	if(err_code == "FALSE")
	{
		std::cout << "Wpisany kod jest błędny!" << std::endl << "Zakończono." << std::endl;
		return 0;		// 0, bo to nie jest błąd programu
	}

	std::cout << "http_4()" << std::endl;
	http_status = http_4(cookies, nick, zuousername, uokey, err_code);
	if(http_status != 0)
	{
		std::cerr << "Błąd w http_4(), kod błędu " << http_status << std::endl;
		return 1;
	}

	if(err_code != "TRUE")
	{
		std::cout << "Błąd serwera (nieprawidłowy nick?): " << err_code << std::endl;
		std::cout << "Zakończono." << std::endl;
		return 0;		// 0, bo to nie jest błąd programu
	}

	std::cout << "socket_irc()" << std::endl;
	http_status = socket_irc(zuousername, uokey);
	if(http_status != 0)
	{
		std::cerr << "Błąd w socket_irc(), kod błędu " << http_status << std::endl;
		return 1;
	}

	return 0;
}
