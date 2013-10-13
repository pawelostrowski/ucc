#include <cstring>          // memset(), memcpy()
#include <sstream>          // std::string, std::stringstream
#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <openssl/ssl.h>
#include "sockets.hpp"


bool socket_init(int &socketfd, std::string host, short port, std::string &msg_err)
{
/*
    utwórz gniazdo (socket) oraz połącz się z hostem
*/

    struct hostent *host_info;
    struct sockaddr_in serv_info;

    // pobierz adres IP hosta
    host_info = gethostbyname(host.c_str());
    if(host_info == NULL)
    {
        msg_err = "Nie udało się pobrać informacji o hoście " + host + " (sprawdź swoje połączenie internetowe)";
        return false;
    }

    // utwórz deskryptor gniazda (socket)
    socketfd = socket(PF_INET, SOCK_STREAM, 0);     // SOCK_STREAM - TCP, SOCK_DGRAM - UDP
    if(socketfd == -1)
    {
        msg_err = "Nie udało się utworzyć deskryptora gniazda";
        return false;
    }

    serv_info.sin_family = PF_INET;
    serv_info.sin_port = htons(port);
    serv_info.sin_addr = *((struct in_addr *) host_info->h_addr);
    bzero(&(serv_info.sin_zero), 8);

    // połącz z hostem
    if(connect(socketfd, (struct sockaddr *) &serv_info, sizeof(struct sockaddr)) == -1)
    {
        close(socketfd);
        msg_err = "Nie udało się połączyć z hostem " + host;
        return false;
    }

    return true;
}


bool http_get_data(std::string method, std::string host, short port, std::string stock, std::string content, std::string &cookies, bool get_cookies,
                   char *buffer_recv, long &offset_recv, std::string &msg_err)
{
    if(method != "GET" && method != "POST")
    {
        msg_err = "Nieobsługiwana metoda " + method;
        return false;
    }

    int socketfd;               // deskryptor gniazda (socket)
    int bytes_sent, bytes_recv;
    char buffer_tmp[1500];      // bufor tymczasowy pobranych danych
    bool first_recv = true;     // czy to pierwsze pobranie w pętli
    std::string data_send;      // dane do wysłania do hosta
    std::stringstream content_length;

    // utwórz gniazdo (socket) oraz połącz się z hostem
    if(! socket_init(socketfd, host, port, msg_err))
        return false;       // zwróć komunikat błędu w msg_err

    // utwórz dane do wysłania do hosta
    data_send =  method + " " + stock + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n"
                "Cache-Control: no-cache\r\n"
                "Connection: close\r\n";

    if(method == "POST")
    {
        content_length << content.size();       // wczytaj długość zapytania
        data_send += "Content-Length: " + content_length.str() + "\r\n";    // content_length.str()  <--- zamienia liczbę na std::string
    }

    if(cookies.size() != 0)
        data_send += "Cookie:" + cookies + "\r\n";

    data_send += "\r\n";

    if(content.size() != 0)
        data_send += content;

    // połączenie na porcie różnym od 443 uruchomi transmisję nieszyfrowaną
    if(port != 443)
    {
        // wyślij dane do hosta
        bytes_sent = send(socketfd, data_send.c_str(), data_send.size(), 0);
        if(bytes_sent == -1)
        {
            close(socketfd);
            msg_err = "Nie udało się wysłać danych do hosta " + host;
            return false;
        }

        // sprawdź, czy wysłana ilość bajtów jest taka sama, jaką chcieliśmy wysłać
        if(bytes_sent != (int)data_send.size())     // (int) konwertuje zwracaną wartość na int
        {
            close(socketfd);
            msg_err = "Nie udało się wysłać wszystkich danych do hosta " + host;
            return false;
        }

        // poniższa pętla pobiera dane z hosta do bufora aż do napotkania 0 pobranych bajtów (gdy host zamyka połączenie)
        offset_recv = 0;        // offset pobranych danych (istotne do określenia później rozmiaru pobranych danych)
        do
        {
            // pobierz odpowiedź od hosta wraz z liczbą pobranych bajtów
            bytes_recv = recv(socketfd, buffer_tmp, 1500, 0);
            // sprawdź, czy pobieranie danych się powiodło
            if(bytes_recv == -1)
            {
                close(socketfd);
                msg_err = "Nie udało się pobrać danych z hosta " + host;
                return false;
            }
            // sprawdź, przy pierwszym obiegu pętli, czy pobrano jakieś dane
            if(first_recv)
            {
                if(bytes_recv == 0)
                {
                    close(socketfd);
                    msg_err = "Host " + host + " zakończył połączenie";
                    return false;
                }
            }
            first_recv = false;     // kolejne pobrania nie spowodują błędu zerowego rozmiaru pobranych danych
            memcpy(buffer_recv + offset_recv, buffer_tmp, bytes_recv);      // pobrane dane "dopisz" do bufora
            offset_recv += bytes_recv;      // zwiększ offset pobranych danych (sumarycznych, nie w jednym obiegu pętli)
        } while(bytes_recv != 0);

        buffer_recv[offset_recv] = '\0';

        // zamknij połączenie z hostem
        close(socketfd);

    }   // if(port != 443)

    // połączenie na porcie 443 uruchomi transmisję szyfrowaną (SSL)
    else if(port == 443)
    {
        SSL *ssl_handle;
        SSL_CTX *ssl_context;

        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();

        ssl_context = SSL_CTX_new(SSLv23_client_method());
        if(ssl_context == NULL)
        {
            msg_err = "error: SSL_CTX_new";
            return false;
        }

        ssl_handle = SSL_new(ssl_context);
        if(ssl_handle == NULL)
        {
            msg_err = "error: SSL_new";
            return false;
        }

        if(! SSL_set_fd(ssl_handle, socketfd))
        {
            msg_err = "error: SSL_set_fd";
            return false;
        }

        if(SSL_connect(ssl_handle) != 1)
        {
            msg_err = "error: SSL_connect";
            return false;
        }

        // wyślij dane do hosta
        SSL_write(ssl_handle, data_send.c_str(), data_send.size());

        // pobierz odpowiedź
        bytes_recv = SSL_read(ssl_handle, buffer_recv, 1500 - 1);
        buffer_recv[bytes_recv] = '\0';

        // zamknij połączenie z hostem
        close(socketfd);

        SSL_shutdown(ssl_handle);
        SSL_free(ssl_handle);
        SSL_CTX_free(ssl_context);

    }   // else if(port == 443)

    // jeśli trzeba, wyciągnij cookies z bufora
    if(get_cookies)
    {
        if(! find_cookies(buffer_recv, cookies, msg_err))
        {
            return false;       // zwróć komunikat błędu w msg_err
        }
    }

    return true;
}


