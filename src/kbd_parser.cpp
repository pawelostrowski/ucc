#include <sstream>          // std::string, std::stringstream, find(), erase(), c_str(), size()
#include "kbd_parser.hpp"
#include "ucc_colors.hpp"


void kbd_parser(std::string &kbd_buf, std::string &msg, short &msg_color, std::string &cookies, std::string &nick, std::string &zuousername,
                std::string &uokey, bool &captcha_ok, bool &irc_ready, bool irc_ok, std::string &room, bool &room_ok, bool &send_irc, bool &ucc_quit)
{
    // domyślnie nie zwracaj komunikatów (dodanie komunikatu następuje w obsłudze poleceń)
    msg.clear();

    // domyślny kolor komunikatów czerwony (bo takich komunikatów jest więcej)
    msg_color = UCC_RED;

    // zapobiega wykonywaniu się reszty kodu, gdy w buforze nic nie ma
    if(kbd_buf.size() == 0)
    {
        msg = "* Błąd bufora klawiatury (bufor jest pusty)!";
        return;
    }

    // domyślnie wiadomości nie są przeznaczone do wysłania do sieci IRC (chodzi o polecenia, a nie o zwykłe komunikaty)
    send_irc = false;

    // zwykłe komunikaty wyślij do aktywnego pokoju (no i analogicznie do aktywnego okna)
    if(kbd_buf[0] != '/')   // sprawdź, czy pierwszy znak to / (jest to znak, który oznacza, że wpisujemy polecenie)
    {
        // jeśli nie ma połączenia z IRC, pokaż ostrzeżenie
        if(! irc_ok)
        {
            msg = "* Aby wysłać wiadomość do IRC, musisz się zalogować";
            return;
        }

        // j/w ale dotyczy pokoju
        else if(! room_ok)
        {
            msg = "* Nie jesteś w żadnym aktywnym pokoju";
            return;
        }

        else
        {
            msg_color = UCC_MAGENTA;
            msg = zuousername + ": " + kbd_buf;
            return;
        }
    }

    int f_command_status;
    int http_status;
    size_t pos_arg_start = 1;   // pozycja początkowa kolejnego argumentu
    std::string f_command;      // znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
    std::string f_command_org;  // j/w, ale małe litery nie są zamieniane na wielkie
    std::string f_arg;          // kolejne argumenty podane za poleceniem
    std::string captcha, err_code;
    std::stringstream http_status_str;  // użyty pośrednio do zamiany int na std::string

    // gdy pierwszym znakiem był / wykonaj obsługę polecenia
    f_command_status = find_command(kbd_buf, f_command, f_command_org, pos_arg_start);      // pobierz polecenie z bufora klawiatury

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
        find_arg(kbd_buf, captcha, pos_arg_start, false);
        if(pos_arg_start == 0)
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
            captcha_ok = false;     // nie da się w ten sposób wysłać ponownie captcha (serwer tego nie akceptuje)
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
        irc_ready = true;       // gotowość do połączenia z IRC
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
            msg = "* Błąd podczas wywoływania http_1(), kod błędu: " + http_status_str.str();
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
              "\n/quit"
              "\n/raw";
        // dopisać resztę poleceń
    }

    else if(f_command == "JOIN")
    {
        find_arg(kbd_buf, room, pos_arg_start, false);
        if(pos_arg_start == 0)
        {
            msg = "* Nie podano pokoju";
            return;
        }
        // gdy wpisano pokój, przygotuj komunikat do wysłania na serwer
        send_irc = true;
        room_ok = true;     // nad tym jeszcze popracować, bo wpisanie pokoju wcale nie oznacza, że serwer go zaakceptuje
        msg = "JOIN " + room;
    }

    else if(f_command == "NICK")
    {
        find_arg(kbd_buf, nick, pos_arg_start, false);
        if(pos_arg_start == 0)
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
        // gdy wpisano nick (od 3 do 32 znaków), wyświetl go
        msg_color = UCC_GREEN;
        msg = "* Nowy nick: " + nick;
    }

    else if(f_command == "QUIT")
    {
        // jeśli podano argument (tekst pożegnalny), wstaw go
        if(insert_rest(kbd_buf, pos_arg_start, msg))
        {
            msg.insert(0, "QUIT :");    // a przed nim dodaj polecenie
        }
        // jeśli nie podano argumentu, wyślij samo polecenie
        else
        {
            msg = "QUIT";
        }
        send_irc = true;    // polecenie do IRC
        ucc_quit = true;    // zamknięcie programu po wysłaniu polecenia do IRC
    }

    else if(f_command == "RAW")
    {
        if(! insert_rest(kbd_buf, pos_arg_start, msg))
        {
            msg = "* Nie podano parametrów";
            return;
        }
        send_irc = true;    // polecenie do IRC (w msg)
    }

    else
    {
        msg = "/" + f_command_org + ": nieznane polecenie";     // tutaj pokaż oryginalnie wpisane polecenie
    }

}


