#include <sstream>          // std::string, std::stringstream, .find(), .insert(), .size()

#include "kbd_parser.hpp"
#include "auth.hpp"
#include "ucc_colors.hpp"


bool kbd_parser(std::string &kbd_buf, std::string &msg, std::string &msg_irc, std::string &my_nick, std::string &my_password, std::string &zuousername, std::string &cookies,
                std::string &uokey, bool &command_ok, bool &captcha_ready, bool &irc_ready, bool irc_ok, std::string &channel, bool &channel_ok, bool &command_me, bool &ucc_quit)
{
    // prosty interpreter wpisywanych poleceń (zaczynających się od / na pierwszej pozycji)

    // domyślnie nie zwracaj komunikatów (dodanie komunikatu następuje w obsłudze poleceń)
    msg.clear();
    msg_irc.clear();

    // domyślnie zakładamy, że wpisano polecenie
    command_ok = true;

    // domyślnie zakładamy, że nie wpisaliśmy polecenia /me
    command_me = false;

    // zapobiega wykonywaniu się reszty kodu, gdy w buforze nic nie ma
    if(kbd_buf.size() == 0)
    {
        msg = "# Błąd bufora klawiatury (bufor jest pusty)!";
        return false;
    }

    // wykryj, czy wpisano polecenie (znak / na pierwszej pozycji o tym świadczy)
    if(kbd_buf[0] != '/')
    {
        // jeśli brak połączenia z IRC, wiadomości nie można wysłać, więc pokaż ostrzeżenie
        if(! irc_ok)
        {
            msg = "# Najpierw się zaloguj.";
            return false;
        }

        // jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
        else if(! channel_ok)
        {
            msg = "# Nie jesteś w aktywnym pokoju.";
            return false;
        }

        // gdy połączono z IRC oraz jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w terminalu oraz polecenie do wysłania do IRC
        command_ok = false;     // wpisano tekst do wysłania do aktywnego pokoju
        msg = "<" + zuousername + "> " + kbd_buf;     // kolor nie jest zmieniany, bo do wyświetlenia komunikatu w terminalu używana jest funkcja, która w parametrze przyjmuje kolor
        msg_irc = "PRIVMSG " + channel + " :" + kbd_buf;
        return true;            // gdy wpisano zwykły tekst, zakończ
    }

/*
    wpisano / na początku, więc zinterpretuj polecenie (o ile istnieje)
*/

    int f_command_status;
    size_t pos_arg_start = 1;   // pozycja początkowa kolejnego argumentu
    std::string f_command;      // znalezione polecenie w buforze klawiatury (małe litery będą zamienione na wielkie)
    std::string f_command_org;  // j/w, ale małe litery nie są zamieniane na wielkie
    std::string f_arg;          // kolejne argumenty podane za poleceniem
    std::string r_args;         // pozostałe argumentu lub argument od pozycji w pos_arg_start zwracane w rest_args()
    std::string captcha, err_code;
    std::stringstream http_status_str;  // użyty pośrednio do zamiany int na std::string

    // pobierz wpisane polecenie
    f_command_status = find_command(kbd_buf, f_command, f_command_org, pos_arg_start);

    // wykryj błędnie wpisane polecenie
    if(f_command_status == 1)
    {
        msg = "# Polecenie błędne, sam znak / nie jest poleceniem.";
        return false;
    }

    else if(f_command_status == 2)
    {
        msg = "# Polecenie błędne, po znaku / nie może być spacji.";
        return false;
    }

/*
    wykonaj polecenie (o ile istnieje), poniższe polecenia są w kolejności alfabetycznej
*/

    if(f_command == "CAPTCHA")
    {
        if(irc_ok)
        {
            msg = "# Już zalogowano się.";
            return false;
        }

        if(! captcha_ready)
        {
            msg = "# Najpierw wpisz /connect";
            return false;
        }

        // pobierz wpisany kod captcha
        find_arg(kbd_buf, captcha, pos_arg_start, false);
        if(pos_arg_start == 0)
        {
            msg = "# Nie podano kodu, spróbuj jeszcze raz.";
            return false;
        }

        if(captcha.size() != 6)
        {
            msg = "# Kod musi mieć 6 znaków, spróbuj jeszcze raz.";
            return false;
        }

        // gdy kod wpisano i ma 6 znaków, wyślij go na serwer
        captcha_ready = false;  // zapobiega ponownemu wysłaniu kodu na serwer
        if(! http_auth_checkcode(cookies, captcha, msg))
        {
            return false;       // w przypadku błędu wróć z komunikatem w msg
        }
        if(! http_auth_getuo(cookies, my_nick, my_password, zuousername, uokey, msg))
        {
            return false;       // w przypadku błędu wróć z komunikatem w msg
        }
        irc_ready = true;       // gotowość do połączenia z IRC

    }   // CAPTCHA

    else if(f_command == "CONNECT")
    {
        if(irc_ok)
        {
            msg = "# Już zalogowano się.";
            return false;
        }

        if(my_nick.size() == 0)
        {
            msg = "# Nie wpisano nicka.";
            return false;
        }

        // gdy wpisano nick, rozpocznij łączenie
        if(! http_auth_init(cookies, msg))
        {
            return false;       // w przypadku błędu wróć z komunikatem w msg
        }

        // gdy wpisano hasło, wykonaj część dla nicka stałego
        if(my_password.size() != 0)
        {
            if(! http_auth_getsk(cookies, msg))
            {
                return false;   // w przypadku błędu wróć z komunikatem w msg
            }
            if(! http_auth_mlogin(cookies, my_nick, my_password, msg))
            {
                return false;   // w przypadku błędu wróć z komunikatem w msg
            }
// dodać override jako polecenie, gdy wykryty zostanie zalogowany nick
/*
            if(! http_auth_useroverride(cookies, my_nick, msg))
            {
                return false;   // w przypadku błędu wróć z komunikatem w msg
            }
*/
            if(! http_auth_getuo(cookies, my_nick, my_password, zuousername, uokey, msg))
            {
                return false;   // w przypadku błędu wróć z komunikatem w msg
            }
            irc_ready = true;       // gotowość do połączenia z IRC
        }

        // gdy nie wpisano hasła, wykonaj część dla nicka tymczasowego
        else
        {
            // pobierz captcha
            if(! http_auth_getcaptcha(cookies, msg))
            {
                return false;   // w przypadku błędu wróć z komunikatem w msg
            }
            msg = "# Przepisz kod z obrazka, w tym celu wpisz /captcha kod_z_obrazka";
            captcha_ready = true;       // można przepisać kod i wysłać na serwer
        }

    }   // CONNECT

    else if(f_command == "DISCONNECT")
    {
        // jeśli nie ma połączenia z IRC, rozłączenie nie ma sensu, więc pokaż ostrzeżenie
        if(! irc_ok)
        {
            msg = "# Nie zalogowano się.";
            return false;
        }

        // przygotuj polecenie do wysłania do IRC
        else
        {
            // jeśli podano argument (tekst pożegnalny), wstaw go
            if(rest_args(kbd_buf, pos_arg_start, r_args))
            {
                msg_irc = "QUIT :" + r_args;    // wstaw polecenie przed komunikatem pożegnalnym
            }
            // jeśli nie podano argumentu, wyślij samo polecenie
            else
            {
                msg_irc = "QUIT";
            }
        }

    }   // DISCONNECT

    else if(f_command == "HELP")
    {
        msg = "# Dostępne polecenia (w kolejności alfabetycznej):"
              "\n/captcha"
              "\n/connect"
              "\n/disconnect"
              "\n/help"
              "\n/join lub /j"
              "\n/me"
              "\n/nick"
              "\n/quit lub /q"
              "\n/raw"
              "\n/whois";
        // dopisać resztę poleceń

    }   // HELP

    else if(f_command == "JOIN" || f_command == "J")
    {
        // jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
        if(irc_ok)
        {
            find_arg(kbd_buf, channel, pos_arg_start, false);
            if(pos_arg_start == 0)
            {
                msg = "# Nie podano pokoju.";
                return false;
            }

            // gdy wpisano pokój, przygotuj komunikat do wysłania na serwer
            channel_ok = true;      // nad tym jeszcze popracować, bo wpisanie pokoju wcale nie oznacza, że serwer go zaakceptuje
            // jeśli nie podano # przed nazwą pokoju, dodaj #
            if(channel[0] != '#')
            {
                channel.insert(0, "#");
            }
            msg_irc = "JOIN " + channel;
        }

        // jeśli nie połączono z IRC, pokaż ostrzeżenie
        else
        {
            msg_connect_irc_err(msg);
            return false;
        }

    }   // JOIN

    else if(f_command == "ME")
    {
        // jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
        if(irc_ok)
        {
            // jeśli nie jest się w aktywnym pokoju, wiadomości nie można wysłać, więc pokaż ostrzeżenie
            if(! channel_ok)
            {
                msg = "# Nie jesteś w aktywnym pokoju.";
                return false;
            }

            // jeśli jest się w aktywnym pokoju, przygotuj komunikat do wyświetlenia w oknie terminala oraz polecenie dla IRC
            command_me = true;      // polecenie to wymaga, aby komunikat wyświetlić zgodnie z kodowaniem bufora w ISO-8859-2
            rest_args(kbd_buf, pos_arg_start, r_args);     // pobierz wpisany komunikat dla /me (nie jest niezbędny)
            msg = "* " + zuousername + " " + r_args;
            msg_irc = "PRIVMSG " + channel + " :\1ACTION " + r_args + "\1";
        }

        // jeśli nie połączono z IRC, pokaż ostrzeżenie
        else
        {
            msg_connect_irc_err(msg);
            return false;
        }

    }   // ME

    else if(f_command == "NICK")
    {
        // po połączeniu z IRC nie można zmienić nicka
        if(irc_ok)
        {
            msg = "# Po zalogowaniu się nie można zmienić nicka.";
            return false;
        }

        // nick można zmienić tylko, gdy nie jest się połączonym z IRC
        else
        {
            find_arg(kbd_buf, f_arg, pos_arg_start, false);
            if(pos_arg_start == 0)
            {
                if(my_nick.size() == 0)
                {
                    msg = "# Nie podano nicka.";
                    return false;
                }
                // wpisanie /nick bez parametrów, gdy wcześniej był już podany, powoduje wypisanie aktualnego nicka
                else
                {
                    if(my_password.size() == 0)
                    {
                        msg = "# Aktualny nick tymczasowy: " + my_nick;
                    }
                    else
                    {
                        msg = "# Aktualny nick stały: " + my_nick;
                    }
                    return true;
                }
            }

            else if(f_arg.size() < 3)
            {
                msg = "# Nick jest za krótki (minimalnie 3 znaki).";
                return false;
            }

            else if(f_arg.size() > 32)
            {
                msg = "# Nick jest za długi (maksymalnie 32 znaki).";
                return false;
            }

            // jeśli za nickiem wpisano hasło, pobierz je do bufora
            find_arg(kbd_buf, my_password, pos_arg_start, false);

            // przepisz nick do zmiennej
            my_nick = f_arg;
            if(my_password.size() == 0)
            {
                msg = "# Nowy nick tymczasowy: " + my_nick;
            }
            else
            {
                msg = "# Nowy nick stały: " + my_nick;
            }
        }

    }   // NICK

    else if(f_command == "QUIT" || f_command == "Q")
    {
        // jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
        if(irc_ok)
        {
            // jeśli podano argument (tekst pożegnalny), wstaw go
            if(rest_args(kbd_buf, pos_arg_start, r_args))
            {
                msg_irc = "QUIT :" + r_args;    // wstaw polecenie przed komunikatem pożegnalnym
            }
            // jeśli nie podano argumentu, wyślij samo polecenie
            else
            {
                msg_irc = "QUIT";
            }
        }
        // zamknięcie programu po ewentualnym wysłaniu polecenia do IRC
        ucc_quit = true;

    }   // QUIT

    else if(f_command == "RAW")
    {
        // jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
        if(irc_ok)
        {
            // jeśli nie podano parametrów, pokaż ostrzeżenie
            if(! rest_args(kbd_buf, pos_arg_start, r_args))
            {
                msg = "# Nie podano parametrów.";
                return false;
            }
            // polecenie do IRC
            msg_irc = r_args;
        }

        // jeśli nie połączono z IRC, pokaż ostrzeżenie
        else
        {
            msg_connect_irc_err(msg);
            return false;
        }

    }   // RAW

    else if(f_command == "WHOIS")
    {
        // jeśli połączono z IRC, przygotuj polecenie do wysłania do IRC
        if(irc_ok)
        {
            find_arg(kbd_buf, f_arg, pos_arg_start, false);
            if(pos_arg_start == 0)
            {
                msg = "# Nie podano nicka do sprawdzenia.";
                return false;
            }
            // polecenie do IRC
            msg_irc = "WHOIS " + f_arg + " " + f_arg;   // 2x nick, aby pokazało idle
        }

        // jeśli nie połączono z IRC, pokaż ostrzeżenie
        else
        {
            msg_connect_irc_err(msg);
            return false;
        }

    }   // WHOIS

    // gdy nie znaleziono polecenia
    else
    {
        msg = "# Nieznane polecenie: /" + f_command_org;     // tutaj pokaż oryginalnie wpisane polecenie
        return false;
    }

    return true;
}