bool find_cookies(char *buffer_recv, std::string &cookies, std::string &msg_err)
{
    size_t pos_cookie_start, pos_cookie_end;
    std::string cookie_string, cookie_tmp;

    cookie_string = "Set-Cookie:";      // celowo bez spacji na końcu, bo każde cookie będzie dopisywane ze spacją na początku

    // znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
    pos_cookie_start = std::string(buffer_recv).find(cookie_string);    // std::string(buffer_recv) zamienia C string na std::string
    if(pos_cookie_start == std::string::npos)
    {
        msg_err = "Nie znaleziono żadnego cookie";
        return false;
    }

    do
    {
        // szukaj ";" od pozycji początku cookie
        pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);
        if(pos_cookie_end == std::string::npos)
        {
            msg_err = "Problem z cookie, brak wymaganego średnika na końcu";
            return false;
        }

        // skopiuj cookie do bufora pomocniczego
        cookie_tmp.clear();     // wyczyść bufor pomocniczy
        cookie_tmp.insert(0, std::string(buffer_recv), pos_cookie_start + cookie_string.size(), pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

        // dopisz kolejne cookie do bufora
        cookies += cookie_tmp;

        // znajdź kolejne cookie
        pos_cookie_start = std::string(buffer_recv).find(cookie_string, pos_cookie_start + cookie_string.size());

    } while(pos_cookie_start != std::string::npos);     // zakończ szukanie, gdy nie znaleziono kolejnego cookie

    return true;
}


bool irc_send(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_send, std::string &msg_err)
{
	int bytes_sent;

    // do każdego zapytania dodaj znak nowego wiersza oraz przejścia do początku linii (aby nie trzeba było go dodawać poza funkcją)
    buffer_irc_send += "\r\n";

    bytes_sent = send(socketfd_irc, buffer_irc_send.c_str(), buffer_irc_send.size(), 0);

    if(bytes_sent == -1)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_err = "# Nie udało się wysłać danych do serwera, rozłączono";
        return false;
    }

    if(bytes_sent == 0)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_err = "# Podczas próby wysłania danych serwer zakończył połączenie";
        return false;
    }

    if(bytes_sent != (int)buffer_irc_send.size())
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_err = "# Nie udało się wysłać wszystkich danych do serwera, rozłączono";
        return false;
    }

    return true;
}


bool irc_recv(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err)
{
    int bytes_recv;
    char buffer_tmp[1500];

    bytes_recv = recv(socketfd_irc, buffer_tmp, 1500 - 1, 0);
    buffer_tmp[bytes_recv] = '\0';

    if(bytes_recv == -1)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_err = "# Nie udało się pobrać danych z serwera, rozłączono";
        return false;
    }

    if(bytes_recv == 0)
    {
        close(socketfd_irc);
        irc_ok = false;
        msg_err = "# Podczas próby pobrania danych serwer zakończył połączenie";
        return false;
    }

    // odebrane dane zwróć w buforze std::string
    buffer_irc_recv.clear();
    buffer_irc_recv = std::string(buffer_tmp);

    //usuń \2 z bufora (występuje zaraz po zalogowaniu się do IRC w komunikacie powitalnym)
    while (buffer_irc_recv.find("\2") != std::string::npos)
        buffer_irc_recv.erase(buffer_irc_recv.find("\2"), 1);

    //usuń \r z bufora (w ncurses wyświetlenie tego na Linuksie powoduje, że linia jest niewidoczna)
    while (buffer_irc_recv.find("\r") != std::string::npos)
        buffer_irc_recv.erase(buffer_irc_recv.find("\r"), 1);

    return true;
}
