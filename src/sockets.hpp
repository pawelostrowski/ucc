#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#define SOCKETS_HPP_NAME "sockets"


//int http(std::string &host, std::string &port, std::string &data_send, std::string &cookies_recv);

/*
Znaczenie zmiennych w http():
	host - adres hosta
	port - port
	data_send - zapytanie do hosta (np. GET /<...>)
	cookies_recv - pobrane cookies
*/









int http_1(std::string &cookies);

int http_2(std::string &cookies);






#endif // SOCKETS_HPP
