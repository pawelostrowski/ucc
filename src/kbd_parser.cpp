#include <iostream>     // std::cin
#include "kbd_parser.hpp"
#include "sockets.hpp"


int kbd_parser(std::string kbd_buf, std::string data_send, int &socketfd)
{
    getline(std::cin, kbd_buf);     // pobierz zawartość bufora klawiatury
    if(kbd_buf.size() != 0)
    {
        data_send.clear();
        data_send = kbd_buf + "\r\n";
        std::cout << "> " + data_send;
        asyn_socket_send(data_send, socketfd);
    }

    return 0;
}
