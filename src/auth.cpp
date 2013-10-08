#include <cstring>          // strlen(), memcpy()
#include <sstream>          // std::string, std::stringstream
#include <fstream>          // std::ofstream
#include <cstdlib>          // system()
#include "auth.hpp"
#include "socket_http.hpp"
#include "socket_irc.hpp"


bool auth_code(std::string &authkey)
{
    if(authkey.size() != 16)        // AUTHKEY musi mieć dokładnie 16 znaków
        return false;

    const int f1[] = {29, 43,  7,  5, 52, 58, 30, 59, 26, 35, 35, 49, 45,  4, 22,  4,  0,  7,  4, 30,
                      51, 39, 16,  6, 32, 13, 40, 44, 14, 58, 27, 41, 52, 33,  9, 30, 30, 52, 16, 45,
                      43, 18, 27, 52, 40, 52, 10,  8, 10, 14, 10, 38, 27, 54, 48, 58, 17, 34,  6, 29,
                      53, 39, 31, 35, 60, 44, 26, 34, 33, 31, 10, 36, 51, 44, 39, 53,  5, 56};

    const int f2[] = { 7, 32, 25, 39, 22, 26, 32, 27, 17, 50, 22, 19, 36, 22, 40, 11, 41, 10, 10,  2,
                      10,  8, 44, 40, 51,  7,  8, 39, 34, 52, 52,  4, 56, 61, 59, 26, 22, 15, 17,  9,
                      47, 38, 45, 10,  0, 12,  9, 20, 51, 59, 32, 58, 19, 28, 11, 40,  8, 28,  6,  0,
                      13, 47, 34, 60,  4, 56, 21, 60, 59, 16, 38, 52, 61, 44,  8, 35,  4, 11};

    const int f3[] = {60, 30, 12, 34, 33,  7, 15, 29, 16, 20, 46, 25,  8, 31,  4, 48,  6, 44, 57, 16,
                      12, 58, 48, 59, 21, 32,  2, 18, 51,  8, 50, 29, 58,  6, 24, 34, 11, 23, 57, 43,
                      59, 50, 10, 56, 27, 32, 12, 59, 16,  4, 40, 39, 26, 10, 49, 56, 51, 60, 21, 37,
                      12, 56, 39, 15, 53, 11, 33, 43, 52, 37, 30, 25, 19, 55,  7, 34, 48, 36};

    const int p1[] = {11,  9, 12,  0,  1,  4, 10, 13,  3,  6,  7,  8, 15,  5,  2, 14};

    const int p2[] = { 1, 13,  5,  8,  7, 10,  0, 15, 12,  3, 14, 11,  2,  9,  6,  4};

    int i, j;
    int ai[16], ai1[16];
    char c;

    for(i = 0; i < 16; ++i)
    {
        c = authkey[i];             // std::string na char (po jednym znaku)
        ai[i] = (c > '9' ? c > 'Z' ? (c - 97) + 36 : (c - 65) + 10 : c - 48);
    }

    for(i = 0; i < 16; ++i)
        ai[i] = f1[ai[i] + i];

    memcpy(ai1, ai, sizeof(ai));    // skopiuj ai do ai1

    for(i = 0; i < 16; ++i)
        ai[i] = (ai[i] + ai1[p1[i]]) % 62;

    for(i = 0; i < 16; ++i)
        ai[i] = f2[ai[i] + i];

    memcpy(ai1, ai, sizeof(ai));    // skopiuj ai do ai1

    for(i = 0; i < 16; ++i)
        ai[i] = (ai[i] + ai1[p2[i]]) % 62;

    for(i = 0; i < 16; ++i)
        ai[i] = f3[ai[i] + i];

    for(i = 0; i < 16; ++i)
    {
        j = ai[i];
        ai[i] = j >= 10 ? j >= 36 ? (97 + j) - 36 : (65 + j) - 10 : 48 + j;
    }

    for(i = 0; i < 16; ++i)
        authkey[i] = ai[i];           // int na std::string (po jednym znaku)

    return true;
}


void header_get(std::string host, std::string data_get, std::string &data_send, std::string &cookies, bool add_cookies)
{
    data_send.clear();

    data_send = "GET " + data_get + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "Connection: close\r\n"
                "Cache-Control: no-cache\r\n"
                "Pragma: no-cache\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n";

    if(add_cookies)
        data_send += "Cookie:" + cookies + "\r\n";

    data_send += "\r\n";
}


