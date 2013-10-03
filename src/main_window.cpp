#include <cstring>          // memset()
#include <string>           // std::string, setlocale()
#include <cerrno>           // errno
#include <sys/select.h>     // select()
#include <iconv.h>
#include "main_window.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "socket_irc.hpp"
#include "ucc_colors.hpp"


int main_window(bool use_colors)
{
    freopen("/dev/tty", "r", stdin);    // zapobiega zapętleniu się programu po wpisaniu w terminalu czegoś w stylu 'echo text | ucc'

    setlocale(LC_ALL, "");  // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    // inicjalizacja ncurses
    if(! initscr())
        return 1;

    bool ucc_quit = false;      // aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
    bool command_ok = false;    // true, gdy wpisano polecenie
    bool captcha_ok = false;    // stan wczytania captcha (jego pobranie z serwera)
    bool irc_ready = false;     // gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
    bool irc_ok = false;        // stan połączenia z czatem
    bool send_irc = false;      // true oznacza, że irc_parser() zwrócił PONG do wysłania do IRC
    bool room_ok = false;       // stan wejścia do pokoju (kanału)
    bool command_me = false;    // true oznacza, że wpisano polecenie /me, które trzeba wyświetlić z uwzględnieniem kodowania bufora w ISO-8859-2
    int term_y, term_x;         // wymiary terminala
    int cur_y, cur_x;           // aktualna pozycja kursora
    int kbd_buf_pos = 0;        // początkowa pozycja bufora klawiatury (istotne podczas używania strzałek, Home, End, Delete itd.)
    int kbd_buf_max = 0;        // początkowy maksymalny rozmiar bufora klawiatury
    int key_code;               // kod ostatnio wciśniętego klawisza
    std::string kbd_buf;        // bufor odczytanych znaków z klawiatury
    std::string key_code_tmp;   // tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)
    std::string msg;            // komunikat do wyświetlenia z którejś z wywoływanych funkcji w main_window() (opcjonalny)
    short msg_color;            // kolor komunikatu z zainicjalizowanej pary kolorów (można posługiwać się prefiksem UCC_)
    std::string msg_irc;        // komunikat (polecenie) do wysłania do IRC po wywołaniu kbd_parser() (opcjonalny)
    std::string zuousername = "Niezalogowany";
    std::string cookies, nick, uokey, authkey, room, data_sent;
    int socketfd_irc;           // gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() )
    char buffer_irc_recv[1500];

    std::string buffer_irc;

    // inicjalizacja gniazda (socket) używanego w połączeniu IRC
    struct sockaddr_in www;
    if(socket_irc_init(socketfd_irc, www) != 0)     // PRZENIEŚĆ W MIEJSCE URUCHAMIANIA POŁĄCZENIA Z IRC
    {
        endwin();
        return 3;
    }

    fd_set readfds;         // deskryptor dla select()
    fd_set readfds_tmp;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);    // klawiatura (stdin)

    raw();                  // zablokuj Ctrl-C i Ctrl-Z
    keypad(stdscr, TRUE);   // klawisze funkcyjne będą obsługiwane
    noecho();               // nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)

    // sprawdź, czy terminal obsługuje kolory, jeśli tak, włącz kolory oraz zainicjalizuj podstawową parę kolorów,
    // ale tylko wtedy, gdy uruchomiliśmy main_window() z use_colors = true, gdy terminal nie obsługuje kolorów, check_colors() zwróci false
    if(use_colors)
        use_colors = check_colors();

    // utwórz okno, w którym będą komunikaty serwera oraz inne (np. diagnostyczne)
    getmaxyx(stdscr, term_y, term_x);   // pobierz wymiary terminala (okna głównego)
    WINDOW *win_diag;
    win_diag = newwin(term_y - 3, term_x, 1, 0);
    scrollok(win_diag, TRUE);           // włącz przewijanie w tym oknie

    // jeśli terminal obsługuje kolory, poniższy komunikat powitalny wyświetl w kolorze zielonym
    wattrset_color(win_diag, use_colors, UCC_GREEN);
    wprintw(win_diag, "Ucieszony Chat Client\n"
                      "* Aby rozpocząć, wpisz:\n"
                      "/nick nazwa_nicka\n"
                      "/connect\n"
                      "* Następnie przepisz kod z obrazka, w tym celu wpisz:\n"
                      "/captcha kod_z_obrazka\n"
                      "* Aby zobaczyć dostępne polecenia, wpisz:\n"
                      "/help\n"
                      "* Aby zakończyć działanie programu, wpisz:\n"
                      "/quit\n");

    wrefresh(stdscr);
    wrefresh(win_diag);

    // zapamiętaj aktualną pozycję kursora w oknie diagnostycznym
    getyx(win_diag, cur_y, cur_x);

