/*
	Ucieszony Chat Client
	Copyright (C) 2013, 2014 Paweł Ostrowski

	This file is part of Ucieszony Chat Client.

	Ucieszony Chat Client is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	Ucieszony Chat Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Ucieszony Chat Client (in the file LICENSE); if not,
	see <http://www.gnu.org/licenses/gpl-2.0.html>.
*/


#ifndef UCC_GLOBAL_HPP
#define UCC_GLOBAL_HPP

// kompilacja na Windows w Cygwin
#ifdef __CYGWIN__

#include <sstream>
#include <cerrno>
#include <cstdlib>

namespace std
{
	template <typename T> std::string to_string(const T &buf_number)
	{
		std::stringstream buf_number_stream;
		buf_number_stream << buf_number;
		return buf_number_stream.str();
	}

	template <typename T> int stoi(const T &buf)
	{
		return atoi(buf.c_str());
	}

	template <typename T> int64_t stol(const T &buf)
	{
		return atol(buf.c_str());
	}
}

#endif		// __CYGWIN__

#include <ncursesw/ncurses.h>	// wersja ncurses ze wsparciem dla UTF-8
#include <fstream>
#include <map>
#include <vector>

// nazwa i numer wersji programu
#define UCC_NAME	"Ucieszony Chat Client"
#define UCC_VER		"1.0 alpha7"

// katalog ucc (sama nazwa, nie jego położenie)
#define UCC_DIR		".ucc"

// minimalna szerokość listy nicków
#define NICKLIST_WIDTH_MIN	24

// początek i koniec logu
#define LOG_STARTED	"--- Rozpoczęto log: " + get_time_full() + " ---" << std::endl
#define LOG_STOPPED	"--- Zakończono log: " + get_time_full() + " ---" << std::endl << std::endl

// przypisanie własnych nazw kolorów dla zainicjalizowanych par kolorów
// (kody 0x09 i 0x0A są pominięte, bo w win_buf_add_str() są interpretowane jako \t i \n)
#define pRED		0x01
#define pGREEN		0x02
#define pYELLOW		0x03
#define pBLUE		0x04
#define pMAGENTA	0x05
#define pCYAN		0x06
#define pWHITE		0x07
#define pDARK		0x08
#define pTERMC		0x0B	// kolor zależny od ustawień terminala
#define pYELLOW_BLACK	0x0C	// żółty font, czarne tło
#define pBLACK_BLUE	0x0D	// czarny font, niebieskie tło
#define pMAGENTA_BLUE	0x0E	// magenta font, niebieskie tło
#define pCYAN_BLUE	0x0F	// cyjan font, niebieskie tło
#define pWHITE_BLUE	0x10	// biały font, niebieskie tło

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
#define xTERMC		xCOLOR "\x0B"
#define xYELLOW_BLACK	xCOLOR "\x0C"
#define xBLACK_BLUE	xCOLOR "\x0D"
#define xMAGENTA_BLUE	xCOLOR "\x0E"
#define xCYAN_BLUE	xCOLOR "\x0F"
#define xWHITE_BLUE	xCOLOR "\x10"

// definicje formatowania tekstu (kody umowne)
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

// często używane symbole informacyjne w odpowiednich kolorach z boldem
#define uINFOn	xBOLD_ON "-" xBLUE "#" xTERMC "-" xBOLD_OFF " "
#define uINFOb	xBOLD_ON "-" xBLUE "#" xTERMC "- "
#define oINFOn	xBOLD_ON "-" xBLUE "!" xTERMC "-" xBOLD_OFF " "
#define oINFOb	xBOLD_ON "-" xBLUE "!" xTERMC "- "
#define oINn	xBOLD_ON "-" xBLUE "!" xTERMC ">" xBOLD_OFF " "
#define oOUTn	xBOLD_ON "<" xBLUE "!" xTERMC "-" xBOLD_OFF " "

// liczba pokoi czata (liczona od zera, czyli najwyższy numer w tablicy pokoi będzie o 1 mniejszy)
#define CHAN_CHAT		20

// numer dla "Status" w tablicy pokoi
#define CHAN_STATUS		(CHAN_CHAT + 0)

