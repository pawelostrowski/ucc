/*
 *
 */

#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>		// system()
#include "auth.hpp"
#include "sockets.hpp"

using namespace std;


int main(int argc, char *argv[])
{
	string cookies, captcha_code, err_code;
	int http_status;

	cout << "Ucieszony Chat Client" << endl;

	http_status = http_1(cookies);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania http_1, kod błędu " << http_status << endl;
		return 1;
	}

	http_status = http_2(cookies);
	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << " podczas wywołania http_2, kod błędu " << http_status << endl;
		return 1;
	}

	system("eog /tmp/onetcaptcha.gif &");

	do
	{
		cout << "Przepisz kod z obrazka: ";
		getline(cin, captcha_code);
		if(captcha_code.size() != 6)
			cout << "Kod musi mieć 6 znaków!" << endl;
	} while(captcha_code.size() != 6);

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



	return 0;
}