/*
    Tymczasowe wskaźniki pomocnicze, usunąć po testach
*/
    int ix = 0, iy = 0;
/*
    Koniec wskaźników pomocniczych
*/

    while(! ucc_quit)
    {
        if(irc_ready)
        {
            FD_SET(socketfd_irc, &readfds);  // gniazdo IRC (socket)
            irc_ready = false;
        }

        readfds_tmp = readfds;

        // wykryj zmianę rozmiaru okna terminala
        if(is_term_resized(term_y, term_x))
        {
            getmaxyx(stdscr, term_y, term_x);       // pobierz nowe wymiary terminala (okna głównego) po jego zmianie
            wresize(stdscr, term_y, term_x);        // zmień rozmiar okna głównego po zmianie rozmiaru okna terminala
            wresize(win_diag, term_y - 3, term_x);  // j/w, ale dla okna diagnostycznego
        }

        // paski (jeśli terminal obsługuje kolory, paski będą niebieskie)
        wattrset_color(stdscr, use_colors, UCC_BLUE_WHITE);
        wattron(stdscr, A_REVERSE);
        wmove(stdscr, 0, 0);
        for(int i = 0; i < term_x; i++)
            wprintw(stdscr, " ");
        wmove(stdscr, term_y - 2, 0);
        for(int i = 0; i < term_x; i++)
            wprintw(stdscr, " ");


/*
    Tymczasowo pokazuj informacje pomocnicze, usunąć po testach
*/
        wmove(stdscr, term_y - 2, 0);
        wprintw(stdscr, "sum: %d, kbd: %d, irc: %d", ix + iy, ix, iy);
/*
    Koniec informacji tymczasowych
*/

        // wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
        wprintw_iso2utf(stdscr, use_colors, UCC_TERM, term_y - 1, 0, "[" + zuousername + "] " + kbd_buf);
        wclrtoeol(stdscr);              // pozostałe znaki w wierszu wykasuj
        wmove(stdscr, term_y - 1, kbd_buf_pos + zuousername.size() + 3); // ustaw kursor w obecnie przetwarzany znak (+ długość nicka, nawias i spacja)

        wrefresh(win_diag);
        wrefresh(stdscr);

        // czekaj na aktywność klawiatury lub gniazda (socket)
        if(select(socketfd_irc + 1, &readfds_tmp, NULL, NULL, NULL) == -1)
        {
            // sygnał SIGWINCH (zmiana rozmiaru okna terminala) powoduje, że select() zwraca -1, więc trzeba to wykryć, aby nie wywalić programu w kosmos
            if(errno == EINTR)      // Interrupted system call (wywołany np. przez SIGWINCH)
            {
                wrefresh(win_diag);
                wrefresh(stdscr);   // odświeżenie w tym miejscu jest wymagane, gdy zmienimy rozmiar terminala
                getch();            // ignoruj KEY_RESIZE
                continue;           // wróć do początku pętli while()
            }
            // inny błąd select() powoduje zakończenie działania programu
            else
            {
                delwin(win_diag);
                endwin();           // zakończ tryb ncurses
                return 2;
            }
        }

        // klawiatura
        if(FD_ISSET(0, &readfds_tmp))
        {
            key_code = getch();

            if(key_code == KEY_LEFT)                // Left Arrow
            {
                if(kbd_buf_pos > 0)
                    --kbd_buf_pos;
            }

            else if(key_code == KEY_RIGHT)          // Right Arrow
            {
                if(kbd_buf_pos < kbd_buf_max)
                    ++kbd_buf_pos;
            }

            else if(key_code == KEY_BACKSPACE)      // Backspace
            {
                if(kbd_buf_pos > 0)
                {
                    --kbd_buf_pos;
                    --kbd_buf_max;
                    kbd_buf.erase(kbd_buf_pos, 1);
                }
            }

            else if(key_code == KEY_DC)             // Delete
            {
                if(kbd_buf_pos < kbd_buf_max)
                {
                    --kbd_buf_max;
                    kbd_buf.erase(kbd_buf_pos, 1);
                }
            }

            else if(key_code == KEY_HOME)           // Home
            {
                kbd_buf_pos = 0;
            }

            else if(key_code == KEY_END)            // End
            {
                kbd_buf_pos = kbd_buf_max;
            }

            else if(key_code == KEY_PPAGE)          // PageUp
            {
                //wscrl(win_diag, 1);
            }

            else if(key_code == KEY_NPAGE)          // PageDown
            {
                //wscrl(win_diag, -1);
            }

            else if(key_code == '\n')               // Enter
            {
                if(kbd_buf.size() != 0)     // wykonaj obsługę bufora tylko, gdy coś w nim jest
                {
                    // "wyczyść" pole wpisywanego tekstu (aby nie było widać zwłoki, np. podczas pobierania obrazka z kodem do przepisania)
                    wmove(stdscr, term_y - 1, zuousername.size() + 3); // ustaw kursor za nickiem i spacją za nawiasem
                    clrtoeol();
                    wrefresh(stdscr);
                    // wykonaj obsługę bufora (zidentyfikuj polecenie)
                    kbd_parser(kbd_buf, msg, msg_color, msg_irc, nick, zuousername, cookies, uokey, command_ok, captcha_ok, irc_ready, irc_ok, room, room_ok, command_me, ucc_quit);
                    // jeśli wpisano zwykły tekst (nie polecenie), pokaż go wraz z nickiem i wyślij polecenie do IRC (wykrycie, czy połączono się z IRC oraz czy otwarty
                    //  jest aktywny pokój jest wykonywane w kbd_parser(), przy błędzie nie jest ustawiany command_ok, aby pokazać komunikat poniżej)
                    if(! command_ok)
                    {
                        // pokaż komunikat z uwzględnieniem tego, że w buforze jest kodowanie ISO-8859-2
                        wprintw_iso2utf(win_diag, use_colors, UCC_MAGENTA, cur_y, cur_x, msg + "\n");
                        // wyślij wiadomość na serwer
                        socket_irc_send(socketfd_irc, irc_ok, msg_irc, data_sent);
                    }
                    // gdy kbd_parser() zwrócił jakiś komunikat przeznaczony do wyświetlenia na terminalu, pokaż go
                    else if(msg.size() != 0)
                    {
                        // komunikaty zwykłe (nie /me) wyświetlaj zgodnie z kodowaniem UTF-8
                        if(! command_me)
                        {
                            wattrset_color(win_diag, use_colors, msg_color);
                            mvwprintw(win_diag, cur_y, cur_x, "%s\n", msg.c_str());
                        }
                        // komunikat związany z poleceniem /me trzeba wyświetlić z uwzględnieniem tego, że bufor kodowany jest w ISO-8859-2
                        else
                        {
                            wprintw_iso2utf(win_diag, use_colors, msg_color, cur_y, cur_x, msg + "\n");
                        }
                    }
                    // gdy kbd_parser() zwrócił jakiś komunikat przeznaczony do wysłania do IRC i nie jest to zwykły tekst wypisany w wprintw_iso2utf(), wyślij go do IRC
                    //  (stan połączenia do IRC wykrywany jest w kbd_parser(), więc nie trzeba się obawiać, że komunikat zostanie wysłany do niezalogowanego czata)
                    if(msg_irc.size() != 0 && command_ok)
                    {
                        socket_irc_send(socketfd_irc, irc_ok, msg_irc, data_sent);
//                        display_buffer(win_diag, use_colors, UCC_BLUE, data_sent);      // tymczasowo pokaż, co program wysyła na serwer
                    }


                    // sprawdź gotowość do połączenia z IRC
                    if(irc_ready)
                    {
//                        irc_ready = false;      // nie próbuj ponownie się łączyć do IRC od zera, bo na tym etapie to nie powinno mieć miejsca
                        socket_irc_connect(socketfd_irc, www, msg, msg_color, buffer_irc_recv);     // połącz z IRC
                        // gdy podczas łączenia otrzymano jakiś komunikat, pokaż go
                        if(msg.size() != 0)
                        {
                            wattrset_color(win_diag, use_colors, msg_color);
                            mvwprintw(win_diag, cur_y, cur_x, "%s\n", msg.c_str());
                        }
                        // gdy msg jest zerowy, uznaje się, że trzeba odczytać socket IRC (nie ma błędu w połączeniu)
                        else
                        {
                            // pobierz pierwszą odpowiedż serwera po połączeniu
                            socket_irc_recv(socketfd_irc, irc_ok, buffer_irc);
                            display_buffer(win_diag, use_colors, UCC_WHITE, buffer_irc);
                            // wyślij: NICK <~nick>
                            socket_irc_send(socketfd_irc, irc_ok, "NICK " + zuousername, data_sent);
                            display_buffer(win_diag, use_colors, UCC_YELLOW, data_sent);
                            // pobierz odpowiedź z serwera
                            socket_irc_recv(socketfd_irc, irc_ok, buffer_irc);
                            display_buffer(win_diag, use_colors, UCC_WHITE, buffer_irc);
                            // wyślij: AUTHKEY
                            socket_irc_send(socketfd_irc, irc_ok, "AUTHKEY", data_sent);
                            display_buffer(win_diag, use_colors, UCC_YELLOW, data_sent);
                            // pobierz odpowiedź z serwera (AUTHKEY)
                            socket_irc_recv(socketfd_irc, irc_ok, buffer_irc);
                            display_buffer(win_diag, use_colors, UCC_WHITE, buffer_irc);
                            // wyszukaj AUTHKEY
                            find_value(strdup(buffer_irc.c_str()), "801 " + zuousername + " :", "\r\n", authkey);  // dodać if
                            // konwersja AUTHKEY
                            auth_code(authkey);     // dodać if
                            // wyślij: AUTHKEY <AUTHKEY>
                            socket_irc_send(socketfd_irc, irc_ok, "AUTHKEY " + authkey, data_sent);
                            display_buffer(win_diag, use_colors, UCC_YELLOW, data_sent);
                            // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>
                            socket_irc_send(socketfd_irc, irc_ok, "USER * " + uokey + " czat-app.onet.pl :" + zuousername, data_sent);
                            display_buffer(win_diag, use_colors, UCC_YELLOW, data_sent);
                            // od tej pory można dodać socketfd_irc do zestawu select()
                            irc_ok = true;
                        }
                        // po połączeniu do IRC dodaj do zestawu select() wartość socketfd_irc
//                        if(irc_ok)
//                        {
//                        }
                    }

                    // zachowaj pozycję kursora dla kolejnego komunikatu
                    getyx(win_diag, cur_y, cur_x);
                    // po obsłudze bufora wyczyść go
                    kbd_buf.clear();
                    kbd_buf_pos = 0;
                    kbd_buf_max = 0;

                }   // if(kbd_buf.size() != 0)

            }   // else if(key_code == '\n')

            // kody ASCII wczytaj do bufora (te z zakresu 32...255)
            else if(key_code >= 32 && key_code <= 255)
            {
                if(key_code != '\r')            // ignoruj kod '\r'
                {
                    if(kbd_buf_max < 256)       // ogranicz pojemność bufora wejściowego
                    {
                        kbd_utf2iso(key_code);  // gdy to UTF-8, to zamień na ISO-8859-2
                        key_code_tmp = key_code;
                        kbd_buf.insert(kbd_buf_pos, key_code_tmp);
                        ++kbd_buf_pos;
                        ++kbd_buf_max;
                    }
                }
            }

                ++ix;

        }   // if(FD_ISSET(0, &readfds_tmp))

        // gniazdo (socket)
        if(FD_ISSET(socketfd_irc, &readfds_tmp))
        {
            // pobierz odpowiedź z serwera
            socket_irc_recv(socketfd_irc, irc_ok, buffer_irc);

            // zinterpretuj odpowiedź
            irc_parser(buffer_irc, msg, msg_color, room, send_irc, irc_ok);

            if(send_irc)
            {
                socket_irc_send(socketfd_irc, irc_ok, msg, data_sent);  // dotychczas wysyłaną odpowiedzią w tym miejscu jest PONG
            }

            else
            {
                if(msg.size() != 0)
                {
                    wattrset_color(win_diag, use_colors, msg_color);
                    mvwprintw(win_diag, cur_y, cur_x, "%s\n", msg.c_str());
                }
                else
                {
                    display_buffer(win_diag, use_colors, UCC_WHITE, buffer_irc);
                }
            }

            // zachowaj pozycję kursora dla kolejnego komunikatu
            getyx(win_diag, cur_y, cur_x);

            // gdy serwer zakończy połączenie, usuń socketfd_irc z zestawu select() oraz ustaw z powrotem nick w pasku wpisywania na Niezalogowany
            if(! irc_ok)
            {
                FD_CLR(socketfd_irc, &readfds);
                close(socketfd_irc);
                zuousername = "Niezalogowany";
            }

                ++iy;

        }   // if(FD_ISSET(socketfd_irc, &readfds_tmp))

    }   // while(! ucc_quit)

