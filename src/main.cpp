/*
 * Opis dodam, jak kod się bardziej rozwinie.
 */

#include <iostream>			// std::cin, std::cout, std::string

#include <stdlib.h>		// system()
//#include <iconv.h>		// konwersja kodowania znaków

#include "sockets.hpp"

using namespace std;


int main(int argc, char *argv[])
{
	string nick, cookies, captcha_code, err_code, uokey, zuousername;
	int http_status;

	cout << "Ucieszony Chat Client" << endl;

	do
	{
		cout << "Podaj nick tymczasowy: ";
		getline(cin, nick);
		while(nick.find(" ") != string::npos)
			nick.erase(nick.find(" "), 1);		// usuń spacje z nicka (nie może istnieć taki nick)
	} while(! nick.size());

	cout << "Pobieranie obrazka z kodem do przepisania... " << endl;

	cout << "http_1()" << endl;
	http_status = http_1(cookies);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania http_1, kod błędu " << http_status << endl;
		return 1;
	}

	cout << "http_2()" << endl;
	http_status = http_2(cookies);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania http_2, kod błędu " << http_status << endl;
		return 1;
	}

	int eog_stat = system("eog /tmp/onetcaptcha.gif 2>/dev/null &");
	cout << eog_stat << endl;

	do
	{
		cout << "Przepisz kod z obrazka: ";
		getline(cin, captcha_code);
		if(captcha_code.size() != 6)
			cout << "Kod musi mieć 6 znaków!" << endl;
	} while(captcha_code.size() != 6);

	cout << "http_3()" << endl;
	http_status = http_3(cookies, captcha_code, err_code);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania http_3, kod błędu " << http_status << endl;
		return 1;
	}

	if(err_code == "FALSE")
	{
		cout << "Wpisany kod jest błędny!" << endl << "Zakończono." << endl;
		return 0;		// 0, bo to nie jest błąd programu
	}

	cout << "http_4()" << endl;
	http_status = http_4(cookies, nick, zuousername, uokey, err_code);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania http_4, kod błędu " << http_status << endl;
		return 1;
	}

	if(err_code != "TRUE")
	{
		cout << "Błąd serwera (nieprawidłowy nick?): " << err_code << endl;
		cout << "Zakończono." << endl;
		return 0;		// 0, bo to nie jest błąd programu
	}

	cout << "irc()" << endl;
	http_status = irc(zuousername, uokey);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania irc, kod błędu " << http_status << endl;
		return 1;
	}

	return 0;
}