// numer dla "DebugIRC" w tablicy pokoi
#define CHAN_DEBUG_IRC		(CHAN_CHAT + 1)

// numer dla "RawUnknown" w tablicy pokoi
#define CHAN_RAW_UNKNOWN	(CHAN_CHAT + 2)

// najwyższy numer pokoju minus jeden, w którym wyświetlane są normalne powiadomienia (czyli pokoje czata wraz ze "Status")
#define CHAN_NORMAL		(CHAN_CHAT + 1)

// maksymalna łączna liczba pokoi
#define CHAN_MAX		(CHAN_CHAT + 3)

// struktura flag do odpowiedniego sterowania wyświetlanych informacji zależnie od tego, czy serwer sam je zwrócił, czy po wpisaniu polecenia
struct command_flags
{
	bool card;
	bool favourites_empty;
	bool friends;
	bool friends_empty;
	bool ignore;
	bool ignore_empty;
	bool invite;
	bool join_priv;
	bool kick;
	bool lusers;
	bool names;
	bool names_empty;
	bool vhost;
	bool whois;
};

// struktura wizytówki pobieranej z serwera
struct card_onet
{
	std::string avatar;
	std::string birthdate;
	std::string city;
	std::string country;
	std::string email;
	std::string long_desc;
	std::string offmsg;
	std::string prefs;
	std::string rank;
	std::string sex;
	std::string short_desc;
	std::string type;
	std::string v_email;
	std::string www;
};

// struktura dla /kban i /kbanip
struct kban
{
	std::string nick;
	std::string chan;
	std::string reason;
};

// struktura zmiennych (wybranych) używanych w całym programie
struct global_args
{
	int tmp1, tmp2;

	WINDOW *win_chat;
	WINDOW *win_info;

	int wterm_y;
	int wterm_x;

	int time_len;

	bool use_colors;
	bool debug_irc;
	bool ucc_quit;
	bool ucc_quit_time;

	bool win_chat_refresh;

	bool win_info_state;
	bool win_info_refresh;

	int win_info_current_width;

	std::string ucc_home_dir;
	int ucc_home_dir_stat;

	std::string user_dir;

	std::ofstream debug_http_f;

	bool is_irc_recv_buf_incomplete;

	int socketfd_irc;

	bool captcha_ready;
	bool irc_ready;
	bool irc_ok;

	int current;		// aktualnie otwarty pokój

	std::string my_nick;
	std::string my_password;
	std::string zuousername;
	std::string uokey;
	std::string my_zuo;

	std::vector<std::string> cookies;

	bool my_away;
	bool my_busy;

	int64_t ping;
	int64_t pong;
	int64_t lag;
	bool lag_timeout;

	bool nick_in_use;
	bool authkey_failed;

	std::string cs_homes;

	std::string my_favourites;
	std::string my_friends;
	std::vector<std::string> my_friends_online;
	std::string my_ignore;

	std::string away_msg;

	struct command_flags cf;

	std::map<std::string, struct card_onet> co;

	std::map<std::string, std::string> names;

	std::string whois_short;
	std::map<std::string, std::vector<std::string>> whois;

	std::map<std::string, std::vector<std::string>> whowas;

	std::map<std::string, struct kban> kb;
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
	std::string zuo_ip;

	struct nick_flags nf;
};

// struktura kanału
struct channel_irc
{
	std::vector<std::string> win_buf;
	std::string channel;
	std::string topic;

        std::map<std::string, struct nick_irc> ni;

        std::ofstream chan_log;

	int chan_act;           // 0 - brak aktywności, 1 - wejścia/wyjścia itp., 2 - ktoś pisze, 3 - ktoś pisze mój nick

	bool win_scroll_lock;	// scroll okna, false oznacza ciągłe przesuwanie aktualnego tekstu
	int lock_act;		// analogicznie jak chan_act, ale dla napisu "--Więcej--" na dolnym pasku przy włączonym scrollu okna

	int win_pos_first;
	int win_skip_lead_first;

	int win_pos_last;
	int win_skip_lead_last;
};

#endif		// UCC_GLOBAL_HPP
