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

// globalne zmienne, używane w całym programie
struct ucc_global_args
{
    bool ucc_quit;
    bool command_ok;
    bool captcha_ready;
    bool irc_ready;
    bool irc_ok;
    bool channel_ok;
    bool command_me;
    std::string kbd_buf;	// bufor odczytanych znaków z klawiatury
    std::string msg;		// komunikat do wyświetlenia z którejś z wywoływanych funkcji w main_window() (opcjonalny)
    std::string msg_irc;	// komunikat (polecenie) do wysłania do IRC po wywołaniu kbd_parser() lub irc_parser() (opcjonalny)
    std::string my_nick;
    std::string my_password;
    std::string cookies;
    std::string zuousername;
    std::string uokey;
    std::string channel;
    std::string msg_sock;
};

#endif		// UCC_GLOBAL_HPP
