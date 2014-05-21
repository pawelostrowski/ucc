#ifndef UCC_GLOBAL_HPP
#define UCC_GLOBAL_HPP

// przypisanie własnych nazw kolorów dla zainicjalizowanych par kolorów
#define pRED		1
#define pGREEN		2
#define pYELLOW		3
#define pBLUE		4
#define pMAGENTA	5
#define pCYAN		6
#define pWHITE		7
#define pTERMC		8	// kolor zależny od ustawień terminala
#define pBLUE_WHITE	9	// niebieski font, białe tło

// definicje kodów kolorów używanych w string (same numery kolorów muszą być te same, co powyżej, aby kolor się zgadzał, natomiast \x03 to umowny kod koloru)
#define xRED		"\x03\x01"
#define xGREEN		"\x03\x02"
#define xYELLOW		"\x03\x03"
#define xBLUE		"\x03\x04"
#define xMAGENTA	"\x03\x05"
#define xCYAN		"\x03\x06"
#define xWHITE		"\x03\x07"
#define xTERMC		"\x03\x08"
#define xBLUE_WHITE	"\x03\x09"

// definicje formatowania testu (kody umowne)
#define xBOLD_ON	"\x04"
#define xBOLD_OFF	"\x05"
#define xREVERSE_ON	"\x11"
#define xREVERSE_OFF	"\x12"
#define xUNDERLINE_ON	"\x13"
#define xUNDERLINE_OFF	"\x14"
#define xNORMAL		"\x17"		// przywraca ustawienia domyślne (atrybuty normalne, czyli brak kolorów, bolda, itd.)

// maksymalna liczba kanałów wraz z kanałem "Status" oraz "Debug"
#define CHAN_MAX	20 + 2		// kanały czata + "Status" i "Debug"

// nadanie numerów w tablicy kanałom: "Status" i "Debug"
#define CHAN_STATUS	0
#define CHAN_DEBUG_IRC	CHAN_MAX	// - 1, bo liczymy od zera

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

	int chan_nr;

	std::string msg_err;

	std::string my_nick;
	std::string my_password;
	std::string zuousername;
	std::string uokey;
	std::string cookies;
};

// struktura kanału
struct channel_irc
{
	bool channel_ok;	// czy to kanał czata i czy można w nim pisać (czy np. nie wyrzucono z pokoju, mimo, że jest on pokazywany)

	std::string win_buf;
	std::string channel;
};

// struktura nicka (każdego na czacie, ale nie własnego, który jest w global_args)
struct nick_irc
{
	std::string nick;
	std::string zuo;
//	bool index_chan[CHAN_CHAT_MAX];
};

#endif		// UCC_GLOBAL_HPP
