#include <string>		// std::string

#include "expression.hpp"


int find_cookies(char *c_buffer, std::string &cookies)
{
	size_t pos_cookie_start, pos_cookie_end;
	std::string cookie_tmp;
	std::string cookie_string = "Set-Cookie:";

	// std::string(c_buffer) zamienia C string na std::string
	pos_cookie_start = std::string(c_buffer).find(cookie_string);	// znajdź pozycję pierwszego cookie (od miejsca: Set-Cookie:)
	if(pos_cookie_start == std::string::npos)
		return 11;	// kod błędu, gdy nie znaleziono cookie (pierwszego)

	do
	{
		pos_cookie_end = std::string(c_buffer).find(";", pos_cookie_start);	// szukaj ";" od pozycji początku cookie
		if(pos_cookie_end == std::string::npos)
			return 12;	// kod błędu, gdy nie znaleziono oczekiwanego ";" na końcu każdego cookie

	cookie_tmp.clear();	// wyczyść bufor pomocniczy
	cookie_tmp.insert(0, std::string(c_buffer), pos_cookie_start + cookie_string.size(), pos_cookie_end - pos_cookie_start - cookie_string.size() + 1);	// skopiuj cookie
																																						//  do bufora pomocniczego
	cookies += cookie_tmp;	// dopisz kolejny cookie do bufora
	pos_cookie_start = std::string(c_buffer).find(cookie_string, pos_cookie_start + cookie_string.size());	// znajdź kolejny cookie
	} while(pos_cookie_start != std::string::npos);		// zakończ szukanie, gdy nie znaleziono kolejnego cookie

	return 0;
}


int find_value(char *c_buffer, std::string &expr_before, std::string &expr_after, std::string &f_value)
{
	size_t pos_expr_before, pos_expr_after;		// pozycja początkowa i końcowa szukanych wyrażeń

	f_value.clear();	// wyczyść bufor szukanej wartości

	pos_expr_before = std::string(c_buffer).find(expr_before);		// znajdź pozycję początku szukanego wyrażenia
	if(pos_expr_before == std::string::npos)
		return 21;		// kod błędu, gdy nie znaleziono początku szukanego wyrażenia

	pos_expr_after = std::string(c_buffer).find(expr_after, pos_expr_before + expr_before.size());		// znajdź pozycję końca szukanego wyrażenia,
																									//  zaczynając od znalezionego początku + jego jego długości
	if(pos_expr_after == std::string::npos)
		return 22;		// kod błędu, gdy nie znaleziono końca szukanego wyrażenia

	f_value.insert(0, std::string(c_buffer), pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());		// wstaw szukaną wartość

	return 0;
}
