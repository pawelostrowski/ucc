#include <string>		// std::string
#include <cstring>		// memset(), strlen(), strstr()
#include <fstream>		// pliki
//#include <sys/socket.h>
#include <netdb.h>		// getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>		// close() - socket
#include "sockets.h"

using namespace std;


int socket_a(string &host, string &port, string &data_send, char *c_buffer)
{
	int socketfd; // The socket descripter
	ssize_t bytes_recv;

	struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
	struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

	memset(&host_info, 0, sizeof host_info);

	host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
	host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

	// zapis przykładowo host.c_str() oznacza, że string zostaje zamieniony na const char
	if(getaddrinfo(host.c_str(), port.c_str(), &host_info, &host_info_list) != 0)	// pobierz status adresu
		return 1;		// kod błedu przy niepowodzeniu w pobraniu statusu adresu

	socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);		// utwórz socket

	if(socketfd == -1)
	{
		freeaddrinfo(host_info_list);
		return 2;		// kod błedu przy niepowodzeniu w tworzeniu socketa
	}

	if(connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1)	// pobierz status połączenia do hosta
	{
		freeaddrinfo(host_info_list);
		close(socketfd);				// zamknij połączenie z hostem
		return 3;		// kod błędu przy niepowodzeniu połączenia do hosta
	}

	send(socketfd, data_send.c_str(), strlen(data_send.c_str()), 0);		// wysłanie zapytania (np. GET /<...>)

	bytes_recv = recv(socketfd, c_buffer, 2000, 0);		// pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów

	if(bytes_recv == 0)
	{
		freeaddrinfo(host_info_list);
		close(socketfd);				// zamknij połączenie z hostem
		return 4;		// kod błędu przy pobraniu zerowej ilości bajtów (możliwy powód: host został wyłączony)
	}

	if(bytes_recv == -1)
	{
		freeaddrinfo(host_info_list);
		close(socketfd);				// zamknij połączenie z hostem
		return 5;		// kod błędu przy niepowodzeniu w pobieraniu danych od hosta
	}

	freeaddrinfo(host_info_list);
	close(socketfd);				// zamknij połączenie z hostem

	return 0;
}


int find_cookies(char *c_buffer, string &cookies)
{
	size_t cookie_start, cookie_end;
	string cookie_tmp;
	string cookie_string = "Set-Cookie:";

	// string(c_buffer) zamienia C string na std::string
	cookie_start = string(c_buffer).find(cookie_string);		// znajdź pozycję pierwszego cookie (bez pominięcia "Set-Cookie:")
	if(cookie_start == string::npos)
		return 6;		// kod błędu, gdy nie znaleziono cookie (pierwszego)

	do
	{
		cookie_end = string(c_buffer).find(";", cookie_start);		// szukaj ";" od pozycji początku cookie
		if(cookie_end == string::npos)
			return 7;	// kod błędu, gdy nie znaleziono oczekiwanego ";" na końcu każdego cookie

		cookie_tmp.clear();			// wyczyść bufor pomocniczy
		cookie_tmp.insert(0, string(c_buffer), cookie_start + cookie_string.size(), cookie_end - cookie_start - cookie_string.size() + 1);	// skopiuj cookie do bufora pomocniczego
		cookies += cookie_tmp;			// dopisz kolejny cookie do bufora
		cookie_start = string(c_buffer).find(cookie_string, cookie_start + cookie_string.size());		// znajdź kolejny cookie
	} while(cookie_start != string::npos);

	return 0;
}


int http_1(string &cookies)
{
	int socket_status, cookies_status;
	char c_buffer[2000];

	string host = "kropka.onet.pl";
	string port = "80";
	string data_send = 	"GET /_s/kropka/1?DV=czat/applet/FULL HTTP/1.1\r\n"
						"Host: " + host + "\r\n"
						"Connection: Close\r\n\r\n";

	socket_status = socket_a(host, port, data_send, c_buffer);

	if(socket_status != 0)
		return socket_status;		// kod błędu, gdy napotkano problem z socketem

	cookies.clear();		// wyczyść bufor cookies
	cookies_status = find_cookies(c_buffer, cookies);

	if(cookies_status != 0)
		return cookies_status;		// kod błędu, gdy napotkano problem z cookies

	return 0;
}


int http_2(string &cookies)
{
	int socket_status, cookies_status;
	char c_buffer[2000];

	string host = "czat.onet.pl";
	string port = "80";
	string data_send = 	"GET /myimg.gif HTTP/1.1\r\n"
						"Host: " + host + "\r\n"
						"Connection: Close\r\n"
						"Cookie:" + cookies + "\r\n\r\n";

	socket_status = socket_a(host, port, data_send, c_buffer);

	if(socket_status != 0)
		return socket_status;		// kod błędu, gdy napotkano problem z socketem

	cookies_status = find_cookies(c_buffer, cookies);

	if(cookies_status != 0)
		return cookies_status;		// kod błędu, gdy napotkano problem z cookies

	return 0;
}


int irc()
{
	return 0;
}
