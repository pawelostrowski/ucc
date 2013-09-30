#include <iostream>     // find(), erase(), std::string
#include "kbd_parser.hpp"
#include "sockets.hpp"
#include "auth.hpp"


void kbd_parser(WINDOW *active_room, bool use_colors, int &socketfd_irc, fd_set &readfds, std::string kbd_buf, std::string &cookies,
                std::string &nick, std::string &zuousername, std::string room, bool &captcha_ok, bool &irc_ok, bool &ucc_quit)
{
    int f_command_status;
    int http_status;
    size_t arg_start = 0;   // pozycja początkowa kolejnego argumentu
    std::string f_command;  // znalezione polecenie w buforze klawiatury
    std::string f_arg;      // kolejne argumenty podane za poleceniem
    std::string captcha, err_code, uokey;

    if(kbd_buf[0] != '/')   // sprawdź, czy pierwszy znak to / (jest to znak, który oznacza, że wpisujemy polecenie)
    {
        wattrset(active_room, COLOR_PAIR(5));
        wprintw(active_room, "%s: %s", zuousername.c_str(), kbd_buf.c_str());
//        asyn_socket_send("PRIVMSG #scc :" + kbd_buf, socketfd);
//            show_buffer_send("PRIVMSG #scc :" + kbd_buf, active_room);
        // usuń kod "\n" z końca bufora (zostanie on zastąpiony "\r\n" w poniższej funkcji)
        kbd_buf.erase(kbd_buf.size(), 1);
//        asyn_socket_send("PRIVMSG #Computers :" + kbd_buf, socketfd_irc, active_room);
        return;
    }

    else
    {
        f_command_status = find_command(kbd_buf, f_command, arg_start);    // gdy pierwszym znakiem był / wykonaj obsługę polecenia
    }

    // wykryj błędnie wpisane polecenie
    if(f_command_status == 1)
    {
        if(use_colors)
            wattrset(active_room, COLOR_PAIR(1));
        else
            wattrset(active_room, A_NORMAL);
        wprintw(active_room, "* Polecenie błędne (po znaku / nie może być spacji)\n");
        return;
    }

    else if(f_command_status == 2)
    {
        if(use_colors)
            wattrset(active_room, COLOR_PAIR(1));
        else
            wattrset(active_room, A_NORMAL);
        wprintw(active_room, "* Polecenie błędne (sam znak / nie jest poleceniem)\n");
        return;
    }

    // wykonaj polecenie (o ile istnieje)
    else if(f_command == "CAPTCHA")
    {
        if(! captcha_ok)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Najpierw wpisz /connect\n");
            return;
        }
        find_arg(kbd_buf, captcha, arg_start, false);
        if(arg_start == 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Nie podano kodu, spróbuj jeszcze raz\n");
            return;
        }
        if(captcha.size() != 6)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Kod musi mieć 6 znaków, spróbuj jeszcze raz\n");
            return;
        }
        // gdy kod wpisano i ma 6 znaków, wyślij go na serwer
        // http_3()
        http_status = http_3(cookies, captcha, err_code);
        if(http_status != 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Błąd podczas wywoływania http_3(), kod błędu: %d\n", http_status);
            return;
        }
        if(err_code == "FALSE")
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Wpisany kod jest błędny, aby zacząć od nowa, wpisz /connect\n");
            return;
        }
        // http_4()
        http_status = http_4(cookies, nick, zuousername, uokey, err_code);
        if(http_status != 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Błąd podczas wywoływania http_4(), kod błędu: %d\n", http_status);
            return;
        }
        if(err_code != "TRUE")
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Błąd serwera (nieprawidłowy nick?): %s\n", err_code.c_str());
            return;
        }
        captcha_ok = false;     // zapobiega ponownemu wysłaniu kodu na serwer (jeśli chcemy inny kod, trzeba wpisać /connect)
        // socket_irc()
