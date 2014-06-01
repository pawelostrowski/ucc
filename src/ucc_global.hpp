#ifndef UCC_GLOBAL_HPP
#define UCC_GLOBAL_HPP

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8 (w tym miejscu dodano ze względu na WINDOW)

// przypisanie własnych nazw kolorów dla zainicjalizowanych par kolorów
#define pRED		0x01
#define pGREEN		0x02
#define pYELLOW		0x03
#define pBLUE		0x04
#define pMAGENTA	0x05
#define pCYAN		0x06
#define pWHITE		0x07
#define pTERMC		0x08	// kolor zależny od ustawień terminala
#define pWHITE_BLUE	0x09	// biały font, niebieskie tło
#define pCYAN_BLUE	0x0A	// cyjan font, niebieskie tło
#define pMAGENTA_BLUE	0x0B	// magenta font, niebieskie tło
#define pYELLOW_BLACK	0x0C	// żółty font, czarne tło

// definicje kodów kolorów używanych w string (same numery kolorów muszą być te same, co powyżej, aby kolor się zgadzał, natomiast \x03 to umowny kod koloru)
#define xCOLOR		"\x03"
#define xRED		xCOLOR "\x01"
#define xGREEN		xCOLOR "\x02"
#define xYELLOW		xCOLOR "\x03"
#define xBLUE		xCOLOR "\x04"
#define xMAGENTA	xCOLOR "\x05"
#define xCYAN		xCOLOR "\x06"
#define xWHITE		xCOLOR "\x07"
#define xTERMC		xCOLOR "\x08"
#define xWHITE_BLUE	xCOLOR "\x09"
#define xCYAN_BLUE	xCOLOR "\x0A"
#define xMAGENTA_BLUE	xCOLOR "\x0B"
#define xYELLOW_BLACK	xCOLOR "\x0C"

// definicje formatowania testu (kody umowne)
#define xBOLD_ON	"\x04"
#define xBOLD_OFF	"\x05"
#define xREVERSE_ON	"\x11"
#define xREVERSE_OFF	"\x12"
#define xUNDERLINE_ON	"\x13"
#define xUNDERLINE_OFF	"\x14"
#define xNORMAL		"\x17"		// przywraca ustawienia domyślne (atrybuty normalne, czyli brak kolorów, bolda, itd.)

// wartości liczbowe dla pętli sprawdzającej kody (muszą być takie same jak wartości dla string, aby program działał prawidłowo)
#define dCOLOR		0x03
#define dBOLD_ON	0x04
#define dBOLD_OFF	0x05
#define dREVERSE_ON	0x11
#define dREVERSE_OFF	0x12
#define dUNDERLINE_ON	0x13
#define dUNDERLINE_OFF	0x14
#define dNORMAL		0x17

// maksymalna liczba kanałów (dodano zapas) wraz z kanałem "Status" oraz "Debug"
#define CHAN_MAX	22 + 2		// kanały czata + "Status" i "Debug"

// nadanie numerów w tablicy kanałom "Status" i "Debug"
#define CHAN_STATUS	0		// "Status" zawsze pod numerem 0 w tablicy
#define CHAN_DEBUG_IRC	CHAN_MAX - 1	// "Debug" zawsze jako ostatni w tablicy (- 1, bo liczymy od 0)

// struktura zmiennych (wybranych) używanych w całym programie
struct global_args
{
	WINDOW *win_chat;

	bool use_colors;
	int wcur_y, wcur_x;

	bool ucc_dbg_irc;

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

	long ping, pong, lag;
	bool lag_timeout;

	bool busy_state;

	std::string message_day;

	// tymczasowo NAMES do zmiennej, zanim nie zostanie zaimplementowana lista nicków
	std::string names;

	// używane podczas pobierania wizytówki
	std::string card_avatar, card_birthdate, card_city, card_country, card_email, card_long_desc, card_offmsg, card_prefs,
			card_rank, card_sex, card_short_desc, card_type, card_v_email, card_www;
	std::string card_friend, card_ignore, card_favourites;
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
