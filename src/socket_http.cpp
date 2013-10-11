#include <cstring>          // memset(), memcpy()
#include <sstream>          // std::string, std::stringstream
#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>         // close() - socket
#include "socket_http.hpp"


bool socket_http(std::string method, std::string host, std::string stock, std::string content, std::string &cookies, bool get_cookies,
                 char *buffer_recv, long &offset_recv, std::string &msg_err)
{
    if(method != "GET" && method != "POST")
    {
        msg_err = "Nieobsługiwana metoda " + method;
        return false;
    }

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

    // pobierz status adresu
    if(getaddrinfo(host.c_str(), "80", &host_info, &host_info_list) != 0)       // zapis host.c_str() oznacza zamianę std::string na C string
    {
        msg_err = "Nie udało się pobrać informacji o hoście " + host + " (sprawdź swoje połączenie internetowe)";
        return false;
    }

    // utwórz deskryptor gniazda (socket)
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if(socketfd == -1)
    {
        freeaddrinfo(host_info_list);
        msg_err = "Nie udało się utworzyć deskryptora gniazda";
        return false;
    }

    // pobierz status połączenia do hosta
    if(connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1)
    {
        freeaddrinfo(host_info_list);
        close(socketfd);        // zamknij połączenie z hostem
        msg_err = "Nie udało się połączyć z hostem " + host;
        return false;
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
        msg_err = "Nie udało się wysłać danych do hosta " + host;
        return false;
    }

    // sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
    if(bytes_sent != (int)data_send.size())     // (int) konwertuje zwracaną wartość na int
    {
        freeaddrinfo(host_info_list);
        close(socketfd);
        msg_err = "Nie udało się wysłać wszystkich danych do hosta " + host;
        return false;
    }

    // poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie)
    offset_recv = 0;        // offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
    do
    {
        // pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
        bytes_recv = recv(socketfd, buffer_tmp, 1500, 0);
        // sprawdź, czy pobieranie danych się powiodło
        if(bytes_recv == -1)
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            msg_err = "Nie udało się pobrać danych z hosta " + host;
            return false;
        }
        // sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
        if(first_recv)
        {
            if(bytes_recv == 0)
            {
                freeaddrinfo(host_info_list);
                close(socketfd);
                msg_err = "Host " + host + " zakończył połączenie";
                return false;
            }
        }
        first_recv = false;     // kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
        memcpy(buffer_recv + offset_recv, buffer_tmp, bytes_recv);      // pobrane dane "dopisz" do bufora
        offset_recv += bytes_recv;      // zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)
    } while(bytes_recv != 0);

    buffer_recv[offset_recv] = '\0';

    freeaddrinfo(host_info_list);
    close(socketfd);        // zamknij połączenie z hostem

    // jeśli trzeba, wyciągnij cookies z bufora
    if(get_cookies)
    {
        if(! find_cookies(buffer_recv, cookies, msg_err))
        {
            return false;       // zwróć komunikat błędu w msg_err
        }
    }

    return true;
}


bool find_cookies(char *buffer_recv, std::string &cookies, std::string &msg_err)
{
    size_t pos_cookie_start, pos_cookie_end;
    std::string cookie_string, cookie_tmp;

    cookie_string = "Set-Cookie:";

    // znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
    pos_cookie_start = std::string(buffer_recv).find(cookie_string);    // std::string(buffer_recv) zamienia C string na std::string
    if(pos_cookie_start == std::string::npos)
    {
        msg_err = "Nie znaleziono żadnego cookie";
        return false;
    }

    do
    {
        // szukaj ";" od pozycji początku cookie
        pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);
        if(pos_cookie_end == std::string::npos)
        {
            msg_err = "Problem z cookie, brak wymaganego średnika na końcu";
            return false;
        }

        // skopiuj cookie do bufora pomocniczego
        cookie_tmp.clear();     // wyczyść bufor pomocniczy
        cookie_tmp.insert(0, std::string(buffer_recv), pos_cookie_start + cookie_string.size(), pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

        // dopisz kolejny cookie do bufora
        cookies += cookie_tmp;

        // znajdź kolejny cookie
        pos_cookie_start = std::string(buffer_recv).find(cookie_string, pos_cookie_start + cookie_string.size());

    } while(pos_cookie_start != std::string::npos);     // zakończ szukanie, gdy nie znaleziono kolejnego cookie

    return true;
}
