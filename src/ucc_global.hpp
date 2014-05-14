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

struct channel_irc
{
        std::string win_buf_chan;
        std::string channel;
};

#endif		// UCC_GLOBAL_HPP
