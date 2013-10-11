#include <cstring>          // memset()
#include <string>           // std::string, setlocale()
#include <ctime>            // czas
#include <cerrno>           // errno
#include <iconv.h>          // konwersja kodowania znaków
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

    setlocale(LC_ALL, "");      // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    // inicjalizacja ncurses
    if(! initscr())
        return 1;

    bool ucc_quit = false;      // aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
    bool command_ok = false;    // true, gdy wpisano polecenie
    bool captcha_ready = false; // stan wczytania captcha (jego pobranie z serwera)
    bool irc_ready = false;     // gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
    bool irc_auth_status;       // status wykonania którejś z funkcji irc_auth_x()
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
    std::string cookies, my_nick, my_password, uokey, room, msg_sock;
    int socketfd_irc = 0;       // gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne
    std::string buffer_irc_recv;    // bufor odebranych danych z IRC
    std::string buffer_irc_recv_tmp;    // bufor pomocniczy do zachowania fragmentu ostatniego wiersza, jeśli nie został pobrany w całości w jednej ramce
    std::string buffer_irc_sent;    // dane wysłane do serwera w irc_auth_x() (informacje przydatne do debugowania)

    struct sockaddr_in irc_info;

    fd_set readfds;         // deskryptor dla select()
    fd_set readfds_tmp;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);    // klawiatura (stdin)

    raw();                  // zablokuj Ctrl-C i Ctrl-Z
    keypad(stdscr, TRUE);   // klawisze funkcyjne będą obsługiwane
    noecho();               // nie pokazuj wprowadzanych danych (bo w tym celu będzie używany bufor)
    nodelay(stdscr, TRUE);  // nie blokuj getch() (zwróć ERR, gdy nie ma żadnego znaku do odczytania)

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
    wprintw_iso2utf(win_diag, use_colors, UCC_GREEN, "Ucieszony Chat Client"
                                                     "\n# Aby się zalogować na nick tymczasowy, wpisz:"
                                                     "\n/nick nazwa_nicka"
                                                     "\n/connect"
                                                     "\n# Następnie przepisz kod z obrazka, w tym celu wpisz:"
                                                     "\n/captcha kod_z_obrazka"
                                                     "\n# Aby się zalogować na nick stały (zarejestrowany), wpisz:"
                                                     "\n/nick nazwa_nicka hasło_do_nicka"
                                                     "\n/connect"
                                                     "\n# Aby zobaczyć dostępne polecenia, wpisz:"
                                                     "\n/help"
                                                     "\n# Aby zakończyć działanie programu, wpisz:"
                                                     "\n/quit lub /q");

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

    // pętla główna programu
    while(! ucc_quit)
    {
        readfds_tmp = readfds;

        // wykryj zmianę rozmiaru okna terminala
        if(is_term_resized(term_y, term_x))
        {
            getmaxyx(stdscr, term_y, term_x);       // pobierz nowe wymiary terminala (okna głównego) po jego zmianie
            wresize(stdscr, term_y, term_x);        // zmień rozmiar okna głównego po zmianie rozmiaru okna terminala
            wresize(win_diag, term_y - 3, term_x);  // j/w, ale dla okna diagnostycznego
            // po zmianie rozmiaru terminala sprawdź, czy maksymalna pozycja kursora Y nie jest większa od wymiarów okna
            if(cur_y >= term_y - 3)
                cur_y = term_y - 4;     // - 4, bo piszemy do max granicy, nie wchodząc na nią
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

        wmove(stdscr, 0, 0);
        wprintw(stdscr, "socketfd_irc: %d", socketfd_irc);
/*
    Koniec informacji tymczasowych
*/

        // wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
        //  oraz ustaw kursor w obecnie przetwarzany znak
        kbd_buf_show(kbd_buf, zuousername, term_y, kbd_buf_pos);

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
                if(kbd_buf.size() != 0)             // wykonaj obsługę bufora tylko, gdy coś w nim jest
                {
                    // "wyczyść" pole wpisywanego tekstu (aby nie było widać zwłoki, np. podczas pobierania obrazka z kodem do przepisania)
                    wmove(stdscr, term_y - 1, zuousername.size() + 3);      // ustaw kursor za nickiem i spacją za nawiasem
                    clrtoeol();
                    wrefresh(stdscr);
                    // wykonaj obsługę bufora (zidentyfikuj polecenie)
                    kbd_parser(kbd_buf, msg, msg_color, msg_irc, my_nick, my_password, zuousername, cookies,
                               uokey, command_ok, captcha_ready, irc_ready, irc_ok, room, room_ok, command_me, ucc_quit);
                    // jeśli wpisano zwykły tekst (nie polecenie), pokaż go wraz z nickiem i wyślij polecenie do IRC (wykrycie, czy połączono się z IRC oraz czy otwarty
                    //  jest aktywny pokój jest wykonywane w kbd_parser(), przy błędzie nie jest ustawiany command_ok, aby pokazać komunikat poniżej)
                    if(! command_ok)
                    {
                        // pokaż komunikat z uwzględnieniem tego, że w buforze jest kodowanie ISO-8859-2
                        wprintw_iso2utf(win_diag, use_colors, UCC_MAGENTA, msg);
                        // wyślij wiadomość na serwer
                        if(! socket_irc_send(socketfd_irc, irc_ok, msg_irc, msg_sock))
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg_sock);       // w przypadku błędu pokaż, co się stało
                        }
                    }
                    // gdy kbd_parser() zwrócił jakiś komunikat przeznaczony do wyświetlenia na terminalu, pokaż go
                    else if(msg.size() != 0)
                    {
                        wprintw_iso2utf(win_diag, use_colors, msg_color, msg);
                    }
                    // gdy kbd_parser() zwrócił jakiś komunikat przeznaczony do wysłania do IRC i nie jest to zwykły tekst wypisany w wprintw_iso2utf(), wyślij go do IRC
                    //  (stan połączenia do IRC wykrywany jest w kbd_parser(), więc nie trzeba się obawiać, że komunikat zostanie wysłany do niezalogowanego czata)
                    if(msg_irc.size() != 0 && command_ok)
                    {
                        wprintw_iso2utf(win_diag, use_colors, UCC_BLUE, msg_irc);           // tymczasowo pokaż, co program wysyła na serwer
                        if(! socket_irc_send(socketfd_irc, irc_ok, msg_irc, msg_sock))
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg_sock);       // w przypadku błędu pokaż, co się stało
                        }
                    }
                    // sprawdź gotowość do połączenia z IRC
                    if(irc_ready)
                    {
                        irc_ready = false;      // po połączeniu nie próbuj się znowu łączyć do IRC od zera
                        // inicjalizacja gniazda (socket) używanego w połączeniu IRC
                        if(socket_irc_init(socketfd_irc, irc_info) != 0)
                        {
                            delwin(win_diag);
                            endwin();
                            return 3;
                        }
                        // połącz z serwerem IRC
                        irc_auth_status = irc_auth_1(socketfd_irc, irc_ok, buffer_irc_recv, irc_info, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);      // pokaż odpowiedź serwera
                        if(! irc_auth_status)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg);    // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: NICK <~nick>
                        irc_auth_status = irc_auth_2(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, zuousername, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);      // pokaż odpowiedź serwera
                        if(! irc_auth_status)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg);    // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: AUTHKEY
                        irc_auth_status = irc_auth_3(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);      // pokaż odpowiedź serwera
                        if(! irc_auth_status)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg);    // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: AUTHKEY <AUTHKEY>
                        irc_auth_status = irc_auth_4(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        if(! irc_auth_status)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg);    // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>
                        irc_auth_status = irc_auth_5(socketfd_irc, irc_ok, buffer_irc_sent, zuousername, uokey, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        if(! irc_auth_status)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg);    // w przypadku błędu pokaż, co się stało
                        }
                        // od tej pory, o ile poprawnie połączono się do IRC, można dodać socketfd_irc do zestawu select()
                        if(irc_ok)
                        {
                            FD_SET(socketfd_irc, &readfds);  // gniazdo IRC (socket)
                        }
                        // gdy połączenie do IRC nie powiedzie się, wyzeruj socket i ustaw z powrotem nick w pasku wpisywania na Niezalogowany
                        else
                        {
                            socketfd_irc = 0;
                            zuousername = "Niezalogowany";
                        }

                    }   // if(irc_ready)

                    // zachowaj pozycję kursora dla kolejnego komunikatu
                    getyx(win_diag, cur_y, cur_x);
                    // po obsłudze bufora wyczyść go
                    kbd_buf.clear();
                    kbd_buf_pos = 0;
                    kbd_buf_max = 0;

                }   // if(kbd_buf.size() != 0)

            }   // else if(key_code == '\n')

            // kody ASCII (oraz rozszerzone) wczytaj do bufora (te z zakresu 32...255)
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
        else if(FD_ISSET(socketfd_irc, &readfds_tmp))
        {
            // pobierz odpowiedź z serwera
            if(! socket_irc_recv(socketfd_irc, irc_ok, buffer_irc_recv, msg_sock))
            {
                wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg_sock);       // w przypadku błędu pokaż, co się stało
            }

            // zinterpretuj odpowiedź
            irc_parser(buffer_irc_recv, buffer_irc_recv_tmp, msg, msg_color, room, send_irc, irc_ok);

            if(send_irc)
            {
                if(! socket_irc_send(socketfd_irc, irc_ok, msg, msg_sock))      // dotychczas wysyłaną odpowiedzią w tym miejscu jest PONG
                {
                    wprintw_iso2utf(win_diag, use_colors, UCC_RED, msg_sock);   // w przypadku błędu pokaż, co się stało
                }
            }

            else
            {
                if(msg.size() != 0)
                {
                    wprintw_iso2utf(win_diag, use_colors, msg_color, msg);
                }
                else
                {
                    wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);
                }
            }

            // zachowaj pozycję kursora dla kolejnego komunikatu
            getyx(win_diag, cur_y, cur_x);

            // gdy serwer zakończy połączenie, usuń socketfd_irc z zestawu select(), wyzeruj socket oraz ustaw z powrotem nick w pasku wpisywania na Niezalogowany
            if(! irc_ok)
            {
                FD_CLR(socketfd_irc, &readfds);
                close(socketfd_irc);
                socketfd_irc = 0;
                zuousername = "Niezalogowany";
            }

                ++iy;

        }   // else if(FD_ISSET(socketfd_irc, &readfds_tmp))

    }   // while(! ucc_quit)

    // jeśli podczas zamykania programu gniazdo IRC (socket) jest nadal otwarte (co nie powinno się zdarzyć), zamknij je
    if(socketfd_irc > 0)
        close(socketfd_irc);

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


