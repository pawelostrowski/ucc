#include <cstring>          // strlen()
#include <string>           // std::string
#include "socket_irc.hpp"
#include "ucc_colors.hpp"


int socket_irc_init(int &socketfd_irc, struct sockaddr_in &www)
{
    struct hostent *he;

    he = gethostbyname("czat-app.onet.pl");
    if(he == NULL)
        return 41;

    socketfd_irc = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd_irc == -1)
        return 42;

    www.sin_family = AF_INET;
    www.sin_port = htons(5015);
    www.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(www.sin_zero), '\0', 8);

    return 0;
}


void socket_irc_connect(int &socketfd_irc, struct sockaddr_in &www, std::string &msg, short &msg_color, char *buffer_irc_recv)
{
    if(connect(socketfd_irc, (struct sockaddr *)&www, sizeof(struct sockaddr)) == -1)
    {
        close(socketfd_irc);
        msg_color = UCC_RED;
        msg = "* Nie udało się połączyć z siecią IRC";
        return;
    }

/*
    Od tego momentu zostaje nawiązane połączenie IRC
*/

}


int socket_irc_send(int &socketfd_irc, bool &irc_ok, std::string data_send, std::string &data_sent)
{
//	int bytes_sent;

    // do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)
    data_send += "\r\n";

//    std::cout << "> " + data_send;

    /*bytes_sent =*/ send(socketfd_irc, data_send.c_str(), data_send.size(), 0);

    data_sent = data_send;

    return 0;
}


int socket_irc_recv(int &socketfd_irc, bool &irc_ok, char *buffer_recv)
{
    int bytes_recv = recv(socketfd_irc, buffer_recv, 1500 - 1, 0);

    if(bytes_recv == -1)
    {
        close(socketfd_irc);
        irc_ok = false;
        return 1;
    }

    if(bytes_recv == 0)
    {
        close(socketfd_irc);
        irc_ok = false;
        return 2;
    }

    buffer_recv[bytes_recv] = '\0';

    return 0;
}
