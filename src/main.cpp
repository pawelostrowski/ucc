/*
 *
 */

#include <iostream>
#include <string>
#include <cstring>
#include "auth.hpp"
#include "sockets.hpp"

using namespace std;


int main(int argc, char *argv[])
{
/*
	char authkey[16];
	strcpy(authkey, argv[1]);	// skopiuj argv[1] do authkey

	if(!auth(authkey))		// w tym sprawdzeniu gdy funkcja wykona się poprawnie, w *authkey będzie przekonwertowany klucz autoryzacji
	{
		cerr << argv[0] << ": Nieprawidłowy rozmiar AUTHKEY!" << endl;
		return 1;
	}

	cout << authkey; << endl;
*/


/*
	char host[100];
	strcpy(host, argv[2]);
	char port[10];
	strcpy(port, argv[3]);
	char getdata[200] = "GET / HTTP/1.1\r\n\Host: ";
	char getdata_end[] = "\r\nConnection: Close\r\n\r\n";

	// doklejenie do końca getdata zawartości host
	int j = strlen(getdata);
	for(int i = 0; i < strlen(host); i++)
	{
		getdata[j] = host[i];
		j++;
	}

	// doklejenie do końca getdata zawartości getdata_end
	j = strlen(getdata);
	for(int i = 0; i < strlen(getdata_end); i++)
	{
		getdata[j] = getdata_end[i];
		j++;
	}

*/

/*
	string cookies_send;

	int http_status = http(host, port, getdata, cookies_recv);

	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_H_NAME << ", kod błędu " << http_status << endl;
		return 1;
	}

	cout << cookies_recv << endl;


	http_status = http(host, port, getdata, cookies_recv);

	if(http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_H_NAME << ", kod błędu " << http_status << endl;
		return 1;
	}

	cout << cookies_recv << endl;
*/

	string cookies;
	http_1(cookies);
	int http_status = http_2(cookies);
	if (http_status != 0)
	{
		cerr << "Błąd w module " << SOCKETS_HPP_NAME << ", kod błędu " << http_status << endl;
		return 1;
	}

	cout << cookies << endl;

	return 0;
}
