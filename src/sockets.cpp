#include <cstring>      // memset(), strlen(), strstr()
#include <sstream>      // std::stringstream, std::string
#include <fstream>      // std::ofstream, perror()
//#include <sys/socket.h>
#include <netdb.h>      // getaddrinfo(), freeaddrinfo(), socket()
//#include <fcntl.h>      // asynchroniczne gniazdo (sokcet)
#include <unistd.h>     // close() - socket
#include "sockets.hpp"
#include "expression.hpp"
#include "auth.hpp"

#include <iostream>     // docelowo pozbyć się stąd tej biblioteki, komunikaty będą wywoływane w innym miejscu


int socket_a(std::string host, std::string port, std::string data_send, char *c_buffer, long &offset_recv, bool useirc)
{
    int socketfd;       // deskryptor gniazda (socket)
    int bytes_sent, bytes_recv, data_send_len;
    char tmp_buffer[1024];      // bufor tymczasowy pobranych danych
    bool first_recv = true;     // czy to pierwsze pobranie w pętli

    struct addrinfo host_info;          // The struct that getaddrinfo() fills up with data.
    struct addrinfo *host_info_list;    // Pointer to the to the linked list of host_info's.

    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;        // IP version not specified. Can be both.
    host_info.ai_socktype = SOCK_STREAM;    // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

    // zapis przykładowo host.c_str() oznacza, że string zostaje zamieniony na const char
    if(getaddrinfo(host.c_str(), port.c_str(), &host_info, &host_info_list) != 0)   // pobierz status adresu
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
        data_send_len = strlen(data_send.c_str());      // rozmiar danych, jakie chcieliśmy wysłać
        if(bytes_sent != data_send_len)         // sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 5;       // kod błędu przy różnicy w wysłanych bajtach względem tych, które chcieliśmy wysłać
        }
    }

    // poniższa pętla pobiera dane z hosta do bufora
    offset_recv = 0;        // offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
    do
    {
        bytes_recv = recv(socketfd, tmp_buffer, 1024 - 1, 0);       // pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
        if(bytes_recv == -1)        // sprawdź, czy pobieranie danych się powiodło
        {
            freeaddrinfo(host_info_list);
            close(socketfd);
            return 6;       // kod błędu przy niepowodzeniu w pobieraniu danych od hosta
        }
        if(first_recv)      // sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
        {
            if(bytes_recv == 0)
            {
                freeaddrinfo(host_info_list);
                close(socketfd);
                return 7;       // kod błędu przy pobraniu zerowej ilości bajtów (możliwy powód: host został wyłączony)
            }
        }
        first_recv = false;     // kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
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


void header_get(std::string host, std::string data_get, std::string cookies, std::string &data_send, bool add_cookies)
{
    data_send.clear();

    data_send = "GET " + data_get + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "Connection: close\r\n"
                "Cache-Control: no-cache\r\n"
                "Pragma: no-cache\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n";

    if(add_cookies)
        data_send += "Cookie:" + cookies + "\r\n";

    data_send += "\r\n";
}


void header_post(std::string host, std::string cookies, std::string api_function, std::string &data_send)
{
    std::stringstream content_length;

    content_length.clear();
    data_send.clear();

    content_length << api_function.size();      // wczytaj długość zapytania

    data_send = "POST /include/ajaxapi.xml.php3 HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "Connection: close\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Content-Length: " + content_length.str() + "\r\n"      // content_length.str()  <--- zamienia liczbę na std::string
                "Cache-Control: no-cache\r\n"
                "Pragma: no-cache\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n"
                "Cookie:" + cookies + "\r\n\r\n"
                + api_function;
}


int http_1(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char c_buffer[50000];
    std::string data_send;

    header_get("kropka.onet.pl", "/_s/kropka/1?DV=czat/applet/FULL", cookies, data_send);   // utwórz zapytanie do wysłania

    socket_status = socket_a("kropka.onet.pl", "80", data_send, c_buffer, offset_recv);     // wyślij dane
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    cookies.clear();        // wyczyść bufor cookies
    cookies_status = find_cookies(c_buffer, cookies);       // z bufora c_buffer pobierz cookies
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies

    return 0;
}


int http_2(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char c_buffer[50000];
    char *gif_buffer;
    std::string data_send;

    header_get("czat.onet.pl", "/myimg.gif", cookies, data_send, true);

    socket_status = socket_a("czat.onet.pl", "80", data_send, c_buffer, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    cookies_status = find_cookies(c_buffer, cookies);
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies

    gif_buffer = strstr(c_buffer, "GIF");       // daj wskaźnik na początek obrazka
    if(gif_buffer == NULL)
        return 8;       // kod błędu, gdy nie znaleziono obrazka w buforze

    // zapisz obrazek z captcha na dysku
    std::ofstream gif_file(GIF_FILE, std::ios::binary);
    if(gif_file == NULL)
    {
        perror("gif_file: " GIF_FILE);
        return 9;
    }
    gif_file.write(gif_buffer, &c_buffer[offset_recv] - gif_buffer);    // &c_buffer[offset_recv] - gif_buffer <--- adres końca bufora - adres początku obrazka = rozmiar obrazka
    gif_file.close();

    return 0;
}


int http_3(std::string &cookies, std::string captcha_code, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char c_buffer[50000];
    std::string api_function, data_send;

    api_function = "api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha_code + "\";}";

    header_post("czat.onet.pl", cookies, api_function, data_send);

    socket_status = socket_a("czat.onet.pl", "80",data_send, c_buffer, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    // sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE)
    f_value_status = find_value(c_buffer, "err_code=\"", "\"", err_code);   // szukaj wartości między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
    if(f_value_status != 0)
        return f_value_status;      // kod błedu, gdy nie udało się pobrać err_code

    if(err_code != "TRUE")
        if(err_code != "FALSE")
            return 99;          // kod błedu, gdy serwer nie zwrócił wartości TRUE lub FALSE

    return 0;
}


int http_4(std::string &cookies, std::string &nick, std::string &zuousername, std::string &uokey, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char c_buffer[50000];
    std::string api_function, data_send;
    std::stringstream nick_length;

    nick_length << nick.size();

    api_function =  "api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + nick_length.str() + ":\""
                    + nick + "\";s:8:\"tempNick\";i:1;s:7:\"version\";s:22:\"1.1(20130621-0052 - R)\";}";

    header_post("czat.onet.pl", cookies, api_function, data_send);

    socket_status = socket_a("czat.onet.pl", "80", data_send, c_buffer, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    // pobierz kod błędu
    f_value_status = find_value(c_buffer, "err_code=\"", "\"", err_code);
    if(f_value_status != 0)
        return f_value_status;      // kod błędu, gdy nie udało się pobrać err_code

    // sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
    if(err_code != "TRUE")
        return 0;               // 0, bo to nie jest błąd programu

    // pobierz uoKey
    f_value_status = find_value(c_buffer, "<uoKey>", "</uoKey>", uokey);
    if(f_value_status != 0)
        return f_value_status + 10; // kod błedu, gdy nie udało się pobrać uoKey (+10, aby można było go odróżnić od poprzedniego użycia find_value() )

    // pobierz zuoUsername (nick, który zwrócił serwer)
    f_value_status = find_value(c_buffer, "<zuoUsername>", "</zuoUsername>", zuousername);
    if(f_value_status != 0)
        return f_value_status + 20; // kod błędu, gdy serwer nie zwrócił nicka (+20, aby można było go odróżnić od poprzedniego użycia find_value() )

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

    c_buffer[bytes_recv] = '\0';

    std::cout << c_buffer;

    return 0;
}


int irc(std::string &zuousername, std::string &uokey)
{
/*
    int socket_status;
    char c_buffer[5000];
    long offset_recv;

    std::string host = "czat-app.onet.pl";
    std::string port = "5015";
    std::string data_send;

    data_send.clear();      // niczego nie wysyłaj na tym etapie do serwera

    socket_status = socket_a(host, port, data_send, c_buffer, offset_recv, true);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem
*/

    size_t first_line_char;
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

//    fcntl(socketfd, F_SETFL, O_SYNC);     // asynchroniczne gniazdo (socket)

    www.sin_family = AF_INET;
    www.sin_port = htons(5015);
    www.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(www.sin_zero), '\0', 8);

    if(connect(socketfd, (struct sockaddr *)&www, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        close(socketfd);
        return 103;
    }

    // połącz z serwerem
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


    // pobierz odpowiedź z serwera
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);


    // wyślij: JOIN #<kanal>
    data_send.clear();
    data_send = "JOIN #scc\r\n";
    std::cout << "> " + data_send;
    asyn_socket_send(data_send, socketfd);


    // pobierz odpowiedź z serwera
    asyn_socket_recv(c_buffer, bytes_recv, socketfd);

/*
    do
    {
        data_send.clear();
        getline(std::cin, data_send);
        data_send += "\r\n";
        asyn_socket_send(data_send, socketfd);
        asyn_socket_recv(c_buffer, bytes_recv, socketfd);
    } while(true);
*/

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

    close(socketfd);

    return 0;
}
