#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP
#define EXPRESSION_HPP_NAME "expression"

int find_cookies(char *c_buffer, std::string &cookies);

int find_value(char *c_buffer, std::string expr_before, std::string expr_after, std::string &f_value);	// szuka wartości pomiędzy wyrażeniem początkowym i końcowym

void header_get(std::string host, std::string data_get, std::string cookies, std::string &data_send, bool add_cookies = false);

void header_post(std::string cookies, std::string api_function, std::string &data_send);

#endif      // EXPRESSION_HPP