//    if(socketfd_irc > 0)
//        close(socketfd_irc);

    delwin(win_diag);
    endwin();           // zakończ tryb ncurses

    return 0;
}


bool check_colors()
{
    if(has_colors() == FALSE)       // gdy nie da się używać kolorów, nie kończ programu, po prostu używaj czarno-białego terminala
        return false;

    if(start_color() == ERR)
        return false;

    short font_color = COLOR_WHITE;
    short background_color = COLOR_BLACK;

    if(use_default_colors() == OK)       // jeśli się da, dopasuj kolory do ustawień terminala
    {
        font_color = -1;
        background_color = -1;
    }

    init_pair(1, COLOR_RED, background_color);
    init_pair(2, COLOR_GREEN, background_color);
    init_pair(3, COLOR_YELLOW, background_color);
    init_pair(4, COLOR_BLUE, background_color);
    init_pair(5, COLOR_MAGENTA, background_color);
    init_pair(6, COLOR_CYAN, background_color);
    init_pair(7, COLOR_WHITE, background_color);
    init_pair(8, font_color, background_color);
    init_pair(9, COLOR_BLUE, COLOR_WHITE);

    return true;
}


void wattrset_color(WINDOW *active_window, bool use_colors, short color_p)
{
    if(use_colors)
        wattrset(active_window, COLOR_PAIR(color_p));    // wattrset() nadpisuje atrybuty, wattron() dodaje atrybuty do istniejących
    else
        wattrset(active_window, A_NORMAL);
}


