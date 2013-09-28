#include <cstring>          // memset(), strlen(), memcpy()
#include <string>           // std::string
#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>         // close() - socket
#include "socket_http.hpp"


int socket_http(std::string host, std::string data_send, char *buffer_recv, long &offset_recv)
{
    int socketfd;       // deskryptor gniazda (socket)
    int bytes_sent, bytes_recv, data_send_length;
    char buffer_tmp[1500];      // bufor tymczasowy pobranych danych
    bool first_recv = true;     // czy to pierwsze pobranie w pętli

    struct addrinfo host_info;          // ta struktura wypełni się danymi w getaddrinfo()
    struct addrinfo *host_info_list;    // wskaźnik do połączonej listy host_info

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family = AF_UNSPEC;        // wersja IP niesprecyzowana (można ją określić)
    host_info.ai_socktype = SOCK_STREAM;    // SOCK_STREAM - TCP, SOCK_DGRAM - UDP

    // zapis przykładowo host.c_str() oznacza, że string zostaje zamieniony na const char
    if(getaddrinfo(host.c_str(), "80", &host_info, &host_info_list) != 0)   // pobierz status adresu
        return 31;          // kod błedu przy niepowodzeniu w pobraniu statusu adresu

    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);     // utwórz deskryptor gniazda (socket)

    if(socketfd == -1)
    {
        freeaddrinfo(host_info_list);
        return 32;          // kod błedu przy niepowodzeniu w tworzeniu deskryptora gniazda (socket)
    }

    if(connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1)        // pobierz status połączenia do hosta
    {
        freeaddrinfo(host_info_list);
        close(socketfd);        // zamknij połączenie z hostem
        return 33;          // kod błędu przy niepowodzeniu połączenia do hosta
    }

    // wyślij zapytanie do hosta
    if(data_send.size() != 0)       // jeśli bufor pusty, nie próbuj nic wysyłać
    {
        bytes_sent = send(socketfd, data_send.c_str(), strlen(data_send.c_str()), 0);       // wysłanie zapytania (np. GET /<...>)
        if(bytes_sent == -1)
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 34;      // kod błędu przy niepowodzeniu w wysłaniu danych do hosta
		}
        data_send_length = strlen(data_send.c_str());      // rozmiar danych, jakie chcieliśmy wysłać
        if(bytes_sent != data_send_length)         // sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 35;      // kod błędu przy różnicy w wysłanych bajtach względem tych, które chcieliśmy wysłać
        }
    }

    // poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie)
    offset_recv = 0;        // offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
    do
    {
        bytes_recv = recv(socketfd, buffer_tmp, 1500, 0);   // pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
        if(bytes_recv == -1)        // sprawdź, czy pobieranie danych się powiodło
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 36;      // kod błędu przy niepowodzeniu w pobieraniu danych od hosta
        }
        if(first_recv)      // sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
        {
            if(bytes_recv == 0)
            {
                freeaddrinfo(host_info_list);
                close(socketfd);
                return 37;  // kod błędu przy pobraniu zerowej ilości bajtów (możliwy powód: host zakończył połączenie)
            }
        }
        first_recv = false;     // kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
        memcpy(buffer_recv + offset_recv, buffer_tmp, bytes_recv);      // pobrane dane "dopisz" do bufora
        offset_recv += bytes_recv;      // zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)
    } while(bytes_recv != 0);

    buffer_recv[offset_recv] = '\0';

    freeaddrinfo(host_info_list);
    close(socketfd);        // zamknij połączenie z hostem

    return 0;
}
