#include <string>           // std::string
#include <cstring>          // strdup()

#include "irc_parser.hpp"
#include "auth.hpp"
#include "ucc_colors.hpp"


void irc_parser(std::string &buffer_irc_recv, std::string &msg, std::string &channel, bool &send_irc, bool &irc_ok)
{
    std::string f_value;

//    std::string buffer_irc_raw;
//    static size_t pos_raw_end = 0;

    // zacznij od wyczyszczenia bufora powrotnego
    msg.clear();

/*
    // znajdź koniec wiersza
    pos_raw_end = buffer_irc_recv.find("\n", pos_raw_end);

    // nie może dojść do sytuacji, że na końcu wiersza nie ma \n
    if(pos_raw_end == std::string::npos)
    {
        pos_raw_end = 0;
        msg = "# Błąd w buforze IRC!";
        return;
    }

    // wstaw aktualnie obsługiwany wiersz (raw)
    buffer_irc_raw.insert(0, buffer_irc_recv, )
*/

    // domyślnie wiadomości nie są przeznaczone do wysłania do sieci IRC
    send_irc = false;

    // odpowiedz na PING
    if(find_value(strdup(buffer_irc_recv.c_str()), "PING :", "\n", f_value) == 0)
    {
        send_irc = true;    // wiadomość do odesłania na IRC
        msg = "PONG :" + f_value;
        return;
    }

    // nieeleganckie na razie wycinanie z tekstu (z założeniem, że chodzi o 1 pokój), aby pokazać komunikat usera
    else if(find_value(strdup(buffer_irc_recv.c_str()), "PRIVMSG " + channel + " :", "\n", f_value) == 0)
    {
        std::string nick_on_irc;
        find_value(strdup(buffer_irc_recv.c_str()), ":", "!", nick_on_irc);
        msg = "<" + nick_on_irc + "> " + f_value;
        return;
    }

    // wykryj, gdy serwer odpowie ERROR
    if(find_value(strdup(buffer_irc_recv.c_str()), "ERROR :", "\n", f_value) == 0)
        irc_ok = false;
}