void display_buffer(WINDOW *active_window, bool use_colors, short color_p, std::string &buffer_out)
{
    wattrset_color(active_window, use_colors, color_p);

    for(int i = 0; i < (int)buffer_out.size(); ++i)
    {
        if(buffer_out[i] != '\r')
            wprintw(active_window, "%c", buffer_out[i]);
    }

    wrefresh(active_window);
}


/*
    Celowo poniżej nie używam iconv() do konwersji, bo zamieniany jest tylko jeden znak, iconv() używany jest natomiast w socket_irc_recv(), bo konwertuje
    cały pobrany bufor na raz. Konwersja w tym miejscu jest wymagana, bo upraszcza zliczanie wprowadzonych z klawiatury znaków (w UTF-8 polskie znaki zajmują 2 bajty).
*/


void kbd_utf2iso(int &key_code)
{
    // zamień polski znak (jeden) w UTF-8 na ISO-8859-2, jest to wersja bardzo uproszczona i nie działa na innych znakach w UTF-8 poza polskimi znakami

    if(key_code == 0xC3)
    {
        // pobierz resztę kodu znaku
        key_code = getch();

        switch(key_code)
        {
        case 0x93:              // Ó (w kodzie UTF-8 zapisywana jako 0xC393)
            key_code = 0xD3;    // zamień na ISO-8859-2
            break;

        case 0xB3:              // ó
            key_code = 0xF3;
            break;
        }
    }

    else if(key_code == 0xC4)
    {
        // pobierz resztę kodu znaku
        key_code = getch();

        switch(key_code)
        {
        case 0x84:              // Ą
            key_code = 0xA1;
            break;

        case 0x86:              // Ć
            key_code = 0xC6;
            break;

        case 0x98:              // Ę
            key_code = 0xCA;
            break;

        case 0x85:              // ą
            key_code = 0xB1;
            break;

        case 0x87:              // ć
            key_code = 0xE6;
            break;

        case 0x99:              // ę
            key_code = 0xEA;
            break;
        }
    }

    else if(key_code == 0xC5)
    {
        // pobierz resztę kodu znaku
        key_code = getch();

        switch(key_code)
        {
        case 0x81:              // Ł
            key_code = 0xA3;
            break;

        case 0x83:              // Ń
            key_code = 0xD1;
            break;

        case 0x9A:              // Ś
            key_code = 0xA6;
            break;

        case 0xB9:              // Ź
            key_code = 0xAC;
            break;

        case 0xBB:              // Ż
            key_code = 0xAF;
            break;

        case 0x82:              // ł
            key_code = 0xB3;
            break;

        case 0x84:              // ń
            key_code = 0xF1;
            break;

        case 0x9B:              // ś
            key_code = 0xB6;
            break;

        case 0xBA:              // ź
            key_code = 0xBC;
            break;

        case 0xBC:              // ż
            key_code = 0xBF;
            break;
        }
    }
}


