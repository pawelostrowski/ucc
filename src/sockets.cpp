#include <cstring>      // memset(), strlen(), strstr()
#include <sstream>      // std::stringstream, std::string
#include "sockets.hpp"
#include "auth_http.hpp"
#include "auth_code.hpp"
#include "expression.hpp"
#include "irc_parser.hpp"
#include "kbd_parser.hpp"

#define STDIN 0         // deskryptor pliku dla standardowego wejścia

//#include <iostream>     // docelowo pozbyć się stąd tej biblioteki, komunikaty będą wywoływane w innym miejscu


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
        bytes_recv = recv(socketfd, buffer_tmp, 1500, 0);   // pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
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
                return 7;       // kod błędu przy pobraniu zerowej ilości bajtów (możliwy powód: host zakończył połączenie)
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


void show_buffer_send(std::string data_send, WINDOW *active_room)
{
    int data_send_len = data_send.size();

    wattrset(active_room, COLOR_PAIR(3));

    for(int i = 0; i < data_send_len; ++i)
    {
        if(data_send[i] != '\r')
            wprintw(active_room, "%c", data_send[i]);
    }
}


void show_buffer_recv(char *buffer_recv, WINDOW *active_room)
{
    int buffer_recv_len = strlen(buffer_recv);

    wattrset(active_room, A_NORMAL);

    for(int i = 0; i < buffer_recv_len; ++i)
    {
        if(buffer_recv[i] != '\r')
            wprintw(active_room, "%c", buffer_recv[i]);
    }
}


int asyn_socket_send(std::string data_send, int socketfd, WINDOW *active_room)
{
//	int bytes_sent;

    data_send += "\r\n";    // do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)

//    std::cout << "> " + data_send;
            // tymczasowo!!!
            if(data_send.find("PONG :") != std::string::npos)
                show_buffer_send(data_send, active_room);

    /*bytes_sent =*/ send(socketfd, data_send.c_str(), strlen(data_send.c_str()), 0);

    return 0;
}

int asyn_socket_recv(char *buffer_recv, int bytes_recv, int socketfd)
{
    bytes_recv = recv(socketfd, buffer_recv, 1500 - 1, 0);
    if(bytes_recv == -1)
    {
        close(socketfd);
        return bytes_recv;
    }
    if(bytes_recv == 0)
    {
        close(socketfd);
        return 1;
    }

    buffer_recv[bytes_recv] = '\0';

    return 0;
}


int socket_irc(std::string &zuousername, std::string &uokey, int &socketfd_irc, WINDOW *active_room)
{
//    size_t first_line_char;
//    bool connect_status = true;
    int socketfd;       // deskryptor gniazda (socket)
    int bytes_recv = 0, f_value_status;
    char buffer_recv[1500];
    std::string data_send, authkey, f_value, kbd_buf;

    struct hostent *he;
    struct sockaddr_in www;

    he = gethostbyname("czat-app.onet.pl");
    if(he == NULL)
        return 101;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1)
        return 102;

    fcntl(socketfd, F_SETFL, O_ASYNC);      // asynchroniczne gniazdo (socket)

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
    asyn_socket_recv(buffer_recv, bytes_recv, socketfd);
//    std::cout << buffer_recv;
        show_buffer_recv(buffer_recv, active_room);

    // wyślij: NICK <~nick>
    asyn_socket_send("NICK " + zuousername, socketfd, active_room);

    // pobierz odpowiedź z serwera
    asyn_socket_recv(buffer_recv, bytes_recv, socketfd);
//    std::cout << buffer_recv;
        show_buffer_recv(buffer_recv, active_room);

    // wyślij: AUTHKEY
    asyn_socket_send("AUTHKEY", socketfd, active_room);

    // pobierz odpowiedź z serwera (AUTHKEY)
    asyn_socket_recv(buffer_recv, bytes_recv, socketfd);
//    std::cout << buffer_recv;
        show_buffer_recv(buffer_recv, active_room);




    // wyszukaj AUTHKEY
//    char authkey[16 + 1];       // AUTHKEY ma co prawda 16 znaków, ale w 17. będzie wpisany kod zera, aby odróżnić koniec tablicy
//    std::string authkey_s;
//    std::stringstream authkey_tmp;

    f_value_status = find_value(buffer_recv, "801 " + zuousername + " :", "\r\n", authkey);
    if(f_value_status != 0)
        return f_value_status + 110;

    // AUTHKEY string na char
//    memcpy(authkey, authkey_s.data(), 16);
//    authkey[16] = '\0';

    // konwersja AUTHKEY
    if(auth(authkey) != 0)
        return 104;

    // char na string
//    authkey_tmp << authkey;
//    authkey_s.clear();
//   authkey_s = authkey_tmp.str();


    // wyślij: AUTHKEY <AUTHKEY>
    asyn_socket_send("AUTHKEY " + authkey, socketfd, active_room);




    // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>
    asyn_socket_send("USER * " + uokey + " czat-app.onet.pl :" + zuousername, socketfd, active_room);

/*
    Od tej pory obsługa gniazda będzie zależała od jego stanu, który wykrywać będzie select()
*/


        return 0;

/*
    fd_set readfds;
    fd_set readfds_tmp;

    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);    // klawiatura
    FD_SET(socketfd, &readfds); // gniazdo (socket)

    do
    {
        readfds_tmp = readfds;
        select(socketfd + 1, &readfds_tmp, NULL, NULL, NULL);       // czekaj, aż wejście (klawiatura) lub gniazdo (socket) zgłosi gotowość do odebrania danych
        // sprawdź, czy to gniazdo (socket) jest gotowe do odebrania danych
        if(FD_ISSET(socketfd, &readfds_tmp))
            if(irc_parser(buffer_recv, data_send, socketfd, connect_status))
            {
                close(socketfd);
                std::cerr << "Błąd w module irc_parser!" << std::endl;
                return 1;
            }
        // sprawdź, czy klawiatura coś wysłała (zgłoszenie następuje dopiero po wciśnięciu Enter)
        if(FD_ISSET(STDIN, &readfds_tmp))
// //            if(kbd_parser(kbd_buf, data_send, socketfd) != 0)       // wykonaj obsługę bufora klawiatury
            {
                close(socketfd);
                std::cerr << "Błąd w module kbd_parser!" << std::endl;
                return 1;
            }

    } while(connect_status);

    close(socketfd);

    return 0;
*/
}
