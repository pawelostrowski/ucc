#include <string>           // std::string
#include <cstring>          // strdup()

#include "irc_parser.hpp"
#include "auth.hpp"
#include "ucc_colors.hpp"


void irc_parser(std::string &buffer_irc_recv, std::string &buffer_irc_recv_tmp, std::string &msg, short &msg_color, std::string &room, bool &send_irc, bool &irc_ok)
{
    std::string f_value;
    size_t pos_last_n;

    // dopisz do początku bufora głównego ewentualnie zachowany fragment poprzedniego wiersza z bufora tymczasowego
    buffer_irc_recv.insert(0, buffer_irc_recv_tmp);
    buffer_irc_recv_tmp.clear();        // usuń tymczasową zawartość

    // wykryj, czy w buforze głównym jest niepełny wiersz (brak \r\n na końcu), jeśli tak, przenieś go do bufora tymczasowego
    pos_last_n = buffer_irc_recv.rfind("\n");
    if(pos_last_n != buffer_irc_recv.size() - 1)    // - 1, bo pozycja jest liczona od zera, a długość jest całkowitą liczbą zajmowanych bajtów
    {
        buffer_irc_recv_tmp.insert(0, buffer_irc_recv, pos_last_n + 1, buffer_irc_recv.size() - pos_last_n - 1);    // zachowaj ostatni niepełny wiersz
        buffer_irc_recv.erase(pos_last_n + 1, buffer_irc_recv.size() - pos_last_n - 1);         // oraz usuń go z głównego bufora
    }

    // zacznij od wyzerowania bufora powrotnego
    msg.clear();

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
    else if(find_value(strdup(buffer_irc_recv.c_str()), "PRIVMSG " + room + " :", "\n", f_value) == 0)
    {
        std::string nick_on_irc;
        find_value(strdup(buffer_irc_recv.c_str()), ":", "!", nick_on_irc);
        msg_color = UCC_TERM;
        msg = "<" + nick_on_irc + "> " + f_value;
        return;
    }

    // wykryj, gdy serwer odpowie ERROR
    if(find_value(strdup(buffer_irc_recv.c_str()), "ERROR :", "\n", f_value) == 0)
        irc_ok = false;
}