void get_time(char *time_hms)
{
    // funkcja zwraca w *time_hms lokalny czas w postaci [HH:MM:SS] (ze spacją na końcu)

    time_t time_g;      // czas skoordynowany z Greenwich
    struct tm *time_l;  // czas lokalny

    time(&time_g);
    time_l = localtime(&time_g);
    strftime(time_hms, 20, "[%H:%M:%S] ", time_l);
}


void kbd_utf2iso(int &key_code)
{
    // zamień znak (jeden) w UTF-8 na ISO-8859-2
    // UWAGA - funkcja nie działa dla znaków więcej, niż 2-bajtowych oraz nie wykrywa nieprawidłowo wprowadzonych znaków!

    int det_utf;

    det_utf = key_code & 0xE0;      // iloczyn bitowy 11100000b do wykrycia 0xC0, oznaczającego znak w UTF-8

    if(det_utf != 0xC0)     // wykrycie 0xC0 oznacza, że mamy znak UTF-8 dwubajtowy
        return;             // jeśli to nie UTF-8, wróć bez zmian we wprowadzonym kodzie

    char c_in[5];
    char c_out[5];

    // wpisz bajt wejściowy oraz drugi bajt znaku
    c_in[0] = (char)key_code;
    c_in[1] = (char)getch();
    c_in[2] = '\0';     // NULL na końcu

    // dokonaj konwersji znaku (dwubajtowego) z UTF-8 na ISO-8859-2
    char *c_in_ptr = c_in;
    size_t c_in_len = strlen(c_in) + 1;
    char *c_out_ptr = c_out;
    size_t c_out_len = sizeof(c_out);

    iconv_t cd = iconv_open("ISO-8859-2", "UTF-8");
    iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
    iconv_close(cd);

    // po konwersji zakłada się, że znak w ISO-8859-2 ma jeden bajt (brak sprawdzania poprawności wprowadzanych znaków), zwróć ten znak
    key_code = c_out[0];
}


