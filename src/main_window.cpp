//#include <cstring>      // strlen()
#include <string>       // std::string, setlocale()
#include <sys/select.h> // select()
#include "ncursesw/ncurses.h"   // wersja ncurses ze wsparciem dla UTF-8
#include "main_window.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "sockets.hpp"


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
    init_pair(7, font_color, background_color);

    return true;
}


int main_window()
{
    setlocale(LC_ALL, "");  // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    if(! initscr())         // inicjalizacja ncurses
        return 1;

    bool use_colors;
    bool ucc_quit = false;  // aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
    bool captcha_ok = false;    // stan wczytania captcha (jego pobranie z serwera)
    bool irc_ok = false;    // stan połączenia z czatem
    int term_y, term_x;     // wymiary terminala
    int cur_y, cur_x;       // aktualna pozycja kursora
    int kbd_buf_pos = 0;    // początkowa pozycja bufora klawiatury (istotne podczas używania strzałek oraz Home i End)
    int kbd_buf_max = 0;    // początkowy maksymalny rozmiar bufora klawiatury
    int key_code;           // kod ostatnio wciśniętego klawisza
    int socketfd_irc = 3;   // gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() )
    std::string kbd_buf;    // bufor odczytanych znaków z klawiatury
    std::string key_code_tmp;   // tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)
    std::string cookies, nick, zuousername, room;
    char buffer_recv[1500];

    fd_set readfds;         // deskryptor dla select()
    fd_set readfds_tmp;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);    // klawiatura
    FD_SET(socketfd_irc, &readfds); // gniazdo (socket)


    raw();                  // zablokuj Ctrl-C i Ctrl-Z
    keypad(stdscr, TRUE);   // klawisze funkcyjne będą obsługiwane
    noecho();               // nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)

    use_colors = check_colors();    // sprawdź, czy terminal obsługuje kolory, jeśli tak, włącz kolory oraz zainicjalizuj podstawową parę kolorów

    // utwórz okno, w którym będą komunikaty serwera oraz inne (np. diagnostyczne)
    getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala (okna głównego)
    WINDOW *win_diag;
    win_diag = newwin(term_y - 3, term_x, 1, 0);
    scrollok(win_diag, TRUE);       // włącz przewijanie w tym oknie

    // jeśli terminal obsługuje kolory, poniższy komunikat powitalny wyświetl w kolorze zielonym
    if(use_colors)
        wattrset(win_diag, COLOR_PAIR(2));    // attrset() nadpisuje atrybuty, attron() dodaje atrybuty do istniejących
    else
        wattrset(win_diag, A_NORMAL);
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

    getyx(win_diag, cur_y, cur_x);

            int ix = 0;
    do
    {
        readfds_tmp = readfds;

        getmaxyx(stdscr, term_y, term_x); // pobierz wymiary terminala (okna głównego)
        wresize(stdscr, term_y, term_x);     // zmień wymiary okna głównego, gdy terminal zmieni romiar
        wresize(win_diag, term_y - 3, term_x);  // j/w, ale dla okna diagnostycznego

        // tymczasowo wykasuj 3 od końca wiersz
//        wmove(stdscr, term_y - 3, 0);
//        wclrtoeol(stdscr);

        // paski (jeśli terminal obsługuje kolory, paski będą niebieskie)
        if(use_colors)
            wattrset(stdscr, COLOR_PAIR(4) | A_REVERSE);
        else
            wattrset(stdscr, A_REVERSE);
        wmove(stdscr, 0, 0);
        for(int i = 0; i < term_x; i++)
            wprintw(stdscr, " ");
        wmove(stdscr, term_y - 2, 0);
        for(int i = 0; i < term_x; i++)
            wprintw(stdscr, " ");


                wattron(stdscr, COLOR_PAIR(7));
                wmove(stdscr, 0, 0);
                wprintw(stdscr, "socketfd_irc: %d", socketfd_irc);

                wmove(stdscr, term_y - 2, 0);
                wprintw(stdscr, "ix: %d", ix);


        // wypisz bufor w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
        wattrset(stdscr, A_NORMAL);      // tekst wypisywany z bufora ze zwykłymi atrybutami
        wmove(stdscr, term_y - 1, 0);    // przenieś kursor do ostatniego wiersza
        wprintw(stdscr, ">%s", kbd_buf.c_str());     // wypisz > oraz zawartość bufora
        wclrtoeol(stdscr);             // pozostałe znaki w wierszu wykasuj
        wmove(stdscr, term_y - 1, kbd_buf_pos + 1);      // + 1, bo na początku jest znak >

        wrefresh(win_diag);
        wrefresh(stdscr);

        select(socketfd_irc + 1, &readfds_tmp, NULL, NULL, NULL);   // czekaj na aktywność klawiatury lub gniazda (socket)

        if(FD_ISSET(0, &readfds_tmp))
        {
            wrefresh(win_diag);
            wrefresh(stdscr);      // odświeżenie w tym miejscu jest wymagane, gdy zmieniamy wymiary terminala

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
                    // dodaj kod nowej linii na końcu bufora
                    kbd_buf.insert(kbd_buf_max, "\n");
                    // ustaw kursor w miejscu, gdzie był po ostatnim wypisaniu tekstu
                    wmove(win_diag, cur_y, cur_x);
                    // wykonaj obsługę bufora (zidentyfikuj polecenie lub wyślij tekst do aktywnego pokoju)
                    wattrset(win_diag, A_NORMAL);
                    kbd_parser(win_diag, use_colors, socketfd_irc, readfds, kbd_buf, cookies, nick, zuousername, room, captcha_ok, irc_ok, ucc_quit);
                    getyx(win_diag, cur_y, cur_x);
                    // po obsłudze bufora wyczyść go
                    kbd_buf.clear();
                    kbd_buf_pos = 0;
                    kbd_buf_max = 0;
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

        }

        if(FD_ISSET(socketfd_irc, &readfds_tmp))
        {
                ++ix;

            irc_parser(buffer_recv, socketfd_irc, win_diag);
            getyx(win_diag, cur_y, cur_x);

            wrefresh(win_diag);
            wrefresh(stdscr);
        }

    } while(! ucc_quit);

    if(socketfd_irc)
        close(socketfd_irc);

    delwin(win_diag);
    endwin();       // zakończ tryb ncurses

    return 0;
}