void header_post(std::string host, std::string data_post, std::string &data_send, std::string &cookies, std::string &api_function)
{
    std::stringstream content_length;

    content_length.clear();

    content_length << api_function.size();      // wczytaj długość zapytania

    data_send.clear();

    data_send = "POST " + data_post + " HTTP/1.1\r\n"
                "Host: " + host + "\r\n"
                "Connection: close\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Content-Length: " + content_length.str() + "\r\n"      // content_length.str()  <--- zamienia liczbę na std::string
                "Cache-Control: no-cache\r\n"
                "Pragma: no-cache\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:24.0) Gecko/20100101 Firefox/24.0\r\n"
                "Cookie:" + cookies + "\r\n\r\n"
                + api_function;
}


int find_cookies(char *buffer_recv, std::string &cookies)
{
    size_t pos_cookie_start, pos_cookie_end;
    std::string cookie_string, cookie_tmp;

    cookie_string = "Set-Cookie:";

    // std::string(buffer_recv) zamienia C string na std::string
    pos_cookie_start = std::string(buffer_recv).find(cookie_string);    // znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
    if(pos_cookie_start == std::string::npos)
        return 1;           // kod błędu, gdy nie znaleziono cookie (pierwszego)

    do
    {
        pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);      // szukaj ";" od pozycji początku cookie
        if(pos_cookie_end == std::string::npos)
            return 2;       // kod błędu, gdy nie znaleziono oczekiwanego ";" na końcu każdego cookie

    cookie_tmp.clear();     // wyczyść bufor pomocniczy

    // skopiuj cookie do bufora pomocniczego
    cookie_tmp.insert(0, std::string(buffer_recv), pos_cookie_start + cookie_string.size(), pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);

    cookies += cookie_tmp;      // dopisz kolejny cookie do bufora

    pos_cookie_start = std::string(buffer_recv).find(cookie_string, pos_cookie_start + cookie_string.size());   // znajdź kolejny cookie

    } while(pos_cookie_start != std::string::npos);     // zakończ szukanie, gdy nie znaleziono kolejnego cookie

    return 0;
}


int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value)
{
    size_t pos_expr_before, pos_expr_after;     // pozycja początkowa i końcowa szukanych wyrażeń

    pos_expr_before = std::string(buffer_recv).find(expr_before);      // znajdź pozycję początku szukanego wyrażenia
    if(pos_expr_before == std::string::npos)
        return 3;           // kod błędu, gdy nie znaleziono początku szukanego wyrażenia

   // znajdź pozycję końca szukanego wyrażenia, zaczynając od znalezionego początku + jego jego długości
    pos_expr_after = std::string(buffer_recv).find(expr_after, pos_expr_before + expr_before.size());
    if(pos_expr_after == std::string::npos)
        return 4;           // kod błędu, gdy nie znaleziono końca szukanego wyrażenia

    f_value.clear();        // wyczyść bufor szukanej wartości

    // wstaw szukaną wartość
    f_value.insert(0, std::string(buffer_recv), pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());

    return 0;
}


int http_auth_1(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char buffer_recv[50000];
    std::string data_send;

    header_get("kropka.onet.pl", "/_s/kropka/1?DV=czat/applet/FULL", data_send, cookies);   // utwórz zapytanie do wysłania

    socket_status = socket_http("kropka.onet.pl", data_send, buffer_recv, offset_recv);     // wyślij dane
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem (31...37)

    cookies.clear();        // wyczyść bufor cookies
    cookies_status = find_cookies(buffer_recv, cookies);    // pobierz cookies z buffer_recv
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies (1 lub 2)

    return 0;
}