void kbd_buf_show(std::string &kbd_buf, std::string &zuousername, int term_y, int kbd_buf_pos)
{
    // konwersja nicka oraz zawartości bufora klawiatury z ISO-8859-2 na UTF-8
    char c_in[kbd_buf.size() + zuousername.size() + 1 + 3];         // bufor + nick (+ 1 na NULL, + 3, bo nick objęty jest nawiasem oraz spacją za nawiasem)
    char c_out[(kbd_buf.size() + zuousername.size()) * 6 + 1 + 3];  // przyjęto najgorszy możliwy przypadek, gdzie są same 6-bajtowe znaki

    c_in[0] = '<';      // początek nawiasu przed nickiem
    memcpy(c_in + 1, zuousername.c_str(), zuousername.size());  // dopisz nick z czata
    c_in[zuousername.size() + 1] = '>';     // koniec nawiasu
    c_in[zuousername.size() + 2] = ' ';     // spacja za nawiasem
    memcpy(c_in + zuousername.size() + 3, kbd_buf.c_str(), kbd_buf.size());     // dopisz bufor klawiatury
    c_in[kbd_buf.size() + zuousername.size() + 3] = '\0';       // NULL na końcu

    char *c_in_ptr = c_in;
    size_t c_in_len = kbd_buf.size() + zuousername.size() + 1 + 3;
    char *c_out_ptr = c_out;
    size_t c_out_len = (kbd_buf.size() + zuousername.size()) * 6 + 1 + 3;

    iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");
    iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
    *c_out_ptr = '\0';
    iconv_close(cd);

    // normalne atrybuty fontu
    attrset(A_NORMAL);

    // ustaw kursor na początku ostatniego wiersza
    move(term_y - 1, 0);

    // wyświetl nick (z czata, nie ustawiony przez /nick) oraz zawartość przekonwertowanego bufora
    printw("%s", c_out);

    // pozostałe znaki w wierszu wykasuj
    clrtoeol();

    // ustaw kursor w obecnie przetwarzany znak (+ długość nicka, nawias i spacja)
    move(term_y - 1, kbd_buf_pos + zuousername.size() + 3);

    // odświeżenie ekranu nastąpi poza funkcją
}


