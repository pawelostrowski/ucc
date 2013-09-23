#include <cstring>      // strstr()
#include <sstream>      // std::stringstream, std::string
#include <fstream>      // std::ofstream, perror()
#include "auth_http.hpp"
#include "sockets.hpp"
#include "expression.hpp"


int http_1(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char c_buffer[50000];
    std::string data_send;

    header_get("kropka.onet.pl", "/_s/kropka/1?DV=czat/applet/FULL", cookies, data_send);   // utwórz zapytanie do wysłania

    socket_status = socket_a("kropka.onet.pl", "80", data_send, c_buffer, offset_recv);     // wyślij dane
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    cookies.clear();        // wyczyść bufor cookies
    cookies_status = find_cookies(c_buffer, cookies);       // z bufora c_buffer pobierz cookies
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies

    return 0;
}


int http_2(std::string &cookies)
{
    int socket_status, cookies_status;
    long offset_recv;
    char c_buffer[50000];
    char *gif_buffer;
    std::string data_send;

    header_get("czat.onet.pl", "/myimg.gif", cookies, data_send, true);

    socket_status = socket_a("czat.onet.pl", "80", data_send, c_buffer, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    cookies_status = find_cookies(c_buffer, cookies);
    if(cookies_status != 0)
        return cookies_status;      // kod błędu, gdy napotkano problem z cookies

    gif_buffer = strstr(c_buffer, "GIF");       // daj wskaźnik na początek obrazka
    if(gif_buffer == NULL)
        return 8;       // kod błędu, gdy nie znaleziono obrazka w buforze

    // zapisz obrazek z captcha na dysku
    std::ofstream gif_file(GIF_FILE, std::ios::binary);
    if(gif_file == NULL)
    {
        perror("gif_file: " GIF_FILE);
        return 9;
    }
    gif_file.write(gif_buffer, &c_buffer[offset_recv] - gif_buffer);    // &c_buffer[offset_recv] - gif_buffer <--- adres końca bufora - adres początku obrazka = rozmiar obrazka
    gif_file.close();

    return 0;
}


int http_3(std::string &cookies, std::string captcha_code, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char c_buffer[50000];
    std::string api_function, data_send;

    api_function = "api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha_code + "\";}";

    header_post("czat.onet.pl", cookies, api_function, data_send);

    socket_status = socket_a("czat.onet.pl", "80",data_send, c_buffer, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    // sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE)
    f_value_status = find_value(c_buffer, "err_code=\"", "\"", err_code);   // szukaj wartości między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
    if(f_value_status != 0)
        return f_value_status;      // kod błedu, gdy nie udało się pobrać err_code

    if(err_code != "TRUE")
        if(err_code != "FALSE")
            return 99;          // kod błedu, gdy serwer nie zwrócił wartości TRUE lub FALSE

    return 0;
}


int http_4(std::string &cookies, std::string &nick, std::string &zuousername, std::string &uokey, std::string &err_code)
{
    int socket_status, f_value_status;
    long offset_recv;
    char c_buffer[50000];
    std::string api_function, data_send;
    std::stringstream nick_length;

    nick_length << nick.size();

    api_function =  "api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + nick_length.str() + ":\""
                    + nick + "\";s:8:\"tempNick\";i:1;s:7:\"version\";s:22:\"1.1(20130621-0052 - R)\";}";

    header_post("czat.onet.pl", cookies, api_function, data_send);

    socket_status = socket_a("czat.onet.pl", "80", data_send, c_buffer, offset_recv);
    if(socket_status != 0)
        return socket_status;       // kod błędu, gdy napotkano problem z socketem

    // pobierz kod błędu
    f_value_status = find_value(c_buffer, "err_code=\"", "\"", err_code);
    if(f_value_status != 0)
        return f_value_status;      // kod błędu, gdy nie udało się pobrać err_code

    // sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
    if(err_code != "TRUE")
        return 0;               // 0, bo to nie jest błąd programu

    // pobierz uoKey
    f_value_status = find_value(c_buffer, "<uoKey>", "</uoKey>", uokey);
    if(f_value_status != 0)
        return f_value_status + 10; // kod błedu, gdy nie udało się pobrać uoKey (+10, aby można było go odróżnić od poprzedniego użycia find_value() )

    // pobierz zuoUsername (nick, który zwrócił serwer)
    f_value_status = find_value(c_buffer, "<zuoUsername>", "</zuoUsername>", zuousername);
    if(f_value_status != 0)
        return f_value_status + 20; // kod błędu, gdy serwer nie zwrócił nicka (+20, aby można było go odróżnić od poprzedniego użycia find_value() )

    return 0;
}