//        http_status = socket_irc(zuousername, uokey, socketfd_irc, active_room);
        if(http_status != 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Błąd podczas wywoływania socket_irc(), kod błędu: %d\n", http_status);
            return;
        }
        return;
    }

    else if(f_command == "CONNECT")
    {
        if(nick.size() == 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Nie wpisano nicka, wpisz /nick nazwa_nicka i dopiero /connect\n");
            return;
        }
        // http_1()
        http_status = http_1(cookies);
        if(http_status != 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Błąd podczas wywoływania http_1(), kod błędu: %d\n", http_status);
            return;
        }
        // http_2()
        http_status = http_2(cookies);
        if(http_status != 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Błąd podczas wywoływania http_2(), kod błędu: %d\n", http_status);
            return;
        }
        if(use_colors)
            wattrset(active_room, COLOR_PAIR(2));
        else
            wattrset(active_room, A_NORMAL);
        wprintw(active_room, "* Przepisz kod z obrazka (wpisz /captcha kod_z_obrazka)\n");
//        system("/usr/bin/eog "FILE_GIF" 2>/dev/null &");	// to do poprawy, rozwiązanie tymczasowe!!!
        captcha_ok = true;
        return;
    }

    else if(f_command == "HELP")
    {
        if(use_colors)
            wattrset(active_room, COLOR_PAIR(2));
        else
            wattrset(active_room, A_NORMAL);
        wprintw(active_room, "* Dostępne polecenia (w kolejności alfabetycznej):\n"
                             "/captcha\n/connect\n/help\n/join\n/nick\n/quit\n");
        // dopisać listę poleceń
        return;
    }

    else if(f_command == "JOIN")
    {
        // tymczasowo!!!
//        asyn_socket_send("JOIN #Computers", socketfd_irc, active_room);
        return;
    }

    else if(f_command == "NICK")
    {
        find_arg(kbd_buf, nick, arg_start, false);
        if(arg_start == 0)
        {
            if(use_colors)
                wattrset(active_room, COLOR_PAIR(1));
            else
                wattrset(active_room, A_NORMAL);
            wprintw(active_room, "* Nie podano nicka\n");
            return;
        }
        // gdy nick wpisano, wyświetl go
        if(use_colors)
            wattrset(active_room, COLOR_PAIR(2));
        else
            wattrset(active_room, A_NORMAL);
        wprintw(active_room, "* Nowy nick: %s\n", nick.c_str());
        return;
    }

    else if(f_command == "QUIT")
    {
        ucc_quit = true;
        return;
    }

    else
    {
        if(use_colors)
            wattrset(active_room, COLOR_PAIR(1));
        else
            wattrset(active_room, A_NORMAL);
        wprintw(active_room, "/%s: nieznane polecenie\n", f_command.c_str());
    }

}


int find_command(std::string kbd_buf, std::string &f_command, size_t &arg_start)
{
    // polecenie może się zakończyć spacją (polecenie z parametrem lub parametrami) lub kodem "\n" (polecenie bez parametru)

    // sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
    if(kbd_buf[1] == ' ')
        return 1;
    // sprawdź, czy za / wpisaliśmy inne znaki (nie może być "\n", bo sam znak / nie jest poleceniem)
    else if(kbd_buf[1] == '\n')
        return 2;

    size_t pos_command_end;     // pozycja, gdzie się kończy polecenie
    int f_command_length;

    // wykryj pozycję końca polecenia
    pos_command_end = kbd_buf.find(" ");
    if(pos_command_end == std::string::npos)    // jeśli nie było spacji, znajdź kod "\n"
        pos_command_end = kbd_buf.find("\n");

    f_command.clear();
    f_command.insert(0, kbd_buf, 1, pos_command_end - 1);      // wstaw szukane polecenie (- 1, pomijamy znak / )

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


void find_arg(std::string kbd_buf, std::string &f_arg, size_t &arg_start, bool lower2upper)
{
    // pobierz argument z bufora klawiatury od pozycji w arg_start

    int kbd_buf_length, f_arg_length;
    int arg_start_tmp;
    size_t arg_end;

    f_arg.clear();

    kbd_buf_length = kbd_buf.size();

    // gdyby pozycja w arg_start była większa od rozmiaru bufora klawiatury, zakończ
    arg_start_tmp = arg_start;
    if(arg_start_tmp > kbd_buf_length)
    {
        arg_start = 0;      // 0 oznacza, że nie było argumentu, bo pozycja była za daleko względem bufora
        return;
    }

    // pomiń spacje między poleceniem a argumentem lub między kolejnymi argumentami
    for(int i = arg_start; i < kbd_buf_length; ++i)
    {
        if(kbd_buf[arg_start] == '\n')  // kod "\n" przerywa (oznacza, że nie było argumentu)
        {
            arg_start = 0;  // wpisanie zera oznacza, że nie ma więcej argumentów lub szukanego argumentu nie było (były same spacje)
            return;
        }
        if(kbd_buf[arg_start] != ' ')   // gdy to wszystkie spacje, przejdź dalej
            break;
        ++arg_start;
    }

    // wykryj pozycję końca argumentu
    arg_end = kbd_buf.find(" ", arg_start);
    if(arg_end == std::string::npos)        // jeśli nie było spacji za argumentem, znajdź kod "\n"
        arg_end = kbd_buf.find("\n", arg_start);

    f_arg.clear();
    f_arg.insert(0, kbd_buf, arg_start, arg_end - arg_start);   // wstaw szukany argument

    // jeśli trzeba, zamień małe litery w argumencie na wielkie
    if(lower2upper)
    {
        f_arg_length = f_arg.size();
        for(int i = 0; i < f_arg_length; ++i)
        {
            if(islower(f_arg[i]))
                f_arg[i] = toupper(f_arg[i]);
        }
    }

    // wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
    arg_start = arg_end;
}