void wprintw_iso2utf(WINDOW *active_window, bool use_colors, short color_p, std::string buffer_str)
{
    // funckja wypisuje zawartość bufora, dodaje do początku wiersza czas oraz wykrywa polskie znaki w kodowaniu ISO-8859-2 i konwertuje je na UTF-8,
    //  jeśli textbox = true, nie będzie pokazywany czas oraz nie będzie przechodzenia do nowego wiersza przed pętlą,
    //  polskie znaki w kodowaniu UTF-8 wyświetla bez konwersji

    unsigned char c;            // aktualnie przetwarzany znak z bufora
    int pos_buffer_end;
    char time_hms[20];          // tablica do pobrania aktualnego czasu [HH:MM:SS] (z nadmiarem)

    // zacznij od przejścia do nowego wiersza, ale tylko, gdy to nie dotyczy paska wpisywania tekstu
    wprintw(active_window, "\n");
    // pokaż czas w każdym wywołaniu tej funkcji (reszta w pętli), ale tylko, gdy to nie dotyczy paska wpisywania tekstu
    wattrset(active_window, A_NORMAL);      // czas ze zwykłymi atrybutami fontu
    get_time(time_hms);
    wprintw(active_window, "%s", time_hms);
    wattrset_color(active_window, use_colors, color_p);     // przywróc kolor wejściowy

    // wyświetl bufor bez ostatniego kodu \n (wykryj, czy ten kod tam jest)
    if(buffer_str[buffer_str.size() - 1] == '\n')
        pos_buffer_end = buffer_str.size() - 1;     // jeśli jest kod \n, pętla wykona się o 1 mniej niż wielkość bufora
    else
        pos_buffer_end = buffer_str.size();         // w przeciwnym razie pętla wykona się tyle razy, ile ma wielkość bufora (nie ma błędu, bo w pętli i = 0)

    // wypisywanie w pętli
    for(int i = 0; i < pos_buffer_end; ++i)
    {
        // pobierz znak z bufora
        c = buffer_str[i];

        // najpierw wykryj polskie znaki w kodowaniu ISO-8859-2
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
            wprintw(active_window, "%c", c);   // gdy to nie był polski znak w kodowaniu ISO-8859-2, wpisz odczytaną wartość bez modyfikacji
            break;
        }

        // po każdym znaku \n pokaż czas (na początku nowego wiersza)
        if(buffer_str[i] == '\n')
        {
            wattrset(active_window, A_NORMAL);      // czas ze zwykłymi atrybutami fontu
            get_time(time_hms);
            wprintw(active_window, "%s", time_hms);
            wattrset_color(active_window, use_colors, color_p);     // przywróc kolor wejściowy
        }
    }

    // odświeżenie ekranu nastąpi poza funkcją
}
