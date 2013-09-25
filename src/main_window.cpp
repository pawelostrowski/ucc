#include <iostream>
#include <cstring>      // strlen()
#include <sstream>
#include "ncursesw/ncurses.h"
#include "main_window.hpp"

#define UCC_VER "0.1"


void check_colors(bool &use_colors)
{

    use_colors = has_colors();      // sprawdź, czy terminal obsługuje kolory

    if(! use_colors)
        return;         // jeśli konsola nie wspiera kolorów, zakończ

/*
    Gdy konsola obsługuje kolory, włącz ich obsługę oraz zdefiniuj pary kolorów
*/

    start_color();

//    init_pair(1, COLOR_);
}


int main_window()
{
    int cur_y, cur_x;
    bool use_colors;
    char nick_c[33];     // maksymalna długość nicka (akceptowana przez serwer) - 1 (bo na końcu dodawany jest '\0')
    std::string nick;
    std::stringstream nick_tmp;

    setlocale(LC_ALL, "");  // aby polskie znaki w UTF-8 wyświetlały się prawidłowo

    if(! initscr())     // inicjalizacja ncurses
    {
        std::cerr << "Nie udało się zainicjalizować biblioteki ncurses!" << std::endl << "Kończenie." << std::endl;
        return 200;
    }

    raw();       // zablokuj możliwość wciśnięcia kombinacji Ctrl-C, Ctrl-Z (nie działa z getnstr() )
    keypad(stdscr, TRUE);   // klawisze sterujące nie będą wypisywane na konsoli

    check_colors(use_colors);

    printw("COLORS: %d, COLOR_PAIR: %d\n", COLORS, COLOR_PAIR(1));

    if(use_colors)
    {
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        attron(COLOR_PAIR(1));
    }
    printw("Ucieszony Chat Client, wersja %s\n", UCC_VER);

    attrset(A_NORMAL);
	printw("Podaj nick tymczasowy: ");
	attrset(A_BOLD);
	getyx(stdscr, cur_y, cur_x);        // pobierz pozycję kursora (przy niewpisaniu nicka kursor wraca na tę samą pozycję)
	do
	{
        beep();
        move(cur_y, cur_x);         // ustaw kursor w pozycji sprzed wejścia w pętlę (ma znaczenie tylko, gdy wpiszemy pusty nick)
		getnstr(nick_c, 33 - 1);    // - 1 na kod '\0'
        nick_tmp << nick_c;         // przepisz C string do std::stringstream
        nick = nick_tmp.str();      // teraz zamień na std::string
		while(nick.find(" ") != std::string::npos)
			nick.erase(nick.find(" "), 1);		// usuń spacje z nicka (nie może istnieć taki nick)
	} while(nick.size() == 0);

    printw("Wpisałeś nick '%s' o długości %d znaków\n", nick.c_str(), nick.size());

    refresh();


    getch();

    endwin();           // powrót do normalnego trybu terminala

    return 0;
}
