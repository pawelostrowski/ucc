#include <cstring>          // memset()
#include <string>           // std::string, setlocale()
#include <cerrno>           // errno
#include <sys/select.h>     // select()
#include <netdb.h>          // getaddrinfo(), freeaddrinfo(), socket()
#include <unistd.h>         // close() - socket
#include "main_window.hpp"
#include "ucc_colors.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "socket_irc.hpp"


int main_window(bool use_colors)
{
    freopen("/dev/tty", "r", stdin);    // zapobiega zapętleniu się programu po wpisaniu w terminalu echo text | ucc

    setlocale(LC_ALL, "");  // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    // inicjalizacja ncurses
    if(! initscr())
        return 1;

    bool ucc_quit = false;  // aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
    bool captcha_ok = false;    // stan wczytania captcha (jego pobranie z serwera)
    bool irc_ok = false;    // stan połączenia z czatem
    int term_y, term_x;     // wymiary terminala
    int cur_y, cur_x;       // aktualna pozycja kursora
    int kbd_buf_pos = 0;    // początkowa pozycja bufora klawiatury (istotne podczas używania strzałek oraz Home i End)
    int kbd_buf_max = 0;    // początkowy maksymalny rozmiar bufora klawiatury
    int key_code;           // kod ostatnio wciśniętego klawisza
    int socketfd_irc;       // gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() )
    std::string kbd_buf;    // bufor odczytanych znaków z klawiatury
    std::string key_code_tmp;   // tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)
    std::string msg;        // komunikat do wyświetlenia z kbd_parser() (opcjonalny)
    short msg_color;        // kolor komunikatu z zainicjalizowanej pary kolorów (można posługiwać się prefiksem UCC_)
    std::string cookies, nick, zuousername, room;
//    char buffer_recv[1500];

    // inicjalizacja gniazda (socket) używanego w połączeniu IRC
    socket_irc_init(socketfd_irc);

    fd_set readfds;                     // deskryptor dla select()
    fd_set readfds_tmp;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);                // klawiatura (stdin)

    raw();                  // zablokuj Ctrl-C i Ctrl-Z
    keypad(stdscr, TRUE);   // klawisze funkcyjne będą obsługiwane
    noecho();               // nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)

    // sprawdź, czy terminal obsługuje kolory, jeśli tak, włącz kolory oraz zainicjalizuj podstawową parę kolorów,
    // ale tylko, gdy uruchomiliśmy main_window() z use_colors = true, gdy terminal nie obsługuje kolorów, check_colors() zwróci false
    if(use_colors)
        use_colors = check_colors();

    // utwórz okno, w którym będą komunikaty serwera oraz inne (np. diagnostyczne)
    getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala (okna głównego)
    WINDOW *win_diag;
    win_diag = newwin(term_y - 3, term_x, 1, 0);
    scrollok(win_diag, TRUE);       // włącz przewijanie w tym oknie

    // jeśli terminal obsługuje kolory, poniższy komunikat powitalny wyświetl w kolorze zielonym
    wattrset_color(win_diag, use_colors, UCC_GREEN);
    mvwprintw(win_diag, 0, 0, "Ucieszony Chat Client\n"
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
    int is = 0, ix = 0, iy = 0;
/*
    Koniec wskaźników pomocniczych
*/

    while(! ucc_quit)
    {
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
        wmove(stdscr, 0, 0);
        wprintw(stdscr, "socketfd_irc: %d", socketfd_irc);

        wmove(stdscr, term_y - 2, 0);
        wprintw(stdscr, "is: %d, ix: %d, iy: %d", is, ix, iy);
/*
    Koniec informacji tymczasowych
*/

        // wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
        wattrset(stdscr, A_NORMAL);     // tekst wypisywany z bufora ze zwykłymi atrybutami
        wmove(stdscr, term_y - 1, 0);   // przenieś kursor do początku ostatniego wiersza
        wprintw(stdscr, ">%s", kbd_buf.c_str());    // wypisz > oraz zawartość bufora
        wclrtoeol(stdscr);              // pozostałe znaki w wierszu wykasuj
        wmove(stdscr, term_y - 1, kbd_buf_pos + 1); // ustaw kursor za wpisywanym tekstem (+ 1, bo na początku jest znak > )

        wrefresh(win_diag);
        wrefresh(stdscr);

        // czekaj na aktywność klawiatury lub gniazda (socket)
        if(select(socketfd_irc + 1, &readfds_tmp, NULL, NULL, NULL) == -1)
        {
            // sygnał SIGWINCH (zmiana rozmiaru okna terminala) powoduje, że select() zwraca -1, więc trzeba to wykryć, aby nie wywalić programu w kosmos
            if(errno == EINTR)  // Interrupted system call (wywołany np. przez SIGWINCH)
            {
                wrefresh(win_diag);
                wrefresh(stdscr);   // odświeżenie w tym miejscu jest wymagane, gdy zmienimy rozmiar terminala
                getch();        // ignoruj KEY_RESIZE
                continue;       // wróć do początku pętli while()
            }
            // inny błąd select() powoduje zakończenie działania programu
            else
            {
                delwin(win_diag);
                endwin();       // zakończ tryb ncurses
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
                    // ustaw kursor w miejscu, gdzie był po ostatnim wypisaniu tekstu
                    wmove(win_diag, cur_y, cur_x);
                    // wykonaj obsługę bufora (zidentyfikuj polecenie lub wyślij tekst do aktywnego pokoju)
                    kbd_parser(kbd_buf, msg, msg_color, cookies, nick, zuousername, captcha_ok, irc_ok, socketfd_irc, ucc_quit);
                    // gdy kbd_parser() zwrócił jakąś wiadomość, pokaż ją
                    if(msg.size() != 0)
                    {
                        wattrset_color(win_diag, use_colors, msg_color);
                        mvwprintw(win_diag, cur_y, cur_x, "%s\n", msg.c_str());
                    }
                    // zachowaj pozycję kursora dla kolejnego komunikatu
                    getyx(win_diag, cur_y, cur_x);
                    // po obsłudze bufora wyczyść go
                    kbd_buf.clear();
                    kbd_buf_pos = 0;
                    kbd_buf_max = 0;
                    // po połączeniu do IRC dodaj do zestawu select() wartość socketfd_irc
                    if(irc_ok)
                        FD_SET(socketfd_irc, &readfds);     // gniazdo IRC (socket)
                }
            }

            else if(key_code >= 32 && key_code <= 255)   // do bufora odczytanych znaków wpisuj tylko te z zakresu 32...255
            {
                if(kbd_buf_max < 256)       // ogranicz pojemność bufora wejściowego
                {
                    key_code_tmp = key_code;
                    kbd_buf.insert(kbd_buf_pos, key_code_tmp);
                    ++kbd_buf_pos;
                    ++kbd_buf_max;
                }
            }

                ++ix;
        }

        // gniazdo (socket)
        if(FD_ISSET(socketfd_irc, &readfds_tmp))
        {

//            irc_parser(buffer_recv, socketfd_irc, win_diag);
            getyx(win_diag, cur_y, cur_x);

            wrefresh(win_diag);
            wrefresh(stdscr);

                ++iy;
        }

                ++is;
    }

//        FD_CLR(socketfd_irc, &readfds);                     // PRZYJRZEĆ SIĘ TEMU ROZWIĄZANIU!!!

//    if(socketfd_irc > 0)
//        close(socketfd_irc);

    delwin(win_diag);
    endwin();       // zakończ tryb ncurses

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


void wattrset_color(WINDOW *active_window, bool use_color, short color_p)
{
    if(use_color)
        wattrset(active_window, COLOR_PAIR(color_p));    // wattrset() nadpisuje atrybuty, wattron() dodaje atrybuty do istniejących
    else
        wattrset(active_window, A_NORMAL);
}


void show_buffer_1(WINDOW *active_window, std::string &data_buf)
{
    int data_buf_len = data_buf.size();

    for(int i = 0; i < data_buf_len; ++i)
    {
        if(data_buf[i] != '\r')
            wprintw(active_window, "%c", data_buf[i]);
    }
}


void show_buffer_2(WINDOW *active_window, char *data_buf)
{
    int data_buf_len = strlen(data_buf);

    for(int i = 0; i < data_buf_len; ++i)
    {
        if(data_buf[i] != '\r')
            wprintw(active_window, "%c", data_buf[i]);
    }
}
