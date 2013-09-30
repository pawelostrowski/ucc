#include <cstring>      // memset(), strlen(), strstr()
#include <sstream>      // std::stringstream, std::string
#include "sockets.hpp"
#include "auth.hpp"
//#include "auth_code.hpp"
//#include "expression.hpp"
#include "irc_parser.hpp"
#include "kbd_parser.hpp"

#define STDIN 0         // deskryptor pliku dla standardowego wejścia


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


int socket_irc(std::string &zuousername, std::string &uokey, int &socketfd_irc, WINDOW *active_room)
{
//    size_t first_line_char;
//    bool connect_status = true;
//    int socketfd;       // deskryptor gniazda (socket)
//    int bytes_recv = 0;
    int f_value_status;
    char buffer_recv[1500];
    std::string data_send, authkey, f_value, kbd_buf;

    struct hostent *he;
    struct sockaddr_in www;

    he = gethostbyname("czat-app.onet.pl");
    if(he == NULL)
        return 101;

    socketfd_irc = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd_irc == -1)
        return 102;

//    fcntl(socketfd_irc, F_SETFL, O_ASYNC);      // asynchroniczne gniazdo (socket)

    www.sin_family = AF_INET;
    www.sin_port = htons(5015);
    www.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(www.sin_zero), '\0', 8);

    if(connect(socketfd_irc, (struct sockaddr *)&www, sizeof(struct sockaddr)) == -1)
    {
        close(socketfd_irc);
        return 103;
    }

/*
    Od tego momentu zostaje nawiązane połączenie IRC
*/

    // pobierz pierwszą odpowiedż serwera po połączeniu
//    asyn_socket_recv(buffer_recv, bytes_recv, socketfd_irc);
//    std::cout << buffer_recv;
        show_buffer_recv(buffer_recv, active_room);

    // wyślij: NICK <~nick>
//    asyn_socket_send("NICK " + zuousername, socketfd_irc, active_room);
        show_buffer_send("NICK " + zuousername + "\n", active_room);

    // pobierz odpowiedź z serwera
//    asyn_socket_recv(buffer_recv, bytes_recv, socketfd_irc);
//    std::cout << buffer_recv;
        show_buffer_recv(buffer_recv, active_room);

    // wyślij: AUTHKEY
//    asyn_socket_send("AUTHKEY", socketfd_irc, active_room);
        show_buffer_send("AUTHKEY\n", active_room);

    // pobierz odpowiedź z serwera (AUTHKEY)
//    asyn_socket_recv(buffer_recv, bytes_recv, socketfd_irc);
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
    if(! auth_code(authkey))
        return 104;

    // char na string
//    authkey_tmp << authkey;
//    authkey_s.clear();
//   authkey_s = authkey_tmp.str();


    // wyślij: AUTHKEY <AUTHKEY>
//    asyn_socket_send("AUTHKEY " + authkey, socketfd_irc, active_room);
        show_buffer_send("AUTHKEY " + authkey + "\n", active_room);




    // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>
//    asyn_socket_send("USER * " + uokey + " czat-app.onet.pl :" + zuousername, socketfd_irc, active_room);
        show_buffer_send("USER * " + uokey + " czat-app.onet.pl :" + zuousername + "\n", active_room);

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
