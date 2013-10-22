#include <cstring>          // memcpy()
#include <sstream>          // std::string, std::stringstream
#include <fstream>          // std::ofstream
#include <cstdlib>          // system()

#include "auth.hpp"
#include "network.hpp"

#define FILE_GIF "/tmp/onetcaptcha.gif"


bool auth_code(std::string &authkey)
{
    if(authkey.size() != 16)        // AUTHKEY musi mieć dokładnie 16 znaków
        return false;

    const int f1[] = {29, 43,  7,  5, 52, 58, 30, 59, 26, 35, 35, 49, 45,  4, 22,  4,
                       0,  7,  4, 30, 51, 39, 16,  6, 32, 13, 40, 44, 14, 58, 27, 41,
                      52, 33,  9, 30, 30, 52, 16, 45, 43, 18, 27, 52, 40, 52, 10,  8,
                      10, 14, 10, 38, 27, 54, 48, 58, 17, 34,  6, 29, 53, 39, 31, 35,
                      60, 44, 26, 34, 33, 31, 10, 36, 51, 44, 39, 53,  5, 56};

    const int f2[] = { 7, 32, 25, 39, 22, 26, 32, 27, 17, 50, 22, 19, 36, 22, 40, 11,
                      41, 10, 10,  2, 10,  8, 44, 40, 51,  7,  8, 39, 34, 52, 52,  4,
                      56, 61, 59, 26, 22, 15, 17,  9, 47, 38, 45, 10,  0, 12,  9, 20,
                      51, 59, 32, 58, 19, 28, 11, 40,  8, 28,  6,  0, 13, 47, 34, 60,
                       4, 56, 21, 60, 59, 16, 38, 52, 61, 44,  8, 35,  4, 11};

    const int f3[] = {60, 30, 12, 34, 33,  7, 15, 29, 16, 20, 46, 25,  8, 31,  4, 48,
                       6, 44, 57, 16, 12, 58, 48, 59, 21, 32,  2, 18, 51,  8, 50, 29,
                      58,  6, 24, 34, 11, 23, 57, 43, 59, 50, 10, 56, 27, 32, 12, 59,
                      16,  4, 40, 39, 26, 10, 49, 56, 51, 60, 21, 37, 12, 56, 39, 15,
                      53, 11, 33, 43, 52, 37, 30, 25, 19, 55,  7, 34, 48, 36};

    const int p1[] = {11,  9, 12,  0,  1,  4, 10, 13,  3,  6,  7,  8, 15,  5,  2, 14};

    const int p2[] = { 1, 13,  5,  8,  7, 10,  0, 15, 12,  3, 14, 11,  2,  9,  6,  4};

    char c;
    char ai[16], ai1[16];

    for(int i = 0; i < 16; ++i)
    {
        c = authkey[i];             // std::string na char (po jednym znaku)
        // ASCII:    9        Z        =        7        0
        ai[i] = c > 57 ? c > 90 ? c - 61 : c - 55 : c - 48;
    }

    for(int i = 0; i < 16; ++i)
        ai[i] = f1[ai[i] + i];

    memcpy(ai1, ai, sizeof(ai1));   // skopiuj ai do ai1

    for(int i = 0; i < 16; ++i)
        ai[i] = (ai[i] + ai1[p1[i]]) % 62;

    for(int i = 0; i < 16; ++i)
        ai[i] = f2[ai[i] + i];

    memcpy(ai1, ai, sizeof(ai1));   // skopiuj ai do ai1

    for(int i = 0; i < 16; ++i)
        ai[i] = (ai[i] + ai1[p2[i]]) % 62;

    for(int i = 0; i < 16; ++i)
        ai[i] = f3[ai[i] + i];

    for(int i = 0; i < 16; ++i)
    {
        c = ai[i];
        // ASCII:    \n         $        =        7        0
        ai[i] = c >= 10 ? c >= 36 ? c + 61 : c + 55 : c + 48;
        authkey[i] = ai[i];         // char na std::string (po jednym znaku)
    }

    return true;
}


