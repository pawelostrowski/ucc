#include <iostream>     // std::cin
#include "kbd_parser.hpp"
#include "sockets.hpp"


int kbd_parser(std::string kbd_buf, std::string data_send, int socketfd)
{
    char command_slash;

    std::cin.clear();		// zapobiega zapętleniu się programu przy kombinacji Ctrl-D
    getline(std::cin, kbd_buf);     // pobierz zawartość bufora klawiatury

    if(kbd_buf.size() != 0)
    {
        command_slash = kbd_buf[0];
        if(command_slash != '/')
            asyn_socket_send("PRIVMSG #scc :" + kbd_buf, socketfd);

        else
        {
            kbd_buf.erase(0, 1);    // usuń / z pierwszego pola
            asyn_socket_send(kbd_buf, socketfd);
        }
    }

    return 0;
}
