#ifndef UCC_GLOBAL_HPP
#define UCC_GLOBAL_HPP

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8 (w tym miejscu dodano ze względu na WINDOW)

// przypisanie własnych nazw kolorów dla zainicjalizowanych par kolorów
#define pRED		1
#define pGREEN		2
#define pYELLOW		3
#define pBLUE		4
#define pMAGENTA	5
#define pCYAN		6
#define pWHITE		7
#define pTERMC		8	// kolor zależny od ustawień terminala
#define pWHITE_BLUE	9	// biały font, niebieskie tło
#define pCYAN_BLUE	10	// cyjan font, niebieskie tło
#define pMAGENTA_BLUE	11	// magenta font, niebieskie tło

// definicje kodów kolorów używanych w string (same numery kolorów muszą być te same, co powyżej, aby kolor się zgadzał, natomiast \x03 to umowny kod koloru)
#define xRED		"\x03\x01"
#define xGREEN		"\x03\x02"
#define xYELLOW		"\x03\x03"
#define xBLUE		"\x03\x04"
#define xMAGENTA	"\x03\x05"
#define xCYAN		"\x03\x06"
#define xWHITE		"\x03\x07"
#define xTERMC		"\x03\x08"
#define xWHITE_BLUE	"\x03\x09"
#define xCYAN_BLUE	"\x03\x0A"
#define xMAGENTA_BLUE	"\x03\x0B"

// definicje formatowania testu (kody umowne)
#define xBOLD_ON	"\x04"
#define xBOLD_OFF	"\x05"
#define xREVERSE_ON	"\x11"
#define xREVERSE_OFF	"\x12"
#define xUNDERLINE_ON	"\x13"
#define xUNDERLINE_OFF	"\x14"
#define xNORMAL		"\x17"		// przywraca ustawienia domyślne (atrybuty normalne, czyli brak kolorów, bolda, itd.)

// maksymalna liczba kanałów (dodano zapas) wraz z kanałem "Status" oraz "Debug"
#define CHAN_MAX	22 + 2		// kanały czata + "Status" i "Debug"

// nadanie numerów w tablicy kanałom "Status" i "Debug"
#define CHAN_STATUS	0		// "Status" zawsze pod numerem 0 w tablicy
#define CHAN_DEBUG_IRC	CHAN_MAX	// "Debug" zawsze jako ostatni w tablicy

// struktura zmiennych (wybranych) używanych w całym programie
struct global_args
{
	WINDOW *win_chat;

	bool use_colors;
	int wcur_y, wcur_x;

	bool ucc_quit;

	int socketfd_irc;

	bool captcha_ready;
	bool irc_ready;
	bool irc_ok;

	int current_chan;

	std::string my_nick;
	std::string my_password;
	std::string zuousername;
	std::string uokey;
	std::string cookies;

	std::string message_day;

	// tymczasowo NAMES do zmiennej, zanim nie zostanie zaimplementowana lista nicków
	std::string names;
};

// struktura kanału
struct channel_irc
{
	bool channel_ok;	// czy to kanał czata i czy można w nim pisać (czy np. nie wyrzucono z pokoju, mimo, że jest on pokazywany)

	int chan_act;           // 0 - brak aktywności, 1 - wejścia/wyjścia itp., 2 - ktoś pisze, 3 - ktoś pisze mój nick

	std::string win_buf;
	std::string channel;
	std::string topic;
};

// struktura nicka (każdego na czacie, ale nie własnego, który jest w global_args)
struct nick_irc
{
	std::string nick;
	std::string zuo;
//	bool index_chan[CHAN_CHAT_MAX];
};

#endif		// UCC_GLOBAL_HPP
