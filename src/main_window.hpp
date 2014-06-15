#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

// maksymalna pojemność bufora klawiatury (za duża pojemność przy pełnym wykorzystaniu może powodować rozłączenie z czatem podczas wysyłania wiadomości)
#define KBD_BUF_MAX_SIZE 400

// co ile sekund wysyłać PING do serwera
#define PING_TIME 10

// po jakim czasie braku odpowiedzi zerwać połączenie (w sekundach)
#define PING_TIMEOUT (PING_TIME * 6)

#if PING_TIMEOUT <= PING_TIME
#error The value of PING_TIMEOUT must be greater than the value of PING_TIME
#endif		// PING_TIMEOUT

// nick na pasku dolnym, gdy nie jesteśmy zalogowani do czata
#define NICK_NOT_LOGGED "Niezalogowany"

int main_window(bool use_colors_main, bool ucc_dbg_irc_main);

#endif		// MAIN_WINDOW_HPP