int find_command(std::string &kbd_buf, std::string &f_command, std::string &f_command_org, size_t &pos_arg_start)
{
    // polecenie może się zakończyć spacją (polecenie z parametrem ) lub końcem bufora (polecenie bez parametru)

    // sprawdź, czy za poleceniem są jakieś znaki (sam znak / nie jest poleceniem)
    if(kbd_buf.size() <= 1)
        return 1;

    // sprawdź, czy za / jest spacja (polecenie nie może zawierać spacji po / )
    if(kbd_buf[1] == ' ')
        return 2;

    size_t pos_command_end;     // pozycja, gdzie się kończy polecenie

    // wykryj pozycję końca polecenia
    pos_command_end = kbd_buf.find(" ");        // wykryj, gdzie jest spacja za poleceniem
    if(pos_command_end == std::string::npos)
        pos_command_end = kbd_buf.size();       // jeśli nie było spacji, koniec polecenia uznaje się za koniec bufora, czyli jego rozmiar

    f_command.clear();
    f_command.insert(0, kbd_buf, 1, pos_command_end - 1);      // wstaw szukane polecenie (- 1, bo pomijamy znak / )

    // znalezione polecenie zapisz w drugim buforze, który użyty zostanie, jeśli wpiszemy nieistniejące polecenie (pokazane będzie dokładnie tak,
    // jak je wpisaliśmy, bez konwersji małych liter na wielkie)
    f_command_org = f_command;

    // zamień małe litery w poleceniu na wielkie (łatwiej będzie je sprawdzać)
    for(int i = 0; i < (int)f_command.size(); ++i)
    {
        if(islower(f_command[i]))
            f_command[i] = toupper(f_command[i]);
    }

    // gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w find_arg() )
    pos_arg_start = pos_command_end;

    return 0;
}


void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper)
{
    // pobierz argument z bufora klawiatury od pozycji w pos_arg_start, jeśli go nie ma, w pos_arg_start będzie 0

    // jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa), więc zakończ
    if(pos_arg_start >= kbd_buf.size())
    {
        pos_arg_start = 0;      // 0 oznacza, że nie było argumentu
        return;
    }

    // pomiń spacje pomiędzy poleceniem a argumentem lub pomiędzy kolejnymi argumentami (z uwzględnieniem rozmiaru bufora, aby nie czytać poza nim)
    while(kbd_buf[pos_arg_start] == ' ' && pos_arg_start < kbd_buf.size())
        ++pos_arg_start;        // kolejny znak w buforze

    // jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
    if(pos_arg_start == kbd_buf.size())
    {
        pos_arg_start = 0;      // 0 oznacza, że nie było argumentu
        return;
    }

    size_t pos_arg_end;

    // wykryj pozycję końca argumentu
    pos_arg_end = kbd_buf.find(" ", pos_arg_start);     // wykryj, gdzie jest spacja za poleceniem lub poprzednim argumentem
    if(pos_arg_end == std::string::npos)
        pos_arg_end = kbd_buf.size();           // jeśli nie było spacji, koniec argumentu uznaje się za koniec bufora, czyli jego rozmiar

    f_arg.clear();
    f_arg.insert(0, kbd_buf, pos_arg_start, pos_arg_end - pos_arg_start);       // wstaw szukany argument

    // jeśli trzeba, zamień małe litery w argumencie na wielkie
    if(lower2upper)
    {
        for(int i = 0; i < (int)f_arg.size(); ++i)
        {
            if(islower(f_arg[i]))
                f_arg[i] = toupper(f_arg[i]);
        }
    }

    // wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
    pos_arg_start = pos_arg_end;
}


bool insert_rest(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest)
{
    // pobierz resztę bufora od pozycji w pos_arg_start

    // jeśli pozycja w pos_arg_start jest równa wielkości bufora klawiatury, oznacza to, że nie ma argumentu (tym bardziej, gdy jest większa), więc zakończ
    if(pos_arg_start >= kbd_buf.size())
        return false;

    // znajdź miejsce, od którego zaczynają się znaki różne od spacji
    while(kbd_buf[pos_arg_start] == ' ' && pos_arg_start < kbd_buf.size())
        ++pos_arg_start;

    // jeśli po pominięciu spacji pozycja w pos_arg_start jest równa wielkości bufora, oznacza to, że nie ma szukanego argumentu, więc zakończ
    if(pos_arg_start == kbd_buf.size())
        return false;

    // wstaw pozostałą część bufora
    f_rest.clear();
    f_rest.insert(0, kbd_buf, pos_arg_start, kbd_buf.size() - pos_arg_start);

    return true;
}
