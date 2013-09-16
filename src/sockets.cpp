#include <string>		// std::string
#include <cstring>		// memset(), strlen(), strstr()
#include <sstream>		// strumień do string
#include <fstream>		// pliki
//#include <sys/socket.h>
#include <netdb.h>		// getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>		// close() - socket
#include "sockets.hpp"

using namespace std;
//	#include <iostream>


int socket_a(string &host, string &port, string &data_send, char *c_buffer, long &offset_recv)
{
	int socketfd; // The socket descripter
	ssize_t bytes_recv;
	char tmp_buffer[2000];
	bool first_recv = true;		// czy pierwsze pobranie w pętli

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

	// poniższa pętla pobiera dane z hosta do bufora
	offset_recv = 0;		// offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
	do
	{
		bytes_recv = recv(socketfd, tmp_buffer, 2000, 0);		// pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
		if(first_recv)
		{
			if(bytes_recv == 0)
			{
				freeaddrinfo(host_info_list);
				close(socketfd);				// zamknij połączenie z hostem
				return 4;		// kod błędu przy pobraniu zerowej ilości bajtów (możliwy powód: host został wyłączony)
			}
		}
		first_recv = false;		// kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
		memcpy(c_buffer + offset_recv, tmp_buffer, bytes_recv);		// pobrane dane "dopisz" do bufora
		offset_recv += bytes_recv;		// zwiększ offset
	} while(bytes_recv != 0);


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
	long offset_recv;

	string host = "kropka.onet.pl";
	string port = "80";
	string data_send = 	"GET /_s/kropka/1?DV=czat/applet/FULL HTTP/1.1\r\n"
						"Host: " + host + "\r\n"
						"Connection: Keep-Alive\r\n\r\n";

	socket_status = socket_a(host, port, data_send, c_buffer, offset_recv);
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
	char c_buffer[100000];				// TO JAKOŚ POTEM PRZEROBIĆ, TYMCZASOWO TAK JEST
	long offset_recv;
	char *gif_buffer;

	string host = "czat.onet.pl";
	string port = "80";
	string data_send = 	"GET /myimg.gif HTTP/1.1\r\n"
						"Host: " + host + "\r\n"
						"Connection: Keep-Alive\r\n"
						"Cookie:" + cookies + "\r\n\r\n";

	socket_status = socket_a(host, port, data_send, c_buffer, offset_recv);
	if(socket_status != 0)
		return socket_status;		// kod błędu, gdy napotkano problem z socketem

	cookies_status = find_cookies(c_buffer, cookies);
	if(cookies_status != 0)
		return cookies_status;		// kod błędu, gdy napotkano problem z cookies

	gif_buffer = strstr(c_buffer, "GIF");
	if(gif_buffer)
	{
		ofstream gif_file("/tmp/onetcaptcha.gif", ios::binary);
		gif_file.write(gif_buffer, offset_recv);				// POPRAWIĆ offset_recv NA (offset_recv - początek nagłówka GIF)
		gif_file.close();
	}

	return 0;
}


int http_3(string &cookies, string &captcha_code, string &err_code)
{
	int socket_status;
	size_t err_code_start, err_code_end;
	char c_buffer[2000];
	long offset_recv;
	stringstream content_length;
	string err_code_string = "err_code=\"";

	string api_function = "api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha_code + "\";}";
	content_length << api_function.size();

	string host = "czat.onet.pl";
	string port = "80";
	string data_send = 	"POST /include/ajaxapi.xml.php3 HTTP/1.1\r\n"
						"Host: " + host + "\r\n"
						"Connection: Keep-Alive\r\n"
						"Content-Type: application/x-www-form-urlencoded\r\n"
						"Content-Length: " + content_length.str() + "\r\n"
						"Cache-Control: no-cache\r\n"
						"Pragma: no-cache\r\n"
						"User-Agent: Mozilla/5.0\r\n"
						"Cookie:" + cookies + "\r\n\r\n"
						+ api_function;

	socket_status = socket_a(host, port, data_send, c_buffer, offset_recv);
	if(socket_status != 0)
		return socket_status;		// kod błędu, gdy napotkano problem z socketem

	// sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera)
	err_code_start = string(c_buffer).find(err_code_string);		// znajdź pozycję pierwszego cookie (bez pominięcia "Set-Cookie:")
	if(err_code_start == string::npos)
		return 8;		// kod błędu, gdy nie znaleziono szukanego wyrażenia

	err_code_end = string(c_buffer).find("\"", err_code_start + err_code_string.size());		// err_code_string.size(), bo trzeba pominąć pierwszy cudzysłów
	if(err_code_end == string::npos)
			return 9;	// kod błędu, gdy nie znaleziono oczekiwanego cudzysłowu na końcu wyrażenia

	err_code.insert(0, string(c_buffer), err_code_start + err_code_string.size(), err_code_end - err_code_start - err_code_string.size());	// wstaw odebraną wartość

	return 0;
}


int irc()
{
	return 0;
}
