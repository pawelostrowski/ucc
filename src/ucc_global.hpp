#ifndef UCC_GLOBAL_HPP
#define UCC_GLOBAL_HPP

// przypisanie własnych nazw kolorów dla zainicjalizowanych par kolorów
#define UCC_RED         1
#define UCC_GREEN       2
#define UCC_YELLOW      3
#define UCC_BLUE        4
#define UCC_MAGENTA     5
#define UCC_CYAN        6
#define UCC_WHITE       7
#define UCC_TERM        8	// kolor zależny od ustawień terminala
#define UCC_BLUE_WHITE  9	// niebieski font, białe tło

struct channel_irc
{
        std::string win_buf_chan;
        std::string channel;
};

#endif		// UCC_GLOBAL_HPP