void msg_connect_irc_err(std::string &msg)
{
    msg = "# Aby wysłać polecenie przeznaczone dla IRC, musisz się zalogować.";
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
        {
            f_command[i] = toupper(f_command[i]);
        }
    }

    // gdy coś było za poleceniem, tutaj będzie pozycja początkowa (ewentualne spacje będą usunięte w find_arg() )
    pos_arg_start = pos_command_end;

    return 0;
}


void find_arg(std::string &kbd_buf, std::string &f_arg, size_t &pos_arg_start, bool lower2upper)
{
    // pobierz argument z bufora klawiatury od pozycji w pos_arg_start, jeśli go nie ma, w pos_arg_start będzie 0

    // wstępnie usuń ewentualną poprzednią zawartość po użyciu tej funkcji
    f_arg.clear();

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

    f_arg.insert(0, kbd_buf, pos_arg_start, pos_arg_end - pos_arg_start);       // wstaw szukany argument

    // jeśli trzeba, zamień małe litery w argumencie na wielkie
    if(lower2upper)
    {
        for(int i = 0; i < (int)f_arg.size(); ++i)
        {
            if(islower(f_arg[i]))
            {
                f_arg[i] = toupper(f_arg[i]);
            }
        }
    }

    // wpisz nową pozycję początkową dla ewentualnego kolejnego argumentu
    pos_arg_start = pos_arg_end;
}


bool rest_args(std::string &kbd_buf, size_t pos_arg_start, std::string &f_rest)
{
    // pobierz resztę bufora od pozycji w pos_arg_start (czyli pozostałe argumenty lub argument)

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
