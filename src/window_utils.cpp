#include <string>           // std::string, setlocale()
#include <cstring>          // strlen()
#include <ctime>            // czas
#include <iconv.h>          // konwersja kodowania znaków

#include "window_utils.hpp"
#include "ucc_colors.hpp"


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

    init_pair(UCC_RED, COLOR_RED, background_color);
    init_pair(UCC_GREEN, COLOR_GREEN, background_color);
    init_pair(UCC_YELLOW, COLOR_YELLOW, background_color);
    init_pair(UCC_BLUE, COLOR_BLUE, background_color);
    init_pair(UCC_MAGENTA, COLOR_MAGENTA, background_color);
    init_pair(UCC_CYAN, COLOR_CYAN, background_color);
    init_pair(UCC_WHITE, COLOR_WHITE, background_color);
    init_pair(UCC_TERM, font_color, background_color);
    init_pair(UCC_BLUE_WHITE, COLOR_BLUE, COLOR_WHITE);

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


void kbd_buf_show(std::string kbd_buf, std::string zuousername, int term_y, int term_x, int kbd_buf_pos, int kbd_buf_max)
{
    int cut_left = 0;
    int cut_right = 0;

    if(kbd_buf_pos + (int)zuousername.size() + 4 > term_x)
    {
        cut_left = kbd_buf_pos + zuousername.size() + 4 - term_x;
        kbd_buf.erase(0, cut_left);
    }

    if((int)kbd_buf.size() + (int)zuousername.size() + 3 > term_x)
    {
        cut_right = kbd_buf.size() + zuousername.size() + 3 - term_x;
        kbd_buf.erase((zuousername.size() + 3 - term_x) * -1, cut_right);
    }

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


void wprintw_utf(WINDOW *active_window, bool use_colors, short color_p, std::string buffer_str)
{
    int pos_buffer_end;
    char time_hms[20];          // tablica do pobrania aktualnego czasu [HH:MM:SS] (z nadmiarem)
    static bool first_use = true;

    // zacznij od przejścia do nowego wiersza, ale tylko, gdy to nie jest pierwsze użycie funkcji
    if(! first_use)
        wprintw(active_window, "\n");

    // kolejne użycie spowoduje przejście do nowego wiersza na początku
    first_use = false;

    // pokaż czas w każdym wywołaniu tej funkcji (reszta w pętli)
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
        if(buffer_str[i] == '\r')
            continue;

        wprintw(active_window, "%c", buffer_str[i]);

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


void wprintw_iso2utf(WINDOW *active_window, bool use_colors, short color_p, std::string buffer_str, bool bold_my_nick)
{
    // funckja wypisuje zawartość bufora, dodaje do początku wiersza czas oraz wykrywa polskie znaki w kodowaniu ISO-8859-2 i konwertuje je na UTF-8,
    //  jeśli textbox = true, nie będzie pokazywany czas oraz nie będzie przechodzenia do nowego wiersza przed pętlą

    unsigned char c;            // aktualnie przetwarzany znak z bufora
    int pos_buffer_end;
    int j;
    char time_hms[20];          // tablica do pobrania aktualnego czasu [HH:MM:SS] (z nadmiarem)
    std::string onet_color, onet_icon;

    // zacznij od przejścia do nowego wiersza
    wprintw(active_window, "\n");

    // pokaż czas w każdym wywołaniu tej funkcji (reszta w pętli)
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
        // obsługa % do wykrywania fontu, koloru i ikon
        j = i;
        do
        {
            if(buffer_str[j] == '%')
            {
                ++j;
                // wykryj atrybuty fontu
                if(buffer_str[j] == 'F' && j < pos_buffer_end)
                {
                    ++j;
                    if(buffer_str[j] == 'b' && j < pos_buffer_end)
                    {
                        for(++j; j < pos_buffer_end && j < pos_buffer_end; ++j)
                        {
                            if(buffer_str[j] == ' ')
                                break;
                            if(buffer_str[j] == '%')
                                break;
                        }
                        // gdy zakończono na %, zmień atrybuty
                        if(buffer_str[j] == '%')
                        {
                            wattron(active_window, A_BOLD);
                            i = j + 1;
                        }
                    }
                    // gdy to nie bold, pomiń %F...% (o ile to prawidłowy kod bez spacji wewnątrz)
                    else
                    {
                        for(++j; j < pos_buffer_end && j < pos_buffer_end; ++j)
                        {
                            if(buffer_str[j] == ' ')
                                break;
                            if(buffer_str[j] == '%')
                                break;
                        }
                        if(buffer_str[j] == '%')
                        {
                            i = j + 1;
                        }
                    }
                }
                // wykryj atrybuty koloru
                else if(buffer_str[j] == 'C' && j < pos_buffer_end)
                {
                    onet_color.clear();
                    // wczytaj kolor
                    for(++j; j < pos_buffer_end; ++j)
                    {
                        if(buffer_str[j] == ' ')
                            break;
                        if(buffer_str[j] == '%')
                            break;
                        onet_color += buffer_str[j];
                    }
                    if(buffer_str[j] == '%' && onet_color.size() == 6)  // kolor musi mieć 6 znaków
                    {
                        wattrset_color(active_window, use_colors, onet_color_conv(onet_color));
                        i = j + 1;
                    }
                }
                // ikony konwertuj na //nazwa_ikony
                else if(buffer_str[j] == 'I' && j < pos_buffer_end)
                {
                    onet_icon.clear();
                    // konwersja ikony
                    for(++j; j < pos_buffer_end; ++j)
                    {
                        if(buffer_str[j] == ' ')
                            break;
                        if(buffer_str[j] == '%')
                            break;
                        onet_icon += buffer_str[j];
                    }
                    if(buffer_str[j] == '%')
                    {
                        // wyświetl ikonę jako //nazwa_ikony
                        wprintw(active_window, "//%s", onet_icon.c_str());
                        i = j + 1;
                    }
                }
            }
        } while(buffer_str[j] == '%');

        // na razie mało elegancji sposób na pogrubienie własnego nicka (do poprawy jeszcze)
        if(bold_my_nick)
        {
            // wykryj początek nicka, który zaczyna się od <
            if(buffer_str[i] == '<')
            {
                wattrset_color(active_window, use_colors, UCC_TERM);
                wattron(active_window, A_BOLD);
            }
            // wykryj koniec nicka
            if(buffer_str[i - 1] == '>')
            {
                wattrset_color(active_window, use_colors, color_p);
                wattroff(active_window, A_BOLD);
            }
        }

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

        case '\r':
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


short onet_color_conv(std::string onet_color)
{
    if(onet_color == "623c00")  // brązowy, ale najbliższy to żółty
        return UCC_YELLOW;

    else if(onet_color == "c86c00") // ciemny pomarańczowy, ale najbliższy to żółty
        return UCC_YELLOW;

    else if(onet_color == "ff6500") // pomarańczowy, ale najbliższy to żółty
        return UCC_YELLOW;

    else if(onet_color == "ff0000") // czerwony
        return UCC_RED;

    else if(onet_color == "e40f0f") // ciemniejszy czerwony
        return UCC_RED;

    else if(onet_color == "990033") // bordowy
        return UCC_RED;

    else if(onet_color == "8800ab") // fioletowy
        return UCC_MAGENTA;

    else if(onet_color == "ce00ff") // magenta
        return UCC_MAGENTA;

    else if(onet_color == "0f2ab1") // granatowy
        return UCC_BLUE;

    else if(onet_color == "3030ce") // ciemny niebieski
        return UCC_BLUE;

    else if(onet_color == "006699") // cyjan
        return UCC_CYAN;

    else if(onet_color == "1a866e") // zielono-cyjanowy
        return UCC_CYAN;

    else if(onet_color == "008100") // zielony
        return UCC_GREEN;

    else if(onet_color == "959595") // szary
        return UCC_WHITE;

    return UCC_TERM;    // gdy żaden z wymienionych
}
