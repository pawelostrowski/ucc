#include <string>           // std::string, setlocale()
#include <cerrno>           // errno
#include <sys/select.h>     // select()

#include "main_window.hpp"
#include "window_utils.hpp"
#include "kbd_parser.hpp"
#include "irc_parser.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_colors.hpp"


int main_window(bool use_colors, bool ucc_dbg_irc)
{
    // zapobiega zapętleniu się programu po wpisaniu w terminalu czegoś w stylu 'echo text | ucc'
    if(freopen("/dev/tty", "r", stdin) == NULL)
        return 1;

    setlocale(LC_ALL, "");      // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    // inicjalizacja ncurses
    if(! initscr())
        return 2;

    bool ucc_quit = false;      // aby zakończyć program, zmienna ta musi mieć wartość prawdziwą
    bool command_ok = false;    // true, gdy wpisano polecenie
    bool captcha_ready = false; // stan wczytania captcha (jego pobranie z serwera)
    bool irc_ready = false;     // gotowość do połączenia z czatem, po połączeniu jest ustawiany na false
    bool irc_auth_status;       // status wykonania którejś z funkcji irc_auth_x()
    bool irc_ok = false;        // stan połączenia z czatem
    bool send_irc = false;      // true oznacza, że irc_parser() zwrócił PONG do wysłania do IRC
    bool channel_ok = false;    // stan wejścia do pokoju (kanału)
    bool command_me = false;    // true oznacza, że wpisano polecenie /me, które trzeba wyświetlić z uwzględnieniem kodowania bufora w ISO-8859-2
    int term_y, term_x;         // wymiary terminala
    int cur_y, cur_x;           // aktualna pozycja kursora
    int kbd_buf_pos = 0;        // początkowa pozycja bufora klawiatury (istotne podczas używania strzałek, Home, End, Delete itd.)
    int kbd_buf_max = 0;        // początkowy maksymalny rozmiar bufora klawiatury
    int key_code;               // kod ostatnio wciśniętego klawisza
    std::string key_code_tmp;   // tymczasowy bufor na odczytany znak z klawiatury (potrzebny podczas konwersji int na std::string)
    std::string kbd_buf;        // bufor odczytanych znaków z klawiatury
    std::string msg;            // komunikat do wyświetlenia z którejś z wywoływanych funkcji w main_window() (opcjonalny)
    std::string msg_irc;        // komunikat (polecenie) do wysłania do IRC po wywołaniu kbd_parser() (opcjonalny)
    std::string zuousername = "Niezalogowany";
    std::string cookies, my_nick, my_password, uokey, channel, msg_sock;
    int socketfd_irc = 0;       // gniazdo (socket), ale używane tylko w IRC (w HTTP nie będzie sprawdzany jego stan w select() ), 0, gdy nieaktywne
    std::string buffer_irc_recv;    // bufor odebranych danych z IRC
    std::string buffer_irc_sent;    // dane wysłane do serwera w irc_auth_x() (informacje przydatne do debugowania)

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
    wprintw_utf(win_diag, use_colors, UCC_GREEN, "Ucieszony Chat Client"
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

    refresh();
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
        attron(A_REVERSE);
        move(0, 0);
        for(int i = 0; i < term_x; i++)
            printw(" ");
        move(term_y - 2, 0);
        for(int i = 0; i < term_x; i++)
            printw(" ");


/*
    Tymczasowo pokazuj informacje pomocnicze, usunąć po testach
*/
        move(term_y - 2, 0);
        printw("sum: %d, kbd: %d, irc: %d", ix + iy, ix, iy);

        move(0, 0);
        printw("socketfd_irc: %d", socketfd_irc);
/*
    Koniec informacji tymczasowych
*/

        // wypisz zawartość bufora klawiatury (utworzonego w programie) w ostatnim wierszu (to, co aktualnie do niego wpisujemy)
        //  oraz ustaw kursor w obecnie przetwarzany znak
        kbd_buf_show(kbd_buf, zuousername, term_y, term_x, kbd_buf_pos);

        // odśwież okna (kolejność jest ważna, bo przy zmianie rozmiaru okna terminala odwrotna kolejność rozwala wygląd)
        wrefresh(win_diag);
        refresh();

        // czekaj na aktywność klawiatury lub gniazda (socket)
        if(select(socketfd_irc + 1, &readfds_tmp, NULL, NULL, NULL) == -1)
        {
            // sygnał SIGWINCH (zmiana rozmiaru okna terminala) powoduje, że select() zwraca -1, więc trzeba to wykryć, aby nie wywalić programu w kosmos
            if(errno == EINTR)      // Interrupted system call (wywołany np. przez SIGWINCH)
            {
                getch();            // ignoruj KEY_RESIZE
                continue;           // wróć do początku pętli while()
            }
            // inny błąd select() powoduje zakończenie działania programu
            else
            {
                delwin(win_diag);
                endwin();           // zakończ tryb ncurses
                fclose(stdin);
                return 3;
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

            else if(key_code == '\n')               // Enter (0x0A)
            {
                if(kbd_buf.size() != 0)             // wykonaj obsługę bufora tylko, gdy coś w nim jest
                {
                    // "wyczyść" pole wpisywanego tekstu (aby nie było widać zwłoki, np. podczas pobierania obrazka z kodem do przepisania)
                    move(term_y - 1, zuousername.size() + 3);       // ustaw kursor za nickiem i spacją za nawiasem
                    clrtoeol();
                    refresh();

                    // wykonaj obsługę bufora (zidentyfikuj polecenie), gdy funkcja zwróci false, wypisz na czerwono komunikat błędu
                    if(! kbd_parser(kbd_buf, msg, msg_irc, my_nick, my_password, zuousername, cookies, uokey, command_ok, captcha_ready, irc_ready, irc_ok,
                                    channel, channel_ok, command_me, ucc_quit))
                    {
                        wprintw_utf(win_diag, use_colors, UCC_RED, msg);
                    }
                    else
                    {
                        // jeśli wpisano zwykły tekst (nie polecenie), pokaż go wraz z nickiem i wyślij polecenie do IRC (wykrycie, czy połączono się z IRC oraz czy otwarty
                        //  jest aktywny pokój jest wykonywane w kbd_parser(), przy błędzie nie jest ustawiany command_ok, aby pokazać komunikat poniżej)
                        if(! command_ok)
                        {
                            // pokaż komunikat z uwzględnieniem tego, że w buforze jest kodowanie ISO-8859-2
                            wprintw_iso2utf(win_diag, use_colors, UCC_TERM, msg, true);
                            // wyślij wiadomość na serwer
                            if(! irc_send(socketfd_irc, irc_ok, msg_irc, msg_sock))
                            {
                                wprintw_utf(win_diag, use_colors, UCC_RED, msg_sock);       // w przypadku błędu pokaż, co się stało
                            }
                        }
                        // gdy kbd_parser() zwrócił jakiś komunikat przeznaczony do wyświetlenia na terminalu i nie jest to poloecenie /me, pokaż go (nie jest to błąd, więc na zielono)
                        else if(msg.size() != 0 && ! command_me)
                        {
                            wprintw_utf(win_diag, use_colors, UCC_GREEN, msg);
                        }
                        // polecenie /me jest z kodowaniem ISO-8859-2, więc tak je wyświetl
                        else if(msg.size() != 0 && command_me)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_MAGENTA, msg);
                        }
                        // gdy kbd_parser() zwrócił jakiś komunikat przeznaczony do wysłania do IRC i nie jest to zwykły tekst wypisany w wprintw_iso2utf(), wyślij go do IRC
                        //  (stan połączenia do IRC wykrywany jest w kbd_parser(), więc nie trzeba się obawiać, że komunikat zostanie wysłany do niezalogowanego czata)
                        if(msg_irc.size() != 0 && command_ok)
                        {
                            wprintw_iso2utf(win_diag, use_colors, UCC_BLUE, msg_irc);       // tymczasowo pokaż, co program wysyła na serwer
                            if(! irc_send(socketfd_irc, irc_ok, msg_irc, msg_sock))
                            {
                                wprintw_utf(win_diag, use_colors, UCC_RED, msg_sock);       // w przypadku błędu pokaż, co się stało
                            }
                        }
                    }

                    // sprawdź gotowość do połączenia z IRC
                    if(irc_ready)
                    {
                        irc_ready = false;      // po połączeniu nie próbuj się znowu łączyć do IRC od zera
                        // połącz z serwerem IRC
                        irc_auth_status = irc_auth_1(socketfd_irc, irc_ok, buffer_irc_recv, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);      // pokaż odpowiedź serwera
                        if(! irc_auth_status)
                        {
                            wprintw_utf(win_diag, use_colors, UCC_RED, msg);        // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: NICK <~nick>
                        irc_auth_status = irc_auth_2(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, zuousername, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);      // pokaż odpowiedź serwera
                        if(! irc_auth_status)
                        {
                            wprintw_utf(win_diag, use_colors, UCC_RED, msg);        // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: AUTHKEY
                        irc_auth_status = irc_auth_3(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        wprintw_iso2utf(win_diag, use_colors, UCC_WHITE, buffer_irc_recv);      // pokaż odpowiedź serwera
                        if(! irc_auth_status)
                        {
                            wprintw_utf(win_diag, use_colors, UCC_RED, msg);        // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: AUTHKEY <AUTHKEY>
                        irc_auth_status = irc_auth_4(socketfd_irc, irc_ok, buffer_irc_recv, buffer_irc_sent, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        if(! irc_auth_status)
                        {
                            wprintw_utf(win_diag, use_colors, UCC_RED, msg);        // w przypadku błędu pokaż, co się stało
                        }
                        // wyślij: USER * <uoKey> czat-app.onet.pl :<~nick>\r\nPROTOCTL ONETNAMESX
                        irc_auth_status = irc_auth_5(socketfd_irc, irc_ok, buffer_irc_sent, zuousername, uokey, msg);
                        wprintw_iso2utf(win_diag, use_colors, UCC_YELLOW, buffer_irc_sent);     // pokaż, co wysłano do serwera
                        if(! irc_auth_status)
                        {
                            wprintw_utf(win_diag, use_colors, UCC_RED, msg);        // w przypadku błędu pokaż, co się stało
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
            if(! irc_recv(socketfd_irc, irc_ok, buffer_irc_recv, msg_sock))
            {
                wprintw_utf(win_diag, use_colors, UCC_RED, msg_sock);       // w przypadku błędu pokaż, co się stało
            }

            // zinterpretuj odpowiedź
            irc_parser(buffer_irc_recv, msg, channel, send_irc, irc_ok);

            if(send_irc)
            {
                if(! irc_send(socketfd_irc, irc_ok, msg, msg_sock))         // dotychczas wysyłaną odpowiedzią w tym miejscu jest PONG
                {
                    wprintw_utf(win_diag, use_colors, UCC_RED, msg_sock);   // w przypadku błędu pokaż, co się stało
                }
            }

            else
            {
                if(msg.size() != 0)
                {
                    wprintw_iso2utf(win_diag, use_colors, UCC_TERM, msg);
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
    fclose(stdin);

    return 0;
}
