#include <cstring>      // strlen()
#include <sstream>      // std::stringstream, std::string
#include "expression.hpp"

#define COOKIE_STRING "Set-Cookie:"


int find_cookies(char *buffer_recv, std::string &cookies)
{
    size_t pos_cookie_start, pos_cookie_end;
    std::string cookie_tmp;

    // std::string(buffer_recv) zamienia C string na std::string
    pos_cookie_start = std::string(buffer_recv).find(COOKIE_STRING);   // znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
    if(pos_cookie_start == std::string::npos)
        return 11;      // kod błędu, gdy nie znaleziono cookie (pierwszego)

    do
    {
        pos_cookie_end = std::string(buffer_recv).find(";", pos_cookie_start);     // szukaj ";" od pozycji początku cookie
        if(pos_cookie_end == std::string::npos)
            return 12;      // kod błędu, gdy nie znaleziono oczekiwanego ";" na końcu każdego cookie

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
        return 21;      // kod błędu, gdy nie znaleziono początku szukanego wyrażenia

    pos_expr_after = std::string(buffer_recv).find(expr_after, pos_expr_before + expr_before.size());   // znajdź pozycję końca szukanego wyrażenia,
                                                                                                        //  zaczynając od znalezionego początku + jego jego długości
    if(pos_expr_after == std::string::npos)
        return 22;      // kod błędu, gdy nie znaleziono końca szukanego wyrażenia

    f_value.clear();    // wyczyść bufor szukanej wartości
    f_value.insert(0, std::string(buffer_recv), pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());   // wstaw szukaną wartość

    return 0;
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