void wprintw_iso2utf(WINDOW *active_window, bool use_colors, short color_p, int cur_y, int cur_x, std::string buffer_str)
{
    unsigned char c;                // aktualnie przetwarzany znak z bufora

    wmove(active_window, cur_y, cur_x);     // przenieś kursor
    wattrset_color(active_window, use_colors, color_p);

    // wypisywanie w pętli
    for(int i = 0; i < (int)buffer_str.size(); ++i)
    {
        // pobierz znak z bufora
        c = buffer_str[i];

        // najpierw wykryj polskie znaki
        switch(c)
        {
        case 0xB1:          // ą
            wprintw(active_window, "%c%c", 0xC4, 0x85);    // zamień na UTF-8
            break;

        case 0xE6:          // ć
            wprintw(active_window, "%c%c", 0xC4, 0x87);
            break;

        case 0xEA:          // ę
            wprintw(active_window, "%c%c", 0xC4, 0x99);
            break;

        case 0xB3:          // ł
            wprintw(active_window, "%c%c", 0xC5, 0x82);
            break;

        case 0xF1:          // ń
            wprintw(active_window, "%c%c", 0xC5, 0x84);
            break;

        case 0xF3:          // ó
            wprintw(active_window, "%c%c", 0xC3, 0xB3);
            break;

        case 0xB6:          // ś
            wprintw(active_window, "%c%c", 0xC5, 0x9B);
            break;

        case 0xBC:          // ź
            wprintw(active_window, "%c%c", 0xC5, 0xBA);
            break;

        case 0xBF:          // ż
            wprintw(active_window, "%c%c", 0xC5, 0xBC);
            break;

        case 0xA1:          // Ą
            wprintw(active_window, "%c%c", 0xC4, 0x84);
            break;

        case 0xC6:          // Ć
            wprintw(active_window, "%c%c", 0xC4, 0x86);
            break;

        case 0xCA:          // Ę
            wprintw(active_window, "%c%c", 0xC4, 0x98);
            break;

        case 0xA3:          // Ł
            wprintw(active_window, "%c%c", 0xC5, 0x81);
            break;

        case 0xD1:          // Ń
            wprintw(active_window, "%c%c", 0xC5, 0x83);
            break;

        case 0xD3:          // Ó
            wprintw(active_window, "%c%c", 0xC3, 0x93);
            break;

        case 0xA6:          // Ś
            wprintw(active_window, "%c%c", 0xC5, 0x9A);
            break;

        case 0xAC:          // Ź
            wprintw(active_window, "%c%c", 0xC5, 0xB9);
            break;

        case 0xAF:          // Ż
            wprintw(active_window, "%c%c", 0xC5, 0xBB);
            break;

        default:
            wprintw(active_window, "%c", c);   // gdy to nie był polski znak, wpisz odczytaną wartość bez modyfikacji
            break;
        }
    }

    // odświeżenie ekranu nastąpi poza funkcją
}
