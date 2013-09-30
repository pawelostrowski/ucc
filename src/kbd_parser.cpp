#include <sstream>          // std::string, std::stringstream, find(), erase(), c_str(), size()
#include "kbd_parser.hpp"
#include "ucc_colors.hpp"
#include "auth.hpp"


void kbd_parser(std::string &kbd_buf, std::string &msg, short &msg_color, std::string &cookies,
                std::string &nick, std::string &zuousername, bool &captcha_ok, bool &irc_ok, int socketfd_irc, bool &ucc_quit)
{
    // domyślnie nie zwracaj komunikatów (dodanie komunikatu następuje w obsłudze poleceń)
    msg.clear();

    // domyślny kolor komunikatów czerwony (bo takich komunikatów jest więcej)
    msg_color = UCC_RED;

    // zwykłe komunikaty wyślij do aktywnego pokoju (no i analogicznie do aktywnego okna)
    if(kbd_buf[0] != '/')   // sprawdź, czy pierwszy znak to / (jest to znak, który oznacza, że wpisujemy polecenie)
    {
        msg_color = UCC_MAGENTA;
        msg = zuousername + ": " + kbd_buf;
//        asyn_socket_send("PRIVMSG #scc :" + kbd_buf, socketfd);
//            show_buffer_send("PRIVMSG #scc :" + kbd_buf, active_window);
//        asyn_socket_send("PRIVMSG #Computers :" + kbd_buf, socketfd_irc, active_window);
        return;
    }

    int f_command_status;
    int http_status;
    size_t arg_start = 1;       // pozycja początkowa kolejnego argumentu
    std::string f_command;      // znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
    std::string f_command_org;  // j/w, ale małe litery nie są zamieniane na wielkie
    std::string f_arg;          // kolejne argumenty podane za poleceniem
    std::string captcha, err_code, uokey;
    std::stringstream http_status_str;  // użyty pośrednio do zamiany int na std::string

    // gdy pierwszym znakiem był / wykonaj obsługę polecenia
    f_command_status = find_command(kbd_buf, f_command, f_command_org, arg_start);      // pobierz polecenie z bufora klawiatury

    // wykryj błędnie wpisane polecenie
    if(f_command_status == 1)
    {
        msg = "* Polecenie błędne (sam znak / nie jest poleceniem)";
        return;
    }

    else if(f_command_status == 2)
    {
        msg = "* Polecenie błędne (po znaku / nie może być spacji)";
        return;
    }

    // wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej
    if(f_command == "CAPTCHA")
    {
        if(! captcha_ok)
        {
            msg = "* Najpierw wpisz /connect";
            return;
        }
        find_arg(kbd_buf, captcha, arg_start, false);
        if(arg_start == 0)
        {
            msg = "* Nie podano kodu, spróbuj jeszcze raz";
            return;
        }
        if(captcha.size() != 6)
        {
            msg = "* Kod musi mieć 6 znaków, spróbuj jeszcze raz";
            return;
        }
        // gdy kod wpisano i ma 6 znaków, wyślij go na serwer
        // http_3()
        http_status = http_3(cookies, captcha, err_code);
        if(err_code == "FALSE")
        {
            msg = "* Wpisany kod jest błędny, aby zacząć od nowa, wpisz /connect";
            return;
        }
        else if(http_status != 0)
        {
            http_status_str << http_status;
            msg = "* Błąd podczas wywoływania http_3(), kod błędu: " + http_status_str.str();
            return;
        }
        // http_4()
        http_status = http_4(cookies, nick, zuousername, uokey, err_code);
        if(err_code != "TRUE")
        {
            msg = "* Błąd serwera (nieprawidłowy nick?), kod błędu: " + err_code;
            return;
        }
        else if(http_status != 0)
        {
            http_status_str << http_status;
            msg = "* Błąd podczas wywoływania http_4(), kod błędu: " + http_status_str.str();
            return;
        }
        captcha_ok = false;     // zapobiega ponownemu wysłaniu kodu na serwer (jeśli chcemy inny kod, trzeba wpisać /connect)
        // socket_irc()
//        http_status = socket_irc(zuousername, uokey, socketfd_irc, active_window);
//        if(http_status != 0)
//        {
//            msg = "* Błąd podczas wywoływania socket_irc(), kod błędu: " + http_status;
//            return;
//        }
                    msg_color = UCC_BLUE;
                    msg = "* OK";
        return;
    }

    else if(f_command == "CONNECT")
    {
        if(nick.size() == 0)
        {
            msg = "* Nie wpisano nicka, wpisz /nick nazwa_nicka i dopiero /connect";
            return;
        }
        // http_1()
        http_status = http_1(cookies);
        if(http_status != 0)
        {
            http_status_str << http_status;
            "* Błąd podczas wywoływania http_1(), kod błędu: " + http_status_str.str();
            return;
        }
        // http_2()
        http_status = http_2(cookies);
        if(http_status != 0)
        {
            http_status_str << http_status;
            msg = "* Błąd podczas wywoływania http_2(), kod błędu: " + http_status_str.str();
            return;
        }
        msg_color = UCC_GREEN;
        msg = "* Przepisz kod z obrazka (wpisz /captcha kod_z_obrazka)";
        captcha_ok = true;      // kod wysłany
        return;
    }

    else if(f_command == "HELP")
    {
        msg_color = UCC_GREEN;
        msg = "* Dostępne polecenia (w kolejności alfabetycznej):"
              "\n/captcha"
              "\n/connect"
              "\n/help"
              "\n/join"
              "\n/nick"
              "\n/quit";
        // dopisać resztę poleceń
        return;
    }

    else if(f_command == "JOIN")
    {
        // tymczasowo!!!
//        asyn_socket_send("JOIN #Computers", socketfd_irc, active_window);
        return;
    }

    else if(f_command == "NICK")
    {
        find_arg(kbd_buf, nick, arg_start, false);
        if(arg_start == 0)
        {
            msg = "* Nie podano nicka";
            return;
        }
        if(nick.size() < 3)
        {
            msg = "* Nick jest za krótki (minimalnie 3 znaki)";
            nick.clear();   // nick jest nieprawidłowy, więc go usuń
            return;
        }
        if(nick.size() > 32)
        {
            msg = "* Nick jest za długi (maksymalnie 32 znaki)";
            nick.clear();   // nick jest nieprawidłowy, więc go usuń
            return;
        }
        // gdy nick wpisano, wyświetl go
        msg_color = UCC_GREEN;
        msg = "* Nowy nick: " + nick;
        return;
    }

    else if(f_command == "QUIT")
    {
        ucc_quit = true;
        return;
    }

    else
    {
        msg = "/" + f_command_org + ": nieznane polecenie";     // tutaj pokaż oryginalnie wpisane polecenie
    }

}