int find_value(char *buffer_recv, std::string expr_before, std::string expr_after, std::string &f_value)
{
    size_t pos_expr_before, pos_expr_after;     // pozycja początkowa i końcowa szukanych wyrażeń

    pos_expr_before = std::string(buffer_recv).find(expr_before);      // znajdź pozycję początku szukanego wyrażenia
    if(pos_expr_before == std::string::npos)
        return 1;           // kod błędu, gdy nie znaleziono początku szukanego wyrażenia

   // znajdź pozycję końca szukanego wyrażenia, zaczynając od znalezionego początku + jego jego długości
    pos_expr_after = std::string(buffer_recv).find(expr_after, pos_expr_before + expr_before.size());
    if(pos_expr_after == std::string::npos)
        return 2;           // kod błędu, gdy nie znaleziono końca szukanego wyrażenia

    // wstaw szukaną wartość
    f_value.clear();        // wyczyść bufor szukanej wartości
    f_value.insert(0, std::string(buffer_recv), pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());

    return 0;
}


bool http_auth_init(std::string &cookies, std::string &msg_err)
{
/*
    pobierz pierwsze 4 ciastka (onet_ubi, onetzuo_ticket, onet_cid, onet_sgn)
*/

    long offset_recv;
    char *buffer_recv;
    std::string msg_err_pre = "# http_auth_init: ";

    // wyczyść bufor cookies przed zapoczątkowaniem połączenia
    cookies.clear();

    buffer_recv = http_get_data("GET", "kropka.onet.pl", 80, "/_s/kropka/1?DV=czat/applet/FULL", "", cookies, true, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // nie wolno zapomnieć o zwolnieniu pamięci! (w przypadku błedu w http_get_data() nie trzeba zwalniać pamięci, bo funkcja zrobiła to sama)
    free(buffer_recv);

    return true;
}


bool http_auth_getcaptcha(std::string &cookies, std::string &msg_err)
{
/*
    pobierz captcha i kolejne, piąte ciastko (onet_sid), jednocześnie wysyłając pobrane poprzednio 4 ciastka
*/

    long offset_recv;
    char *buffer_recv;
    char *buffer_gif_ptr;
    int system_status;
    std::stringstream system_status_str;
    std::string msg_err_pre = "# http_auth_getcaptcha: ";

    buffer_recv = http_get_data("GET", "czat.onet.pl", 80, "/myimg.gif", "", cookies, true, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // daj wskaźnik na początek obrazka
    buffer_gif_ptr = strstr(buffer_recv, "GIF");
    if(buffer_gif_ptr == NULL)
    {
        msg_err = msg_err_pre + "Nie udało się pobrać obrazka z kodem do przepisania.";
        free(buffer_recv);
        return false;
    }

    // zapisz obrazek z captcha na dysku
    std::ofstream file_gif(FILE_GIF, std::ios::binary);
    if(file_gif == NULL)
    {
        msg_err = msg_err_pre + "Nie udało się zapisać obrazka z kodem do przepisania, sprawdź uprawnienia do " + FILE_GIF;
        free(buffer_recv);
        return false;
    }

    // &buffer_recv[offset_recv] - buffer_gif_ptr  <--- <adres końca bufora> - <adres początku obrazka> = <rozmiar obrazka>
    file_gif.write(buffer_gif_ptr, &buffer_recv[offset_recv] - buffer_gif_ptr);

    // zamknij plik po zapisaniu
    file_gif.close();

    free(buffer_recv);

    // wyświetl obrazek z kodem do przepisania
    system_status = system("/usr/bin/eog "FILE_GIF" 2>/dev/null &");	// to do poprawy, rozwiązanie tymczasowe!!!
    if(system_status != 0)
    {
        system_status_str << system_status;
        msg_err = "Proces uruchamiający obrazek do przepisania zakończył się błędem numer: " + system_status_str.str();
        return false;
    }

    return true;
}


bool http_auth_getsk(std::string &cookies, std::string &msg_err)
{
/*
    w przypadku stałego nicka nie trzeba pobierać captcha, ale potrzebne jest piąte ciastko (onet_sid), a można je uzyskać, pobierając z serwera mały obrazek
*/

    long offset_recv;
    char *buffer_recv;
    std::string msg_err_pre = "# http_auth_getsk: ";

    buffer_recv = http_get_data("GET", "czat.onet.pl", 80, "/sk.gif", "", cookies, true, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    free(buffer_recv);

    return true;
}


bool http_auth_checkcode(std::string &cookies, std::string &captcha, std::string &msg_err)
{
    long offset_recv;
    char *buffer_recv;
    std::string err_code;
    std::string msg_err_pre = "# http_auth_checkcode: ";

    buffer_recv = http_get_data("POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
                                "api_function=checkCode&params=a:1:{s:4:\"code\";s:6:\"" + captcha + "\";}",
                                 cookies, false, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // sprawdź, czy wpisany kod jest prawidłowy (wg odpowiedzi serwera: TRUE lub FALSE),
    // czyli pobierz wartość między wyrażeniami: err_code=" oraz " (np. err_code="TRUE" zwraca TRUE)
    if(find_value(buffer_recv, "err_code=\"", "\"", err_code) != 0)
    {
        msg_err = msg_err_pre + "Serwer nie zwrócił err_code.";
        free(buffer_recv);
        return false;
    }

    free(buffer_recv);

    // jeśli serwer zwrócił FALSE, oznacza to błędnie wpisany kod captcha
    if(err_code == "FALSE")
    {
        msg_err = "# Wpisany kod jest błędny, aby zacząć od nowa, wpisz /connect";  // tutaj msg_err_pre nie jest wymagany
        return false;
    }

    // brak TRUE oznacza błąd w odpowiedzi serwera
    if(err_code != "TRUE")
    {
        msg_err = msg_err_pre + "Serwer nie zwrócił oczekiwanego TRUE lub FALSE, zwrócona wartość: " + err_code;
        return false;
    }

    return true;
}


bool http_auth_mlogin(std::string &cookies, std::string my_nick, std::string my_password, std::string &msg_err)
{
/*
    w tym miejscu (logowanie nicka stałego) pobierane są kolejne dwa ciastka (onet_uoi, onet_uid)
*/

    long offset_recv;
    char *buffer_recv;
    std::string msg_err_pre = "# http_auth_mlogin: ";

    buffer_recv = http_get_data("POST", "secure.onet.pl", 443, "/mlogin.html",
                                "r=&url=&login=" + my_nick + "&haslo=" + my_password + "&app_id=20&ssl=1&ok=1",
                                 cookies, true, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    free(buffer_recv);

    return true;
}


bool http_auth_useroverride(std::string &cookies, std::string my_nick, std::string &msg_err)
{
/*
    funkcja ta przydaje się, gdy nas rozłączy, ale nie wyrzuci jeszcze z serwera (łączy od razu, bez komunikatu błędu o zalogowanym już nicku), dotyczy nicka stałego
*/

    long offset_recv;
    char *buffer_recv;
    std::stringstream my_nick_length;
    std::string msg_err_pre = "# http_auth_useroverride: ";

    my_nick_length << my_nick.size();

    buffer_recv = http_get_data("POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
                                "api_function=userOverride&params=a:1:{s:4:\"nick\";s:" + my_nick_length.str() + ":\"" + my_nick + "\";}",
                                 cookies, false, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    free(buffer_recv);

    return true;
}


bool http_auth_getuo(std::string &cookies, std::string my_nick, std::string my_password, std::string &zuousername, std::string &uokey, std::string &msg_err)
{
    long offset_recv;
    char *buffer_recv;
    std::stringstream my_nick_length;
    std::string temp_nick;
    std::string err_code;
    std::string msg_err_pre = "# http_auth_getuo: ";

    // wykryj, czy nick jest stały, czy tymczasowy (na podstawie obecności hasła)
    if(my_password.size() == 0)
    {
        temp_nick = "1";        // tymczasowy
        // jeśli podano nick (tymczasowy) z tyldą na początku, usuń ją, bo serwer takiego nicka nie akceptuje, mimo iż potem taki nick zwraca po zalogowaniu się
        if(my_nick[0] == '~')
        {
            my_nick.erase(0, 1);
        }
    }
    else
    {
        temp_nick = "0";        // stały
    }

    my_nick_length << my_nick.size();

    buffer_recv = http_get_data("POST", "czat.onet.pl", 80, "/include/ajaxapi.xml.php3",
                                "api_function=getUoKey&params=a:3:{s:4:\"nick\";s:" + my_nick_length.str() + ":\"" + my_nick
                                 + "\";s:8:\"tempNick\";i:" + temp_nick + ";s:7:\"version\";s:22:\"1.1(20130621-0052 - R)\";}",
                                 cookies, false, offset_recv, msg_err);
    if(buffer_recv == NULL)
    {
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // pobierz kod błędu
    if(find_value(buffer_recv, "err_code=\"", "\"", err_code) != 0)
    {
        msg_err = msg_err_pre + "Serwer nie zwrócił err_code.";
        free(buffer_recv);
        return false;
    }

    // sprawdź, czy serwer zwrócił wartość TRUE (brak TRUE może wystąpić np. przy błędnym nicku)
    if(err_code != "TRUE")
    {
        if(err_code == "-2")    // -2 oznacza nieprawidłowy nick (stały) lub hasło
        {
            msg_err = "# Błąd serwera (-2): nieprawidłowy nick lub hasło.";      // tutaj msg_err_pre nie jest wymagany
        }
        else if(err_code == "-4")   // -4 oznacza nieprawidłowe znaki w nicku tymczasowym
        {
            msg_err = "# Błąd serwera (-4): nick zawiera niedozwolone znaki.";   // tutaj msg_err_pre nie jest wymagany
        }
        else
        {
            msg_err = msg_err_pre + "Nieznany błąd serwera, kod błędu: " + err_code;
        }
        free(buffer_recv);
        return false;
    }

    // pobierz uoKey
    if(find_value(buffer_recv, "<uoKey>", "</uoKey>", uokey) != 0)
    {
        msg_err = msg_err_pre + "Serwer nie zwrócił uoKey.";
        free(buffer_recv);
        return false;
    }

    // pobierz zuoUsername (nick, który zwrócił serwer)
    if(find_value(buffer_recv, "<zuoUsername>", "</zuoUsername>", zuousername) != 0)
    {
        msg_err = msg_err_pre + "Serwer nie zwrócił zuoUsername.";
        free(buffer_recv);
        return false;
    }

    free(buffer_recv);

    return true;
}


bool irc_auth_1(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &msg_err)
{
    std::string msg_err_pre = "# irc_auth_1: ";

    // zacznij od ustanowienia poprawności połączenia z IRC, zostanie ono zmienione na niepowodzenie, gdy napotkamy błąd podczas któregoś etapu autoryzacji do IRC
    irc_ok = true;

    // zainicjalizuj gniazdo oraz połącz z IRC
    socketfd_irc = socket_init("czat-app.onet.pl", 5015, msg_err);
    if(socketfd_irc == 0)
    {
        irc_ok = false;
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // pobierz pierwszą odpowiedż serwera po połączeniu
    if(! irc_recv(socketfd_irc, irc_ok, buffer_irc_recv, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_recv() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    return true;
}


bool irc_auth_2(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &zuousername, std::string &msg_err)
{
    msg_err.clear();

    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string buffer_irc_send;
    std::string msg_err_pre = "# irc_auth_2: ";

    // wyślij: NICK <zuousername>
    buffer_irc_send = "NICK " + zuousername;
    buffer_irc_sent = buffer_irc_send;      // rozdzielono to w sten sposób, aby można było podejrzeć, co zostało wysłane do serwera (inf. do debugowania)
    if(! irc_send(socketfd_irc, irc_ok, buffer_irc_send, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_send() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // pobierz odpowiedź z serwera
    if(! irc_recv(socketfd_irc, irc_ok, buffer_irc_recv, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_recv() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    return true;
}


bool irc_auth_3(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &msg_err)
{
    msg_err.clear();

    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string buffer_irc_send;
    std::string msg_err_pre = "# irc_auth_3: ";

    // wyślij: AUTHKEY
    buffer_irc_send = "AUTHKEY";
    buffer_irc_sent = buffer_irc_send;
    if(! irc_send(socketfd_irc, irc_ok, buffer_irc_send, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_send() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    // pobierz odpowiedź z serwera (AUTHKEY)
    if(! irc_recv(socketfd_irc, irc_ok, buffer_irc_recv, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_recv() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    return true;
}


bool irc_auth_4(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_recv, std::string &buffer_irc_sent, std::string &msg_err)
{
    msg_err.clear();

    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    size_t raw_801, pos_authkey_start, pos_authkey_end;
    std::string buffer_irc_send;
    std::string authkey;
    std::string msg_err_pre = "# irc_auth_4: ";

    // wyszukaj AUTHKEY z odebranych danych w irc_auth_3(), przykładowa odpowiedź serwera:
    //  :cf1f1.onet 801 ~ucc :t9fSMnY5VQuwX1x9
    raw_801 = buffer_irc_recv.find("801", 0);       // znajdź raw 801
    if(raw_801 == std::string::npos)
    {
        irc_ok = false;
        msg_err = msg_err_pre + "Nie uzyskano AUTHKEY (brak odpowiedzi 801).";
        return false;
    }
    pos_authkey_start = buffer_irc_recv.find(":", raw_801);     // szukaj drugiego dwukropka
    if(pos_authkey_start == std::string::npos)
    {
        irc_ok = false;
        msg_err = msg_err_pre + "Problem ze znalezieniem AUTHKEY (nie znaleziono oczekiwanego dwukropka w odpowiedzi 801).";
        return false;
    }
    pos_authkey_end = buffer_irc_recv.find("\n", raw_801);      // szukaj końca wiersza
    if(pos_authkey_end == std::string::npos)
    {
        irc_ok = false;
        msg_err = msg_err_pre + "Uszkodzony rekord AUTHKEY (nie znaleziono kodu nowego wiersza w odpowiedzi 801).";
        return false;
    }

    // mamy pozycję początku i końca AUTHKEY, teraz wstaw AUTHKEY do bufora
    authkey.insert(0, buffer_irc_recv, pos_authkey_start + 1, pos_authkey_end - pos_authkey_start - 1);     // + 1, bo pomijamy dwukropek

    // konwersja AUTHKEY
    if(! auth_code(authkey))
    {
        irc_ok = false;
        msg_err = msg_err_pre + "AUTHKEY nie zawiera oczekiwanych 16 znaków (zmiana autoryzacji?).";
        return false;
    }

    // wyślij: AUTHKEY <AUTHKEY>
    buffer_irc_send = "AUTHKEY " + authkey;
    buffer_irc_sent = buffer_irc_send;
    if(! irc_send(socketfd_irc, irc_ok, buffer_irc_send, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_send() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    return true;
}


bool irc_auth_5(int &socketfd_irc, bool &irc_ok, std::string &buffer_irc_sent, std::string &zuousername, std::string &uokey, std::string &msg_err)
{
    msg_err.clear();

    // zakończ natychmiast, jeśli irc_ok jest ustawiony na false
    if(! irc_ok)
        return false;

    std::string buffer_irc_send;
    std::string msg_err_pre = "# irc_auth_5: ";

    // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>\r\nPROTOCTL ONETNAMESX
    buffer_irc_send = "USER * " + uokey + " czat-app.onet.pl :" + zuousername + "\r\nPROTOCTL ONETNAMESX";
    buffer_irc_sent = buffer_irc_send;
    if(! irc_send(socketfd_irc, irc_ok, buffer_irc_send, msg_err))
    {
        // usuń # i spację ze zwracanego stringa (bo socket_irc_send() używany jest też w innych miejscach, gdzie # i spacja są potrzebne)
        msg_err.erase(0, 2);
        msg_err = msg_err_pre + msg_err;
        return false;
    }

    return true;
}
