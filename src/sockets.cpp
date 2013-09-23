#include <cstring>      // memset(), strlen(), strstr()
#include <sstream>      // std::stringstream, std::string
#include <sys/select.h> // select()
//#include <sys/socket.h>
#include <netdb.h>      // getaddrinfo(), freeaddrinfo(), socket()
#include <fcntl.h>
#include <unistd.h>     // close() - socket
#include "sockets.hpp"
#include "auth_http.hpp"
#include "auth_code.hpp"
#include "expression.hpp"

#define STDIN 0

#include <iostream>     // docelowo pozbyć się stąd tej biblioteki, komunikaty będą wywoływane w innym miejscu


int socket_http(std::string host, std::string data_send, char *c_buffer, long &offset_recv, bool useirc)
{
    int socketfd;       // deskryptor gniazda (socket)
    int bytes_sent, bytes_recv, data_send_length;
    char tmp_buffer[1500];      // bufor tymczasowy pobranych danych
//    bool first_recv = true;     // czy to pierwsze pobranie w pętli

    struct addrinfo host_info;          // The struct that getaddrinfo() fills up with data.
    struct addrinfo *host_info_list;    // Pointer to the to the linked list of host_info's.

    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;        // IP version not specified. Can be both.
    host_info.ai_socktype = SOCK_STREAM;    // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

    // zapis przykładowo host.c_str() oznacza, że string zostaje zamieniony na const char
    if(getaddrinfo(host.c_str(), "80", &host_info, &host_info_list) != 0)   // pobierz status adresu
        return 1;		// kod błedu przy niepowodzeniu w pobraniu statusu adresu

    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);     // utwórz deskryptor gniazda (socket)

    if(socketfd == -1)
    {
        freeaddrinfo(host_info_list);
        return 2;       // kod błedu przy niepowodzeniu w tworzeniu deskryptora gniazda (socket)
    }

    if(connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1)        // pobierz status połączenia do hosta
    {
        freeaddrinfo(host_info_list);
        close(socketfd);        // zamknij połączenie z hostem
        return 3;       // kod błędu przy niepowodzeniu połączenia do hosta
    }

    // wyślij zapytanie do hosta
    if(data_send.size() != 0)       // jeśli bufor pusty, nie próbuj nic wysyłać
    {
        bytes_sent = send(socketfd, data_send.c_str(), strlen(data_send.c_str()), 0);       // wysłanie zapytania (np. GET /<...>)
        if(bytes_sent == -1)
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 4;       // kod błędu przy niepowodzeniu w wysłaniu danych do hosta
		}
        data_send_length = strlen(data_send.c_str());      // rozmiar danych, jakie chcieliśmy wysłać
        if(bytes_sent != data_send_length)         // sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 5;       // kod błędu przy różnicy w wysłanych bajtach względem tych, które chcieliśmy wysłać
        }
    }

    // poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie)
    offset_recv = 0;        // offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
    do
    {
        bytes_recv = recv(socketfd, tmp_buffer, 1500, 0);       // pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
        if(bytes_recv == -1)        // sprawdź, czy pobieranie danych się powiodło
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 6;       // kod błędu przy niepowodzeniu w pobieraniu danych od hosta
        }
/*
        if(first_recv)      // sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
        {
            if(bytes_recv == 0)
            {
                freeaddrinfo(host_info_list);
                close(socketfd);
                return 7;       // kod błędu przy pobraniu zerowej ilości bajtów (możliwy powód: host zakończył połączenie)
            }
        }
        first_recv = false;     // kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
*/
        memcpy(c_buffer + offset_recv, tmp_buffer, bytes_recv);     // pobrane dane "dopisz" do bufora
        offset_recv += bytes_recv;      // zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)
        if(useirc)
            break;
    } while(bytes_recv != 0);

    c_buffer[offset_recv] = '\0';

    freeaddrinfo(host_info_list);
    close(socketfd);        // zamknij połączenie z hostem

    return 0;
}


int asyn_socket_send(std::string &data_send, int &socketfd)
{
//	int bytes_sent;

    /*bytes_sent =*/ send(socketfd, data_send.c_str(), strlen(data_send.c_str()), 0);

    return 0;
}

int asyn_socket_recv(char *c_buffer, int bytes_recv, int &socketfd)
{
/*
    int offset_recv = 0;
    char tmp_buffer[2000];

    do
    {
        bytes_recv = recv(socketfd, tmp_buffer, 2000 - 1, 0);
        if(bytes_recv == -1)
        {
            close(socketfd);
            return bytes_recv;
        }
        memcpy(c_buffer + offset_recv, tmp_buffer, bytes_recv);     // pobrane dane "dopisz" do bufora
        offset_recv += bytes_recv;      // zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)
    } while(bytes_recv == 0);
*/

    bytes_recv = recv(socketfd, c_buffer, 1500, 0);

    c_buffer[bytes_recv] = '\0';

    if(bytes_recv == -1)
    {
        close(socketfd);
        return bytes_recv;
    }

        std::cout << "Początek ramki -->" <<std::endl;
    std::cout << c_buffer;
        std::cout << "<-- Koniec ramki" << std::endl;

    return 0;
}