int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &arg_start)
{
    // polecenie może się zakończyć spacją (polecenie z parametrem ) lub końcem bufora (polecenie bez parametru)

    int kbd_buf_length;

    kbd_buf_length = kbd_buf.size();   // pobierz rozmiar bufora klawiatury

    // sprawdź, czy za poleceniem są jakieś znaki (sam znak / nie jest poleceniem)
    if(kbd_buf_length <= 1)
        return 1;

    // sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
    if(kbd_buf[1] == ' ')
        return 2;

    size_t pos_command_end;     // pozycja, gdzie się kończy polecenie
    int f_command_length;       // długość polecenia

    // wykryj pozycję końca polecenia
    pos_command_end = kbd_buf.find(" ");        // wykryj, gdzie jest spacja za poleceniem
    if(pos_command_end == std::string::npos)
        pos_command_end = kbd_buf_length;       // jeśli nie było spacji, koniec polecenia uznaje się za koniec bufora, czyli jego rozmiar

    f_command.clear();
    f_command.insert(0, kbd_buf, 1, pos_command_end - 1);      // wstaw szukane polecenie (- 1, bo pomijamy znak / )

    // znalezione polecenie zapisz w drugim buforze, który użyty zostanie, jeśli wpiszemy nieistniejące polecenie (pokazane będzie dokładnie tak,
    // jak je wpisaliśmy, bez konwersji małych liter na wielkie)
    f_command_org = f_command;

    // zamień małe litery w poleceniu na wielkie (łatwiej będzie je sprawdzać)
    f_command_length = f_command.size();
    for(int i = 0; i < f_command_length; ++i)
    {
        if(islower(f_command[i]))
            f_command[i] = toupper(f_command[i]);
    }

    // gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w find_arg() )
    arg_start = pos_command_end;

    return 0;
}


void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &arg_start, bool lower2upper)
{
    // pobierz argument z bufora klawiatury od pozycji w arg_start, jeśli go nie ma, w arg_start będzie 0

    int kbd_buf_length, f_arg_length;
    int arg_start_tmp;
    size_t arg_end;

    f_arg.clear();

    kbd_buf_length = kbd_buf.size();

    // jeśli pozycja w arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa), więc zakończ
    arg_start_tmp = arg_start;
    if(arg_start_tmp >= kbd_buf_length)
    {
        arg_start = 0;      // 0 oznacza, że nie było argumentu
        return;
    }

    // pomiń spacje pomiędzy poleceniem a argumentem lub między kolejnymi argumentami
    for(int i = arg_start; i < kbd_buf_length; ++i)
    {
        if(kbd_buf[arg_start] != ' ')   // gdy to wszystkie spacje, przejdź dalej
            break;

        ++arg_start;
    }

    // wykryj pozycję końca argumentu
    arg_end = kbd_buf.find(" ", arg_start);     // wykryj, gdzie jest spacja za poleceniem lub poprzednim argumentem
    if(arg_end == std::string::npos)
        arg_end = kbd_buf_length;               // jeśli nie było spacji, koniec argumentu uznaje się za koniec bufora, czyli jego rozmiar

    f_arg.clear();
    f_arg.insert(0, kbd_buf, arg_start, arg_end - arg_start);   // wstaw szukany argument

    f_arg_length = f_arg.size();    // pobierz rozmiar argumentu

    // zerowy rozmiar znalezionej pozycji oznacza, że wpisaliśmy same spacje (od miejsca szukania), więc zakończ
    if(f_arg_length == 0)
    {
        arg_start = 0;      // 0 oznacza, że nie było argumentu
        return;
    }

    // jeśli trzeba, zamień małe litery w argumencie na wielkie
    if(lower2upper)
    {
        for(int i = 0; i < f_arg_length; ++i)
        {
            if(islower(f_arg[i]))
                f_arg[i] = toupper(f_arg[i]);
        }
    }

    // wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
    arg_start = arg_end;
}
