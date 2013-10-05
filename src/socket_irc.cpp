#include <cstring>          // strlen()
#include <string>           // std::string
#include <iconv.h>          // konwersja kodowania znaków
#include "socket_irc.hpp"
#include "ucc_colors.hpp"


int socket_irc_init(int &socketfd_irc, struct sockaddr_in &irc_info)
{
    struct hostent *he;

    he = gethostbyname("czat-app.onet.pl");
    if(he == NULL)
        return 41;

    socketfd_irc = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd_irc == -1)
        return 42;

    irc_info.sin_family = AF_INET;
    irc_info.sin_port = htons(5015);
    irc_info.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(irc_info.sin_zero), '\0', 8);

    return 0;
}


bool socket_irc_connect(int &socketfd_irc, struct sockaddr_in &irc_info)
{
    if(connect(socketfd_irc, (struct sockaddr *)&irc_info, sizeof(struct sockaddr)) == -1)
    {
        close(socketfd_irc);
        return false;
    }

/*
    Od tego momentu zostaje nawiązane połączenie z IRC
*/

    return true;
}


bool socket_irc_send(int &socketfd_irc, bool &irc_ok, std::string &msg_sock, std::string &buffer_irc_send)
{
	int bytes_sent;

    // do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)
    buffer_irc_send += "\r\n";

    bytes_sent = send(socketfd_irc, buffer_irc_send.c_str(), buffer_irc_send.size(), 0);

    if(bytes_sent == -1)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_sock = "* Nie udało się wysłać danych do serwera";
        return false;
    }

    if(bytes_sent != (int)buffer_irc_send.size())
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_sock = "* Nie udało się wysłać wszystkich danych do serwera";
        return false;
    }

    return true;
}


bool socket_irc_recv(int &socketfd_irc, bool &irc_ok, std::string &msg_sock, std::string &buffer_irc_recv)
{
    int bytes_recv;

    // bufory tymczasowe
    char buffer_tmp_in[1500];   // tutaj pobierane są dane z serwera IRC
    char buffer_tmp_out[6000];  // tutaj wpisywane są dane po konwersji z ISO-8859-2 na UTF-8 (przyjęto najgorszy przypadek, gdyby było dużo znaków wielobajtowych)

    bytes_recv = recv(socketfd_irc, buffer_tmp_in, 1500 - 1, 0);
    buffer_tmp_in[bytes_recv] = '\0';

    if(bytes_recv == -1)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_sock = "* Nie udało się pobrać danych z serwera";
        return false;
    }

    if(bytes_recv == 0)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_sock = "* Serwer zakończył połączenie";
        return false;
    }

    // zamień ISO-8859-2 na UTF-8
    iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");

    char *buffer_tmp_in_ptr = buffer_tmp_in;
    size_t buffer_tmp_in_len = strlen(buffer_tmp_in);
    char *buffer_tmp_out_ptr = buffer_tmp_out;
    size_t buffer_tmp_out_len = sizeof(buffer_tmp_out);

    iconv(cd, &buffer_tmp_in_ptr, &buffer_tmp_in_len, &buffer_tmp_out_ptr, &buffer_tmp_out_len);
    *buffer_tmp_out_ptr = '\0';
    iconv_close(cd);

    // przekonwertowany bufor zwróć w buforze std::string
    buffer_irc_recv.clear();
    buffer_irc_recv = std::string(buffer_tmp_out);

    //usuń \2 z bufora
    while (buffer_irc_recv.find("\2") != std::string::npos)
        buffer_irc_recv.erase(buffer_irc_recv.find("\2"), 1);

    //usuń \r z bufora (w ncurses wyświetlenie tego na Linuksie powoduje, że linia jest niewidoczna)
    while (buffer_irc_recv.find("\r") != std::string::npos)
        buffer_irc_recv.erase(buffer_irc_recv.find("\r"), 1);

    return true;
}
