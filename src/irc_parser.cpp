#include <string>       // std::string
#include <unistd.h>     // close()
#include "irc_parser.hpp"
#include "auth.hpp"
#include "ucc_colors.hpp"


void irc_parser(char *buffer_irc_recv, std::string &msg, short &msg_color, std::string &room, bool &send_irc, bool &irc_ok)
{
    std::string f_value;

    // zacznij od wyzerowania bufora powrotnego
    msg.clear();

    // domyślnie wiadomości nie są przeznaczone do wysłania do sieci IRC
    send_irc = false;

    // odpowiedz na PING
    if(find_value(buffer_irc_recv, "PING :", "\r\n", f_value) == 0)
    {
        send_irc = true;    // wiadomość do odesłania na IRC
        msg = "PONG :" + f_value;
        return;
    }

    // nieeleganckie na razie wycinanie z tekstu (z założeniem, że chodzi o 1 pokój), aby pokazać w komunikat usera
    else if(find_value(buffer_irc_recv, "PRIVMSG " + room + " :", "\r\n", f_value) == 0)
    {
        std::string nick_on_irc;
        find_value(buffer_irc_recv, ":", "!", nick_on_irc);
        msg_color = UCC_YELLOW;
        msg = nick_on_irc + ": " + f_value;
        return;
    }

    // wykryj, gdy serwer odpowie ERROR
    if(find_value(buffer_irc_recv, "ERROR :", "\r\n", f_value) == 0)
        irc_ok = false;
}