int http_auth_2(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char buffer_recv[50000];
    char *buffer_gif_ptr;
    std::string data_send;

    header_get("czat.onet.pl", "/myimg.gif", data_send, cookies, true);

    socket_status = socket_http("czat.onet.pl", data_send, buffer_recv, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem (31...37)

    cookies_status = find_cookies(buffer_recv, cookies);
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies (1 lub 2)

    buffer_gif_ptr = strstr(buffer_recv, "GIF");        // daj wskaźnik na początek obrazka
    if(buffer_gif_ptr == NULL)
        return 5;           // kod błędu, gdy nie znaleziono obrazka w buforze

    // zapisz obrazek z captcha na dysku
    std::ofstream file_gif(FILE_GIF, std::ios::binary);
    if(file_gif == NULL)
        return 6;           // kod błędu, gdy nie udało się zapisać pliku z obrazkiem (np. przez brak dostępu)

    // &buffer_recv[offset_recv] - buffer_gif_ptr  <--- <adres końca bufora> - <adres początku obrazka> = <rozmiar obrazka>
    file_gif.write(buffer_gif_ptr, &buffer_recv[offset_recv] - buffer_gif_ptr);

    file_gif.close();       // zamknij plik po zapisaniu

    // wyświetl obrazek z kodem do przepisania
    system("/usr/bin/eog "FILE_GIF" 2>/dev/null &");	// to do poprawy, rozwiązanie tymczasowe!!!

    return 0;
}


int http_auth_3(std::string &cookies, std::string &captcha, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char buffer_recv[50000];
    std::string api_function, data_send;

    api_function = "api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha + "\";}";

    header_post("czat.onet.pl", "/include/ajaxapi.xml.php3", data_send, cookies, api_function);

    socket_status = socket_http("czat.onet.pl", data_send, buffer_recv, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem (31...37)

    // sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE),
    // czyli pobierz wartość między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
    f_value_status = find_value(buffer_recv, "err_code=\"", "\"", err_code);
    if(f_value_status != 0)
        return f_value_status;      // kod błedu, gdy nie udało się pobrać err_code (3 lub 4)

    // jeśli serwer zwrócił FALSE, oznacza to błędnie wpisany kod captcha
    if(err_code == "FALSE")
        return 7;                   // kod błędu, gdy wpisany kod captcha był błędny

    // brak TRUE oznacza błąd w odpowiedzi serwera
    if(err_code != "TRUE")
        return 8;                   // kod błędu, gdy mimo poprawnie wpisanego kodu captcha serwer nie zwrócił TRUE

    return 0;
}


int http_auth_4(std::string &cookies, std::string &my_nick, std::string &zuousername, std::string &uokey, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char buffer_recv[50000];
    std::string api_function, data_send;
    std::stringstream my_nick_length;

    my_nick_length << my_nick.size();

    api_function =  "api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + my_nick_length.str() + ":\""
                    + my_nick + "\";s:8:\"tempNick\";i:1;s:7:\"version\";s:22:\"1.1(20130621-0052 - R)\";}";

    header_post("czat.onet.pl", "/include/ajaxapi.xml.php3", data_send, cookies, api_function);

    socket_status = socket_http("czat.onet.pl", data_send, buffer_recv, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem (31...37)

    // pobierz kod błędu
    f_value_status = find_value(buffer_recv, "err_code=\"", "\"", err_code);
    if(f_value_status != 0)
        return f_value_status;      // kod błędu, gdy nie udało się pobrać err_code (3 lub 4)

    // sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
    if(err_code != "TRUE")
        return 9;                   // kod błedu, gdy nie udało się wysłać nicka (prawdopodobnie był błędny)

    // pobierz uoKey
    f_value_status = find_value(buffer_recv, "<uoKey>", "</uoKey>", uokey);
    if(f_value_status != 0)
        return f_value_status + 10; // kod błedu, gdy nie udało się pobrać uoKey (13 lub 14)

    // pobierz zuoUsername (nick, który zwrócił serwer)
    f_value_status = find_value(buffer_recv, "<zuoUsername>", "</zuoUsername>", zuousername);
    if(f_value_status != 0)
        return f_value_status + 20; // kod błędu, gdy serwer nie zwrócił nicka (23 lub 24)

    return 0;
}


bool irc_auth_1(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, struct sockaddr_in &irc_info)
{
    std::string msg_pre = "* irc_auth_1() ";
    std::string msg_sock;

    // zacznij od ustanowienia poprawności połączenia z IRC, zostanie ono zmienione na niepowodzenie, gdy napotkamy błąd podczas któregoś etapu autoryzacji do IRC
    irc_ok = true;

    // połącz z IRC
    if(! socket_irc_connect(socketfd_irc, irc_info))
    {
        irc_ok = false;
        msg = msg_pre + "* Nie udało się połączyć z IRC";     // bez podawania koloru, bo w domyśle komunikaty z irc_auth są komunikatami błędów (na czerwono)
        return false;
    }

    // pobierz pierwszą odpowiedż serwera po połączeniu
    if(! socket_irc_recv(socketfd_irc, irc_ok, msg_sock, buffer_irc_recv))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    return true;
}


bool irc_auth_2(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &zuousername)
{
    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string msg_pre = "* irc_auth_2() ";
    std::string msg_sock;
    std::string buffer_irc_send;

    // wyślij: NICK <~nick>
    buffer_irc_send = "NICK " + zuousername;
    buffer_irc_sent = buffer_irc_send;      // rozdzielono to w sten sposób, aby można było podejrzeć, co zostało wysłane do serwera (inf. do debugowania)
    if(! socket_irc_send(socketfd_irc, irc_ok, msg_sock, buffer_irc_send))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    // pobierz odpowiedź z serwera
    if(! socket_irc_recv(socketfd_irc, irc_ok, msg_sock, buffer_irc_recv))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    return true;
}


bool irc_auth_3(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent)
{
    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string msg_pre = "* irc_auth_3() ";
    std::string msg_sock;
    std::string buffer_irc_send;

    // wyślij: AUTHKEY
    buffer_irc_send = "AUTHKEY";
    buffer_irc_sent = buffer_irc_send;
    if(! socket_irc_send(socketfd_irc, irc_ok, msg_sock, buffer_irc_send))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    // pobierz odpowiedź z serwera (AUTHKEY)
    if(! socket_irc_recv(socketfd_irc, irc_ok, msg_sock, buffer_irc_recv))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    return true;
}


bool irc_auth_4(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_recv, std::string &buffer_irc_sent)
{
    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string msg_pre = "* irc_auth_4() ";
    std::string msg_sock;
    std::string buffer_irc_send;
    size_t raw_801, pos_authkey_start, pos_authkey_end;
    std::string authkey;

    // wyszukaj AUTHKEY z odebranych danych w irc_auth_3(), przykładowa odpowiedź serwera:
    //  :cf1f1.onet 801 ~ucc :t9fSMnY5VQuwX1x9
    raw_801 = buffer_irc_recv.find("801", 0);       // znajdź raw 801
    if(raw_801 == std::string::npos)
    {
        irc_ok = false;
        msg = msg_pre + "* Nie uzyskano AUTHKEY (brak odpowiedzi 801)";
        return false;
    }
    pos_authkey_start = buffer_irc_recv.find(":", raw_801);     // szukaj drugiego dwukropka
    if(pos_authkey_start == std::string::npos)
    {
        irc_ok = false;
        msg = msg_pre + "* Problem ze znalezieniem AUTHKEY (nie znaleziono oczekiwanego dwukropka w odpowiedzi 801)";
        return false;
    }
    pos_authkey_end = buffer_irc_recv.find("\n", raw_801);      // szukaj końca wiersza
    if(pos_authkey_end == std::string::npos)
    {
        irc_ok = false;
        msg = msg_pre + "* Uszkodzony rekord AUTHKEY (nie znaleziono kodu nowego wiersza w odpowiedzi 801)";
        return false;
    }

    // mamy pozycję początku i końca AUTHKEY, teraz wstaw AUTHKEY do bufora
    authkey.insert(0, buffer_irc_recv, pos_authkey_start + 1, pos_authkey_end - pos_authkey_start - 1);     // + 1, bo pomijamy dwukropek

    // konwersja AUTHKEY
    if(! auth_code(authkey))
    {
        irc_ok = false;
        msg = msg_pre + "* AUTHKEY nie zawiera oczekiwanych 16 znaków (zmiana autoryzacji?)";
        return false;
    }

    // wyślij: AUTHKEY <AUTHKEY>
    buffer_irc_send = "AUTHKEY " + authkey;
    buffer_irc_sent = buffer_irc_send;
    if(! socket_irc_send(socketfd_irc, irc_ok, msg_sock, buffer_irc_send))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    return true;
}


bool irc_auth_5(int &socketfd_irc, bool &irc_ok, std::string &msg, std::string &buffer_irc_sent, std::string &zuousername, std::string &uokey)
{
    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string msg_pre = "* irc_auth_5() ";
    std::string msg_sock;
    std::string buffer_irc_send;

    // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>
    buffer_irc_send = "USER * " + uokey + " czat-app.onet.pl :" + zuousername;
    buffer_irc_sent = buffer_irc_send;
    if(! socket_irc_send(socketfd_irc, irc_ok, msg_sock, buffer_irc_send))
    {
        msg = msg_pre + msg_sock;
        return false;
    }

    return true;
}
