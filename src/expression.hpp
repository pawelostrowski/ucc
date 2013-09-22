#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP
#define EXPRESSION_HPP_NAME "expression"
#define COOKIE_STRING "Set-Cookie:"

int find_cookies(char *c_buffer, std::string &cookies);

int find_value(char *c_buffer, std::string expr_before, std::string expr_after, std::string &f_value);	// szuka wartości pomiędzy wyrażeniem początkowym i końcowym

#endif // EXPRESSION_HPP
