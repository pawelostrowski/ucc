#include <iostream>     // find(), erase(), std::string
#include "kbd_parser.hpp"
#include "sockets.hpp"


void kbd_parser(std::string kbd_buf, WINDOW *win_diag, int socketfd, bool &ucc_quit)
{
    int f_command_status;
    std::string f_command;      // znalezione polecenie w buforze klawiatury

    if(kbd_buf[0] != '/')   // sprawdź, czy pierwszy znak to / (jest to znak, który oznacza, że wpisujemy polecenie)
    {
//        asyn_socket_send("PRIVMSG #scc :" + kbd_buf, socketfd);
        wprintw(win_diag, "Ja: %s", kbd_buf.c_str());
        return;
    }

    else
    {
        f_command_status = find_command(kbd_buf, f_command);    // gdy pierwszym znakiem był / wykonaj obsługę polecenia
    }

    // wykryj błędnie wpisane polecenie
    if(f_command_status == 1)
    {
        wprintw(win_diag, "Polecenie błędne (po znaku / nie może być spacji)\n");
        return;
    }

    else if(f_command_status == 2)
    {
        wprintw(win_diag, "Polecenie błędne (sam znak / nie jest poleceniem)\n");
        return;
    }

    // wykonaj polecenie (o ile istnieje)
    if(f_command == "QUIT")
    {
        ucc_quit = true;
        return;
    }

    else if(f_command == "HELP")
    {
        wprintw(win_diag, "Dostępne polecenia:\n");
        // dopisać listę poleceń
        return;
    }

    else
    {
        wprintw(win_diag, "Polecenie %s nie istnieje\n", f_command.c_str());
    }

}


int find_command(std::string kbd_buf, std::string &f_command)
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

    // sprawdź, czy za poleceniem jest spacja
    pos_command_end = kbd_buf.find(" ");

    if(pos_command_end == std::string::npos)    // jeśli nie było spacji, znajdź kod "\n"
        pos_command_end = kbd_buf.find("\n");

    f_command.clear();
    f_command.insert(0, kbd_buf, 1, pos_command_end - 1);      // wstaw szukane polecenie (- 1, bo nie potrzebujemy spacji lub "\n")

    // zamień małe znaki w poleceniu na wielkie (łatwiej będzie je sprawdzać)
    f_command_length = f_command.size();
    for(int i = 0; i < f_command_length; ++i)
    {
        if(islower(f_command[i]))
            f_command[i] = toupper(f_command[i]);
    }

    return 0;
}
