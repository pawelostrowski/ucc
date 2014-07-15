#ifndef UCC_GLOBAL_HPP
#define UCC_GLOBAL_HPP

#include <ncursesw/ncurses.h>
#include <fstream>
#include <map>

#define UCC_NAME "Ucieszony Chat Client"
#define UCC_VER "v1.0 alpha2"

#define FILE_DBG_HTTP "/tmp/ucc_dbg_http.txt"
#define FILE_DBG_IRC "/tmp/ucc_dbg_irc.txt"

// szerokość listy nicków
#define NICKLIST_WIDTH 36

// przypisanie własnych nazw kolorów dla zainicjalizowanych par kolorów
#define pRED		0x01
#define pGREEN		0x02
#define pYELLOW		0x03
#define pBLUE		0x04
#define pMAGENTA	0x05
#define pCYAN		0x06
#define pWHITE		0x07
#define pDARK		0x08
#define pTERMC		0x09	// kolor zależny od ustawień terminala
#define pWHITE_BLUE	0x0A	// biały font, niebieskie tło
#define pCYAN_BLUE	0x0B	// cyjan font, niebieskie tło
#define pMAGENTA_BLUE	0x0C	// magenta font, niebieskie tło
#define pBLACK_BLUE	0x0D	// czarny font, niebieskie tło
#define pYELLOW_BLACK	0x0E	// żółty font, czarne tło
#define pBLUE_WHITE	0x0F	// niebieski font, białe tło

// definicje kodów kolorów używanych w string (same numery kolorów muszą być te same, co powyżej, aby kolor się zgadzał, natomiast \x03 to umowny kod koloru)
#define xCOLOR		"\x03"
#define xRED		xCOLOR "\x01"
#define xGREEN		xCOLOR "\x02"
#define xYELLOW		xCOLOR "\x03"
#define xBLUE		xCOLOR "\x04"
#define xMAGENTA	xCOLOR "\x05"
#define xCYAN		xCOLOR "\x06"
#define xWHITE		xCOLOR "\x07"
#define xDARK		xCOLOR "\x08"
#define xTERMC		xCOLOR "\x09"
#define xWHITE_BLUE	xCOLOR "\x0A"
#define xCYAN_BLUE	xCOLOR "\x0B"
#define xMAGENTA_BLUE	xCOLOR "\x0C"
#define xBLACK_BLUE	xCOLOR "\x0D"
#define xYELLOW_BLACK	xCOLOR "\x0E"
#define xBLUE_WHITE	xCOLOR "\x0F"

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

// liczba pokoi czata (liczona od zera, czyli najwyższy numer w tablicy pokoi będzie o 1 mniejszy)
#define CHAN_CHAT		20

// numer dla "Status" w tablicy pokoi
#define CHAN_STATUS		(CHAN_CHAT + 0)

// numer dla "Debug" w tablicy pokoi
#define CHAN_DEBUG_IRC		(CHAN_CHAT + 1)

// numer dla "RawUnknown" w tablicy pokoi
#define CHAN_RAW_UNKNOWN	(CHAN_CHAT + 2)

// najwyższy numer pokoju minus jeden, w którym wyświetlane są normalne powiadomienia (czyli pokoje czata wraz ze "Status")
#define CHAN_NORMAL		(CHAN_CHAT + 1)

// maksymalna łączna liczba pokoi
#define CHAN_MAX		(CHAN_CHAT + 3)

// struktura zmiennych (wybranych) używanych w całym programie
struct global_args
{
	WINDOW *win_chat, *win_info;
	int wcur_y, wcur_x;

	bool use_colors;

	bool ucc_dbg_irc;

	std::ofstream f_dbg_http, f_dbg_irc;

	bool ucc_quit;

	bool nicklist, nicklist_refresh;

	int socketfd_irc;

	bool captcha_ready;
	bool irc_ready;
	bool irc_ok;

	int current;			// aktualnie otwarty pokój

	std::string my_nick;
	std::string my_password;
	std::string zuousername;
	std::string uokey;
	std::string cookies;

	long ping, pong, lag;
	bool lag_timeout;

	bool busy_state;

	std::string names, cs_homes;

	// używane podczas pobierania wizytówki
	std::string card_avatar, card_birthdate, card_city, card_country, card_email, card_long_desc, card_offmsg, card_prefs,
			card_rank, card_sex, card_short_desc, card_type, card_v_email, card_www;

	std::string my_friend, my_ignore, my_favourites;

	std::string msg_away;

	// poniższe flagi służą do odpowiedniego sterowania wyświetlanych informacji zależnie od tego, czy serwer sam je zwrócił, czy po wpisaniu polecenia
	bool command_card, command_join, command_names, command_names_empty, command_vhost;
};

// wybrane flagi nicka na czacie (wszystkie nie będą używane)
struct nick_flags
{
	bool owner;
	bool op;
	bool halfop;
	bool moderator;
	bool voice;
	bool public_webcam;
	bool private_webcam;
	bool busy;
};

// struktura nicka na czacie
struct nick_irc
{
	std::string nick;
	std::string zuo;
	struct nick_flags flags;
};

// struktura kanału
struct channel_irc
{
	std::string win_buf;
	std::string channel;
	std::string topic;

	int chan_act;           // 0 - brak aktywności, 1 - wejścia/wyjścia itp., 2 - ktoś pisze, 3 - ktoś pisze mój nick

	size_t win_scroll;	// scroll okna, -1 oznacza ciągłe przesuwanie aktualnego tekstu

        std::map<std::string, struct nick_irc> nick_parm;
};

#endif		// UCC_GLOBAL_HPP