int socket_irc(std::string &zuousername, std::string &uokey)
{
//    size_t first_line_char;
    bool connect_status = true;
    int socketfd;       // deskryptor gniazda (socket)
    int bytes_recv = 0, f_value_status;
    char c_buffer[50000];
    char authkey[16 + 1];       // AUTHKEY ma co prawda 16 znaków, ale w 17. będzie wpisany kod zera, aby odróżnić koniec tablicy
    std::string data_send, authkey_s, f_value;
    std::stringstream authkey_tmp;

    struct hostent *he;
    struct sockaddr_in www;

    he = gethostbyname("czat-app.onet.pl");
    if(he == NULL)
        return 101;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1)
        return 102;

    fcntl(socketfd, F_SETFL, O_ASYNC);

    www.sin_family = AF_INET;
    www.sin_port = htons(5015);
    www.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(www.sin_zero), '\0', 8);

    if(connect(socketfd, (struct sockaddr *)&www, sizeof(struct sockaddr)) == -1)
    {
        close(socketfd);
        return 103;
    }

/*
    Od tego momentu zostaje nawiązane połączenie IRC
*/

    // pobierz pierwszą odpowiedż serwera po połączeniu
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);

    // wyślij: NICK <~nick>
    data_send.clear();
    data_send = "NICK " + zuousername + "\r\n";
    std::cout << "> " + data_send;
    asyn_socket_send(data_send, socketfd);

    // pobierz odpowiedź z serwera
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);

    // wyślij: AUTHKEY
    data_send.clear();
    data_send = "AUTHKEY\r\n";
    std::cout << "> " + data_send;
    asyn_socket_send(data_send, socketfd);

    // pobierz odpowiedź z serwera (AUTHKEY)
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);

    // wyszukaj AUTHKEY
    f_value_status = find_value(c_buffer, "801 " + zuousername + " :", "\r\n", authkey_s);
    if(f_value_status != 0)
        return f_value_status + 110;

    // AUTHKEY string na char
    memcpy(authkey, authkey_s.data(), 16);
    authkey[16] = '\0';

    // konwersja AUTHKEY
    if(auth(authkey) != 0)
        return 104;

    // char na string
    authkey_tmp << authkey;
    authkey_s.clear();
    authkey_s = authkey_tmp.str();

    // wyślij: AUTHKEY <AUTHKEY>
    data_send.clear();
    data_send = "AUTHKEY " + authkey_s + "\r\n";
    std::cout << "> " + data_send;
    asyn_socket_send(data_send, socketfd);

    // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>
    data_send.clear();
    data_send = "USER * " + uokey + " czat-app.onet.pl :" + zuousername + "\r\n";
    std::cout << "> " + data_send;
    asyn_socket_send(data_send, socketfd);

/*
    Od tej pory obsługa gniazda będzie zależała od jego stanu, który wykrywać będzie select()
*/

    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(socketfd, &readfds);

    do
    {
        select(socketfd + 1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(socketfd, &readfds))
        {
            asyn_socket_recv(c_buffer, bytes_recv, socketfd);   // gdy gniazdo "zgłosi" dane do pobrania, pobierz te dane
            // odpowiedz na PING
            f_value_status = find_value(c_buffer, "PING :", "\r\n", f_value);
            if(f_value_status == 0)     // jeśli PING, wyślij PONG
            {
                data_send.clear();
                data_send = "PONG :" + f_value + "\r\n";
                std::cout << "> " + data_send;
                asyn_socket_send(data_send, socketfd);
            }
            // wykryj, gdy serwer odpowie ERROR, wtedy zakończ
            f_value_status = find_value(c_buffer, "ERROR :", "\r\n", f_value);
            if(f_value_status == 0)
                connect_status = false;     // zakończ, gdy odebrano błąd połączenia
        }
    } while(connect_status);
/*

    // pobierz odpowiedź z serwera
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);


    // wyślij: JOIN #<kanal>
    data_send.clear();
    data_send = "JOIN #20_21_22_23_lat\r\n";
    std::cout << "> " + data_send;
    asyn_socket_send(data_send, socketfd);


    // pobierz odpowiedź z serwera
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);


    // czekaj na ping i odpowiedz pong oraz "wiś" na kanale
    do
    {
        if(asyn_socket_recv(c_buffer, bytes_recv, socketfd) != 0)   // czekaj na rekord do pobrania
        {
            std::cerr << "Host zakończył połączenie!" << std::endl;
            close(socketfd);
            return 98;
        }
        first_line_char = std::string(c_buffer).find(":");  // na której pozycji jest pierwszy :
        if(first_line_char != 0)    // gdy pierwszym znakiem nie jest :
        {
            f_value_status = find_value(c_buffer, "PING :", "\r\n", f_value);
            if(f_value_status == 0)     // jeśli PING, wyślij PONG
            {
                data_send.clear();
                data_send = "PONG :" + f_value + "\r\n";
                std::cout << "> " + data_send;
                asyn_socket_send(data_send, socketfd);
            }
            f_value_status = find_value(c_buffer, "ERROR :", "\r\n", f_value);
            if(f_value_status == 0)
                connect_status = false;     // zakończ, gdy odebrano błąd połączenia
        }
    } while(connect_status);    // zanim nie napotamy błedu, krąż w pętli

*/

    close(socketfd);

    return 0;
}
