#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

// maksymalna pojemność bufora klawiatury (za duża pojemność przy pełnym wykorzystaniu może powodować rozłączenie z czatem podczas wysyłania wiadomości)
#define KBD_BUF_MAX_SIZE 400

// nick na pasku dolnym, gdy nie jesteśmy zalogowani do czata
#define NICK_NOT_LOGGED "Niezalogowany"

int main_window(bool use_colors_main, bool ucc_dbg_irc);

#endif		// MAIN_WINDOW_HPP
