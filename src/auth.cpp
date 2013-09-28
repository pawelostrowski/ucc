#include <cstring>          // strlen(), memcpy()
#include <sstream>          // std::string, std::stringstream
#include <fstream>          // std::ofstream
#include <cstdlib>          // system()
#include "auth.hpp"
#include "socket_http.hpp"


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


void header_get(std::string host, std::string data_get, std::string cookies, std::string &data_send, bool add_cookies)
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


void header_post(std::string cookies, std::string api_function, std::string &data_send)
{
    std::stringstream content_length;

    content_length.clear();
    data_send.clear();

    content_length << api_function.size();      // wczytaj długość zapytania

    data_send = "POST /include/ajaxapi.xml.php3 HTTP/1.1\r\n"
                "Host: czat.onet.pl\r\n"
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
    std::string cookie_tmp;

    // std::string(buffer_recv) zamienia C string na std::string
    pos_cookie_start = std::string(buffer_recv).find(COOKIE_STRING);   // znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
    if(pos_cookie_start == std::string::npos)
        return 1;           // kod błędu, gdy nie znaleziono cookie (pierwszego)

    do
    {
        pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);     // szukaj ";" od pozycji początku cookie
        if(pos_cookie_end == std::string::npos)
            return 2;       // kod błędu, gdy nie znaleziono oczekiwanego ";" na końcu każdego cookie

    cookie_tmp.clear();     // wyczyść bufor pomocniczy
    cookie_tmp.insert(0, std::string(buffer_recv), pos_cookie_start + strlen(COOKIE_STRING), pos_cookie_end - pos_cookie_start - strlen(COOKIE_STRING) + 1);   // skopiuj cookie
                                                                                                                                                               //  do bufora pomocniczego
    cookies += cookie_tmp;      // dopisz kolejny cookie do bufora
    pos_cookie_start = std::string(buffer_recv).find(COOKIE_STRING, pos_cookie_start + strlen(COOKIE_STRING));     // znajdź kolejny cookie
    } while(pos_cookie_start != std::string::npos);     // zakończ szukanie, gdy nie znaleziono kolejnego cookie

    return 0;
}


int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value)
{
    size_t pos_expr_before, pos_expr_after;     // pozycja początkowa i końcowa szukanych wyrażeń

    pos_expr_before = std::string(buffer_recv).find(expr_before);      // znajdź pozycję początku szukanego wyrażenia
    if(pos_expr_before == std::string::npos)
        return 3;           // kod błędu, gdy nie znaleziono początku szukanego wyrażenia

    pos_expr_after = std::string(buffer_recv).find(expr_after, pos_expr_before + expr_before.size());   // znajdź pozycję końca szukanego wyrażenia,
                                                                                                        //  zaczynając od znalezionego początku + jego jego długości
    if(pos_expr_after == std::string::npos)
        return 4;           // kod błędu, gdy nie znaleziono końca szukanego wyrażenia

    f_value.clear();        // wyczyść bufor szukanej wartości
    f_value.insert(0, std::string(buffer_recv), pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());   // wstaw szukaną wartość

    return 0;
}


int http_1(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char buffer_recv[50000];
    std::string data_send;

    header_get("kropka.onet.pl", "/_s/kropka/1?DV=czat/applet/FULL", cookies, data_send);   // utwórz zapytanie do wysłania

    socket_status = socket_http("kropka.onet.pl", data_send, buffer_recv, offset_recv);     // wyślij dane
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    cookies.clear();        // wyczyść bufor cookies
    cookies_status = find_cookies(buffer_recv, cookies);    // pobierz cookies z buffer_recv
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies (1 lub 2)

    return 0;
}


int http_2(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char buffer_recv[50000];
    char *buffer_gif;
    std::string data_send;

    header_get("czat.onet.pl", "/myimg.gif", cookies, data_send, true);

    socket_status = socket_http("czat.onet.pl", data_send, buffer_recv, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    cookies_status = find_cookies(buffer_recv, cookies);
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies (1 lub 2)

    buffer_gif = strstr(buffer_recv, "GIF");        // daj wskaźnik na początek obrazka
    if(buffer_gif == NULL)
        return 5;           // kod błędu, gdy nie znaleziono obrazka w buforze

    // zapisz obrazek z captcha na dysku
    std::ofstream file_gif(FILE_GIF, std::ios::binary);
    if(file_gif == NULL)
        return 6;           // kod błędu, gdy nie udało się zapisać pliku z obrazkiem

    file_gif.write(buffer_gif, &buffer_recv[offset_recv] - buffer_gif);     // &buffer_recv[offset_recv] - buffer_gif
                                                                            //  <--- adres końca bufora - adres początku obrazka = rozmiar obrazka
    file_gif.close();

    // wyświetl obrazek z kodem do przepisania
    system("/usr/bin/eog "FILE_GIF" 2>/dev/null &");	// to do poprawy, rozwiązanie tymczasowe!!!

    return 0;
}


int http_3(std::string &cookies, std::string captcha_code, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char buffer_recv[50000];
    std::string api_function, data_send;

    api_function = "api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha_code + "\";}";

    header_post(cookies, api_function, data_send);

    socket_status = socket_http("czat.onet.pl", data_send, buffer_recv, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    // sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE)
    f_value_status = find_value(buffer_recv, "err_code=\"", "\"", err_code);    // szukaj wartości między wyrażeniami:
                                                                                //  err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
    if(f_value_status != 0)
        return f_value_status;      // kod błedu, gdy nie udało się pobrać err_code (3 lub 4)

    if(err_code != "TRUE")
        if(err_code != "FALSE")
            return 7;               // kod błedu, gdy serwer nie zwrócił wartości TRUE lub FALSE

    return 0;
}


int http_4(std::string &cookies, std::string &nick, std::string &zuousername, std::string &uokey, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char buffer_recv[50000];
    std::string api_function, data_send;
    std::stringstream nick_length;

    nick_length << nick.size();

    api_function =  "api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + nick_length.str() + ":\""
                    + nick + "\";s:8:\"tempNick\";i:1;s:7:\"version\";s:22:\"1.1(20130621-0052 - R)\";}";

    header_post(cookies, api_function, data_send);

    socket_status = socket_http("czat.onet.pl", data_send, buffer_recv, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    // pobierz kod błędu
    f_value_status = find_value(buffer_recv, "err_code=\"", "\"", err_code);
    if(f_value_status != 0)
        return f_value_status;      // kod błędu, gdy nie udało się pobrać err_code (3 lub 4)

    // sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
    if(err_code != "TRUE")
        return 0;                   // 0, bo to nie jest błąd programu

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