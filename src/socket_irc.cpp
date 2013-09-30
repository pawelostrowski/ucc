#include <cstring>          // strlen()
#include <string>           // std::string
#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
//#include <fcntl.h>          // fcntl()
#include <unistd.h>         // close() - socket
#include "socket_irc.hpp"


int socket_irc_init(int &socketfd_irc)
{
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

    return 0;
}


int socket_irc_send(std::string &data_send, int &socketfd_irc, bool irc_ok)
{
//	int bytes_sent;

    data_send += "\r\n";    // do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)

//    std::cout << "> " + data_send;

    /*bytes_sent =*/ send(socketfd_irc, data_send.c_str(), strlen(data_send.c_str()), 0);

    return 0;
}


int socket_irc_recv(char *buffer_recv, int &socketfd_irc, bool irc_ok)
{
    int bytes_recv = recv(socketfd_irc, buffer_recv, 1500 - 1, 0);

    if(bytes_recv == -1)
    {
        close(socketfd_irc);
        return 1;
    }

    if(bytes_recv == 0)
    {
        close(socketfd_irc);
        return 2;
    }

    buffer_recv[bytes_recv] = '\0';

    return 0;
}
