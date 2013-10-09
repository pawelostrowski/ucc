#include <cstring>          // memset(), memcpy()
#include <sstream>          // std::string, std::stringstream
#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>         // close() - socket
#include "socket_http.hpp"


int socket_http(std::string method, std::string host, std::string stock, std::string content, std::string &cookies, char *buffer_recv, long &offset_recv)
{
    int socketfd;               // deskryptor gniazda (socket)
    int bytes_sent, bytes_recv;
    char buffer_tmp[1500];      // bufor tymczasowy pobranych danych
    bool first_recv = true;     // czy to pierwsze pobranie w pętli
    std::string data_send;      // dane do wysłania do hosta
    std::stringstream content_length;

    struct addrinfo host_info;          // ta struktura wypełni się danymi w getaddrinfo()
    struct addrinfo *host_info_list;    // wskaźnik do połączonej listy host_info

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family = AF_INET;          // wersja IP IPv4
    host_info.ai_socktype = SOCK_STREAM;    // SOCK_STREAM - TCP, SOCK_DGRAM - UDP

    // zapis host.c_str() oznacza zamianę std::string na C string
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

    // utwórz dane do wysłania do hosta
    data_send =  method + " " + stock + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"
                "Cache-Control: no-cache\r\n"
                "Connection: close\r\n";

    if(method == "POST")
    {
        content_length << content.size();       // wczytaj długość zapytania
        data_send += "Content-Length: " + content_length.str() + "\r\n";    // content_length.str()  <--- zamienia liczbę na std::string
    }

    if(cookies.size() != 0)
        data_send += "Cookie:" + cookies + "\r\n";

    data_send += "\r\n";

    if(content.size() != 0)
        data_send += content;

    // wyślij dane do hosta
    bytes_sent = send(socketfd, data_send.c_str(), data_send.size(), 0);
    if(bytes_sent == -1)
    {
        freeaddrinfo(host_info_list);
        close(socketfd);
        return 34;      // kod błędu przy niepowodzeniu w wysłaniu danych do hosta
    }

    // sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
    if(bytes_sent != (int)data_send.size())     // (int) konwertuje zwracaną wartość na int
    {
        freeaddrinfo(host_info_list);
        close(socketfd);
        return 35;      // kod błędu przy różnicy w wysłanych bajtach względem tych, które chcieliśmy wysłać
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


int find_cookies(char *buffer_recv, std::string &cookies)
{
    size_t pos_cookie_start, pos_cookie_end;
    std::string cookie_string, cookie_tmp;

    cookie_string = "Set-Cookie:";

    // std::string(buffer_recv) zamienia C string na std::string
    pos_cookie_start = std::string(buffer_recv).find(cookie_string);    // znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
    if(pos_cookie_start == std::string::npos)
        return 1;           // kod błędu, gdy nie znaleziono cookie (pierwszego)

    do
    {
        pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);      // szukaj ";" od pozycji początku cookie
        if(pos_cookie_end == std::string::npos)
            return 2;       // kod błędu, gdy nie znaleziono oczekiwanego ";" na końcu każdego cookie

        // skopiuj cookie do bufora pomocniczego
        cookie_tmp.clear();     // wyczyść bufor pomocniczy
        cookie_tmp.insert(0, std::string(buffer_recv), pos_cookie_start + cookie_string.size(), pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

        cookies += cookie_tmp;      // dopisz kolejny cookie do bufora

        pos_cookie_start = std::string(buffer_recv).find(cookie_string, pos_cookie_start + cookie_string.size());   // znajdź kolejny cookie

    } while(pos_cookie_start != std::string::npos);     // zakończ szukanie, gdy nie znaleziono kolejnego cookie

    return 0;
}
