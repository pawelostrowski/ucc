#include <iostream>     // std::cerr, std::endl
#include <string>       // std::string
#include <unistd.h>     // close()
#include "irc_parser.hpp"
#include "sockets.hpp"
#include "expression.hpp"


int irc_parser(char *buffer_recv, std::string &data_send, int &socketfd, bool &connect_status)
{
    int bytes_recv = 0;
    std::string f_value, user_msg;

    // gdy gniazdo "zgłosi" dane do pobrania, pobierz te dane
    if(asyn_socket_recv(buffer_recv, bytes_recv, socketfd) != 0)
    {
        close(socketfd);
        std::cerr << "Połączenie zerwane lub problem z siecią!" << std::endl;
        return 1;
    }

    // odpowiedz na PING
    if(find_value(buffer_recv, "PING :", "\r\n", f_value) == 0)
        asyn_socket_send("PONG :" + f_value, socketfd);

    // nieeleganckie na razie wycinanie z tekstu (z założeniem, że chodzi o #scc), aby pokazać w komunikat usera
    else if(find_value(buffer_recv, "PRIVMSG #scc :", "\r\n", user_msg) == 0)
    {
        find_value(buffer_recv, ":", "!", f_value);
        std::cout << "* " + f_value + ": " + user_msg << std::endl;
    }

    else
        std::cout << buffer_recv;

    // wykryj, gdy serwer odpowie ERROR, wtedy zakończ
    if(find_value(buffer_recv, "ERROR :", "\r\n", f_value) == 0)
        connect_status = false;     // zakończ, gdy odebrano błąd połączenia

    return 0;
}
