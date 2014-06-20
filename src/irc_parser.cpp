#include <string>		// std::string
#include <fstream>		// std::ifstream
#include <sys/time.h>		// gettimeofday()

#include "irc_parser.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


std::string get_value_from_buf(std::string buffer_str, std::string expr_before, std::string expr_after)
{
/*
	Znajdź i pobierz wartość pomiędzy wyrażeniem początkowym i końcowym.
*/

	size_t pos_expr_before, pos_expr_after;		// pozycja początkowa i końcowa szukanych wyrażeń
	std::string f_value_from_buf;

	// znajdź pozycję początku szukanego wyrażenia
	pos_expr_before = buffer_str.find(expr_before);

	if(pos_expr_before != std::string::npos)
	{
		// znajdź pozycję końca szukanego wyrażenia, zaczynając od znalezionego początku + jego jego długości
		pos_expr_after = buffer_str.find(expr_after, pos_expr_before + expr_before.size());

		if(pos_expr_after != std::string::npos)
		{
			// wstaw szukaną wartość
			f_value_from_buf.insert(0, buffer_str, pos_expr_before + expr_before.size(),
						pos_expr_after - pos_expr_before - expr_before.size());
		}
	}

	return f_value_from_buf;	// zwróć szukaną wartość między wyrażeniami lub pusty bufor, gdy nie znaleziono początku lub końca
}


void get_raw_parm(std::string &buffer_irc_raw, std::string *raw_parm)
{
/*
	Pobierz tyle pierwszych wartości między spacjami z RAW ile jest w RAW_PARM_MAX (lub mniej, jeśli tyle nie ma w buforze).
*/

	int buffer_irc_raw_len = buffer_irc_raw.size() - 1;	// bez kodu \n (musi być zawsze na końcu bufora)
	int raw_nr = 0;

	// wyczyść tablice
	for(int i = 0; i < RAW_PARM_MAX; ++i)
	{
		raw_parm[i].clear();	// 'i' odwołuje się do wskaźnika tablicy
	}

	// pobierz kolejne wartości do tablic
	for(int i = 0; i < buffer_irc_raw_len && raw_nr < RAW_PARM_MAX; ++i)
	{
		if(buffer_irc_raw[i] == ' ')
		{
			++raw_nr;

			// jeśli za wartością są kolejne spacje, usuń je (np. WHOWAS może zwrócić dodatkową spację, gdy dzień miesiąca jest w zakresie 1...9)
			for(int j = i + 1; j < buffer_irc_raw_len; ++j)
			{
				if(buffer_irc_raw[j] != ' ')
				{
					break;
				}

				++i;
			}
		}

		else
		{
			raw_parm[raw_nr] += buffer_irc_raw[i];	// tu 'raw_nr' odwołuje się do wskaźnika na tablicę std::string, nie mylić z numerem elementu
		}
	}

	// jeśli w zwróconych wartościach jest dwukropek na początku, to go usuń
	for(int i = 0; i < RAW_PARM_MAX && raw_parm[i].size() > 0; ++i)
	{
		if(raw_parm[i][0] == ':')
		{
			raw_parm[i].erase(0, 1);
		}
	}
}


void irc_parser(struct global_args &ga, struct channel_irc *chan_parm[])
{
	std::string buffer_irc_recv;
	std::string buffer_irc_raw;
	size_t pos_raw_start = 0, pos_raw_end = 0;

	std::string raw_parm[RAW_PARM_MAX];
	int raw_numeric;
	bool raw_unknown;

	// pobierz odpowiedź z serwera
	irc_recv(ga, chan_parm, buffer_irc_recv);

	// w przypadku błędu podczas pobierania danych zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// konwersja formatowania fontów, kolorów i emotikon
	buffer_irc_recv = form_from_chat(buffer_irc_recv);

	// obsłuż bufor
	do
	{
		// znajdź koniec wiersza
		pos_raw_end = buffer_irc_recv.find("\n", pos_raw_start);

		// nie może dojść do sytuacji, że na końcu wiersza nie ma \n
		if(pos_raw_end == std::string::npos)
		{
			pos_raw_start = 0;
			pos_raw_end = 0;

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Błąd w buforze IRC!");

			return;		// POPRAWIĆ TO WYCHODZENIE!!!
		}

		// wstaw aktualnie obsługiwany wiersz (RAW)
		buffer_irc_raw.clear();
		buffer_irc_raw.insert(0, buffer_irc_recv, pos_raw_start, pos_raw_end - pos_raw_start + 1);

		// przyjmij, że kolejny wiersz jest za kodem \n, a jeśli to koniec bufora, wykryte to będzie na końcu pętli do{} while()
		pos_raw_start = pos_raw_end + 1;

		// pobierz parametry RAW, aby wykryć dalej, jaki to rodzaj RAW
		get_raw_parm(buffer_irc_raw, raw_parm);

		// dla RAW numerycznych zamień string na int
		raw_numeric = std::stoi("0" + raw_parm[1]);	// std::string na int ("0" - zabezpieczenie się przed próbą zabronionej konwersji znaków
								// innych niż cyfry na początku)

/*
	Zależnie od rodzaju RAW wywołaj odpowiednią funkcję.
*/
		raw_unknown = false;

		if(raw_parm[0] == "ERROR")
		{
			raw_error(ga, chan_parm, buffer_irc_raw);
		}

		else if(raw_parm[0] == "PING")
		{
			raw_ping(ga, chan_parm, raw_parm);
		}

		else if(raw_parm[1] == "INVIGNORE")
		{
			raw_invignore(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "INVITE")
		{
			raw_invite(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "INVREJECT")
		{
			raw_invreject(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "JOIN")
		{
			raw_join(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "KICK")
		{
			raw_kick(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "MODE")
		{
			raw_mode(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "MODERMSG")
		{
			raw_modermsg(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "PART")
		{
			raw_part(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "PONG")
		{
			raw_pong(ga);
		}

		else if(raw_parm[1] == "PRIVMSG")
		{
			raw_privmsg(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "QUIT")
		{
			raw_quit(ga, chan_parm, buffer_irc_raw);
		}

		else if(raw_parm[1] == "TOPIC")
		{
			raw_topic(ga, chan_parm, raw_parm, buffer_irc_raw);
		}

		// RAW numeryczne zwykłe (nie z NOTICE)
		else if(raw_numeric > 0)
		{
			switch(raw_numeric)
			{
			case 001:
				raw_001(ga);
				break;

			case 002:
				raw_002();
				break;

			case 003:
				raw_003();
				break;

			case 004:
				raw_004();
				break;

			case 005:
				raw_005();
				break;

			case 251:
				raw_251();
				break;

			case 252:
				raw_252();
				break;

			case 253:
				raw_253();
				break;

			case 254:
				raw_254();
				break;

			case 255:
				raw_255();
				break;

			case 256:
				raw_256(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 257:
				raw_257(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 258:
				raw_258(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 259:
				raw_259(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 265:
				raw_265();
				break;

			case 266:
				raw_266();
				break;

			case 301:
				raw_301(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 303:
				raw_303(ga, chan_parm, buffer_irc_raw);
				break;

			case 304:
				raw_304(ga, chan_parm, buffer_irc_raw);
				break;

			case 305:
				raw_305(ga, chan_parm);
				break;

			case 306:
				raw_306(ga, chan_parm);
				break;

			case 307:
				raw_307(ga, chan_parm, raw_parm);
				break;

			case 311:
				raw_311(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 312:
				raw_312(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 314:
				raw_314(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 317:
				raw_317(ga, chan_parm, raw_parm);
				break;

			case 318:
				raw_318();
				break;

			case 319:
				raw_319(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 332:
				raw_332(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 333:
				raw_333(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 341:
				raw_341();
				break;

			case 353:
				raw_353(ga, buffer_irc_raw);
				break;

			case 366:
				raw_366(ga, chan_parm, raw_parm);
				break;

			case 369:
				raw_369();
				break;

			case 372:
				raw_372(ga, chan_parm, buffer_irc_raw);
				break;

			case 375:
				raw_375(ga, chan_parm);
				break;

			case 376:
				raw_376(chan_parm);
				break;

			case 378:
				raw_378(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 391:
				raw_391(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 396:
				raw_396(ga, chan_parm, raw_parm);
				break;

			case 401:
				raw_401(ga, chan_parm, raw_parm);
				break;

			case 402:
				raw_402(ga, chan_parm, raw_parm);
				break;

			case 403:
				raw_403(ga, chan_parm, raw_parm);
				break;

			case 404:
				raw_404(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 405:
				raw_405(ga, chan_parm, raw_parm);
				break;

			case 406:
				raw_406(ga, chan_parm, raw_parm);
				break;

			case 412:
				raw_412(ga, chan_parm, raw_parm);
				break;

			case 421:
				raw_421(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 433:
				raw_433(ga, chan_parm, raw_parm);
				break;

			case 451:
				raw_451(ga, chan_parm, raw_parm);
				break;

			case 461:
				raw_461(ga, chan_parm, raw_parm);
				break;

			case 473:
				raw_473(ga, chan_parm, raw_parm);
				break;

			case 482:
				raw_482(ga, chan_parm, raw_parm);
				break;

			case 530:
				raw_530(ga, chan_parm, raw_parm);
				break;

			case 600:
				raw_600(ga, chan_parm, raw_parm);
				break;

			case 601:
				raw_601(ga, chan_parm, raw_parm);
				break;

			case 602:
				raw_602();
				break;

			case 604:
				raw_604(ga, chan_parm, raw_parm);
				break;

			case 605:
				raw_605(ga, chan_parm, raw_parm);
				break;

			case 801:
				raw_801(ga, chan_parm, raw_parm);
				break;

			case 807:
				raw_807(ga, chan_parm);
				break;

			case 808:
				raw_808(ga, chan_parm);
				break;

			case 809:
				raw_809(ga, chan_parm, raw_parm);
				break;

			case 811:
				raw_811(ga, chan_parm, raw_parm);
				break;

			case 812:
				raw_812(ga, chan_parm, raw_parm);
				break;

			case 815:
				raw_815(ga, chan_parm, raw_parm);
				break;

			case 816:
				raw_816(ga, chan_parm, raw_parm);
				break;

			case 817:
				raw_817(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 951:
				raw_951(ga, chan_parm, raw_parm);
				break;

			// nieznany lub niezaimplementowany jeszcze RAW numeryczny
			default:
				raw_unknown = true;
			}

		}	// else if(raw_num)

		// NOTICE ma specjalne znaczenie, bo może również zawierać RAW numeryczny
		else if(raw_parm[1] == "NOTICE")
		{
			// dla RAW NOTICE numerycznych zamień string na int
			raw_numeric = std::stoi("0" + raw_parm[3]);

			if(raw_numeric == 0)
			{
				raw_notice(ga, chan_parm, raw_parm, buffer_irc_raw);
			}

			else if(raw_numeric > 0)
			{
				switch(raw_numeric)
				{
				case 100:
					raw_notice_100(ga, chan_parm, raw_parm, buffer_irc_raw);
					break;

				case 109:
					raw_notice_109(ga, chan_parm, raw_parm, buffer_irc_raw);
					break;

				case 111:
					raw_notice_111(ga, chan_parm, raw_parm, buffer_irc_raw);
					break;

				case 112:
					raw_notice_112(ga, chan_parm, raw_parm);
					break;

				case 121:
					raw_notice_121();
					break;

				case 122:
					raw_notice_122();
					break;

				case 131:
					raw_notice_131();
					break;

				case 132:
					raw_notice_132();
					break;

				case 141:
					raw_notice_141(ga, buffer_irc_raw);
					break;

				case 142:
					raw_notice_142(ga, chan_parm);
					break;

				case 151:
					raw_notice_151(ga, buffer_irc_raw);
					break;

				case 152:
					raw_notice_152(ga, chan_parm, buffer_irc_raw);
					break;

				case 220:
					raw_notice_220(ga, chan_parm, raw_parm);
					break;

				case 221:
					raw_notice_221(ga, chan_parm, raw_parm);
					break;

				case 240:
					raw_notice_240(ga, chan_parm, raw_parm);
					break;

				case 241:
					raw_notice_241(ga, chan_parm, raw_parm);
					break;

				case 255:
					raw_notice_255();
					break;

				case 256:
					raw_notice_256(ga, chan_parm, raw_parm, buffer_irc_raw);
					break;

				case 257:
					raw_notice_257();
					break;

				case 258:
					raw_notice_258(ga, chan_parm, raw_parm);
					break;

				case 260:
					raw_notice_260();
					break;

				case 401:
					raw_notice_401(ga, chan_parm, raw_parm);
					break;

				case 403:
					raw_notice_403(ga, chan_parm, raw_parm);
					break;

				case 404:
					raw_notice_404(ga, chan_parm, raw_parm);
					break;

				case 406:
					raw_notice_406(ga, chan_parm, raw_parm, buffer_irc_raw);
					break;

				case 408:
					raw_notice_408(ga, chan_parm, raw_parm);
					break;

				case 415:
					raw_notice_415(ga, chan_parm, raw_parm);
					break;

				case 416:
					raw_notice_416(ga, chan_parm, raw_parm);
					break;

				case 420:
					raw_notice_420(ga, chan_parm, raw_parm);
					break;

				case 421:
					raw_notice_421(ga, chan_parm, raw_parm);
					break;

				// nieznany lub niezaimplementowany jeszcze RAW NOTICE numeryczny
				default:
					raw_unknown = true;
				}
			}

			// nieznany lub niezaimplementowany jeszcze RAW NOTICE zwykły
			else
			{
				raw_unknown = true;
			}
		}

		// nieznany lub niezaimplementowany jeszcze RAW literowy
		else
		{
			raw_unknown = true;
		}

		// jeśli były nieznane lub niezaimplementowane RAW (każdego typu), to je wyświetl bez zmian w aktywnym kanale
		if(raw_unknown)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
		}

	} while(pos_raw_start < buffer_irc_recv.size());
}


/*
	Poniżej obsługa RAW ERROR i PING, które występują w odpowiedzi serwera są na pierwszej pozycji (w kolejności alfabetycznej).
*/


/*
	ERROR
	ERROR :Closing link (76995189@adei211.neoplus.adsl.tpnet.pl) [Client exited]
	ERROR :Closing link (76995189@adei211.neoplus.adsl.tpnet.pl) [Quit: z/w]
*/
void raw_error(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	ga.irc_ok = false;

	// wyświetl komunikat we wszystkich otwartych pokojach (poza "Debug")
	for(int i = 0; i < CHAN_MAX - 1; ++i)
	{
		if(chan_parm[i])
		{
			chan_parm[i]->nick_parm.clear();

			win_buf_add_str(ga, chan_parm, chan_parm[i]->channel,
					xYELLOW "* " + get_value_from_buf(buffer_irc_raw, " :", "\n"));

			// aktywność typu 1
			chan_act_add(chan_parm, chan_parm[i]->channel, 1);
		}
	}
}


/*
	PING
	PING :cf1f1.onet
*/
void raw_ping(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// odpowiedz PONG na PING
	irc_send(ga, chan_parm, "PONG :" + raw_parm[1]);
}


/*
	Poniżej obsługa RAW z nazwami, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności alfabetycznej).
*/


/*
	INVIGNORE
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVIGNORE ucc_test ^cf1f2753898
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVIGNORE ucc_test #ucc
*/
void raw_invignore(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli zignorowano zaproszenie do rozmowy prywatnej
	if(raw_parm[3].size() > 0 && raw_parm[3][0] == '^')
	{
		win_buf_add_str(ga, chan_parm, raw_parm[3], xRED "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " zignorował(a) Twoje zaproszenie do rozmowy prywatnej.");

		// aktywność typu 1
		chan_act_add(chan_parm, raw_parm[3], 1);
	}

	// jeśli zignorowano zaproszenie do pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " zignorował(a) Twoje zaproszenie do pokoju " + raw_parm[3]);
	}
}


/*
	INVITE
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc INVITE ucc_test :^cf1f1551082
	:ucieszony86!50256503@87edcc.6bc2d5.ee917f.54dae7 INVITE ucc_test :#ucc
*/
void raw_invite(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli to zaproszenie do rozmowy prywatnej
	if(raw_parm[3].size() > 0 && raw_parm[3][0] == '^')
	{
		// informacja w aktywnym pokoju (o ile to nie "Status")
		if(chan_parm[ga.current] && chan_parm[ga.current]->channel != "Status")
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xBOLD_ON xYELLOW_BLACK "* "
					+ get_value_from_buf(buffer_irc_raw, ":", "!") + " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
					+ "] zaprasza Cię do rozmowy prywatnej. Szczegóły w \"Status\" (Alt + 1).");
		}

		// informacja w "Status"
		win_buf_add_str(ga, chan_parm, "Status", xBOLD_ON xYELLOW_BLACK "* "
				+ get_value_from_buf(buffer_irc_raw, ":", "!") + " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
				+ "] zaprasza Cię do rozmowy prywatnej. Aby dołączyć, wpisz /join " + raw_parm[3]);
	}

	// jeśli to zaproszenie do pokoju
	else if(raw_parm[3].size() > 0)
	{
		// informacja w aktywnym pokoju (o ile to nie "Status")
		if(chan_parm[ga.current] && chan_parm[ga.current]->channel != "Status")
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xBOLD_ON xYELLOW_BLACK "* "
					+ get_value_from_buf(buffer_irc_raw, ":", "!") + " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
					+ "] zaprasza Cię do pokoju " + raw_parm[3] + ", szczegóły w \"Status\" (Alt + 1).");
		}

		// informacja w "Status"
		raw_parm[4] = raw_parm[3];	// po /join wytnij #, ale nie wycinaj go w pierwszej części zdania, dlatego użyj innego bufora (wolnego)

		win_buf_add_str(ga, chan_parm, "Status", xBOLD_ON xYELLOW_BLACK "* "
				+ get_value_from_buf(buffer_irc_raw, ":", "!") + " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
				+ "] zaprasza Cię do pokoju " + raw_parm[3] + ", aby wejść, wpisz /join " + raw_parm[4].erase(0, 1));
	}

	// aktywność typu 1 w "Status"
	chan_act_add(chan_parm, "Status", 1);
}


/*
	INVREJECT
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVREJECT ucc_test ^cf1f1123456
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVREJECT ucc_test #ucc
*/
void raw_invreject(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli odrzucono zaproszenie do rozmowy prywatnej
	if(raw_parm[3].size() > 0 && raw_parm[3][0] == '^')
	{
		win_buf_add_str(ga, chan_parm, raw_parm[3], xRED "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " odrzucił(a) Twoje zaproszenie do rozmowy prywatnej.");

		// aktywność typu 1
		chan_act_add(chan_parm, raw_parm[3], 1);
	}

	// jeśli odrzucono zaproszenie do pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " odrzucił(a) Twoje zaproszenie do pokoju " + raw_parm[3]);
	}
}


/*
	JOIN
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c JOIN #ucc :rx,0
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc JOIN ^cf1f1551083 :rx,0
*/
void raw_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli to ja wchodzę, utwórz nowy kanał (jeśli to /join, przechodź od razu do tego okna - ga.command_join)
	if(get_value_from_buf(buffer_irc_raw, ":", "!") == buf_iso2utf(ga.zuousername) && ! new_chan_chat(ga, chan_parm, raw_parm[2], ga.command_join))
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "# Nie udało się utworzyć nowego pokoju (brak pamięci w tablicy pokoi).");
		return;
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	else if(raw_parm[2].size() > 0 && raw_parm[2][0] == '^')
	{
		// jeśli to ja dołączam do rozmowy prywatnej, komunikat będzie inny, niż jeśli to ktoś dołącza
		if(get_value_from_buf(buffer_irc_raw, ":", "!") == buf_iso2utf(ga.zuousername))
		{
			win_buf_add_str(ga, chan_parm, raw_parm[2], xGREEN "* Dołączasz do rozmowy prywatnej.");
		}

		else
		{
			win_buf_add_str(ga, chan_parm, raw_parm[2], xGREEN "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
					+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] dołącza do rozmowy prywatnej.");
		}
	}

	// w przeciwnym razie wyświetl komunikat dla wejścia do pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2], xGREEN "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wchodzi do pokoju " + raw_parm[2]);
	}

	// dodaj nick do listy
	new_or_update_nick_chan(ga, chan_parm, raw_parm[2], get_value_from_buf(buffer_irc_raw, ":", "!"), get_value_from_buf(buffer_irc_raw, "!", " "));

	// dodaj flagi nicka
	struct nick_flags flags = {};
	std::string join_flags = get_value_from_buf(buffer_irc_raw, " :", "\n");

	if(join_flags.find("W") != std::string::npos)
	{
		flags.public_webcam = true;
	}

	if(join_flags.find("V") != std::string::npos)
	{
		flags.private_webcam = true;
	}

	if(join_flags.find("b") != std::string::npos)
	{
		flags.busy = true;
	}

	update_nick_flags_chan(ga, chan_parm, raw_parm[2], get_value_from_buf(buffer_irc_raw, ":", "!"), flags);

	// jeśli nick ma kamerkę, wyświetl o tym informację
	if(flags.public_webcam)
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] ma włączoną publiczną kamerkę.");
	}

	else if(flags.private_webcam)
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] ma włączoną prywatną kamerkę.");
	}

	// skasuj ewentualne użycie /join
	ga.command_join = false;

	// aktywność typu 1
	chan_act_add(chan_parm, raw_parm[2], 1);

	// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
	if(chan_parm[ga.current]->channel == raw_parm[2])
	{
		ga.nicklist_refresh = true;
	}
}


/*
	KICK
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :Zachowuj sie!
*/
void raw_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// pobierz powód, jeśli podano
	std::string reason = get_value_from_buf(buffer_irc_raw, " :", "\n");

	if(reason.size() > 0)
	{
		reason.insert(0, " [Powód: ");
		reason += "]";
	}

	// jeśli to mnie wyrzucono, pokaż inny komunikat
	if(raw_parm[3] == buf_iso2utf(ga.zuousername))
	{
		// usuń kanał z programu
		del_chan_chat(ga, chan_parm, raw_parm[2]);

		// komunikat o wyrzuceniu pokaż w kanale "Status"
		win_buf_add_str(ga, chan_parm, "Status",
				xRED "* Zostajesz wyrzucony(-na) z pokoju " + raw_parm[2] + " przez " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ reason);
	}

	else
	{
		// klucz nicka trzymany jest wielkimi literami
		std::string nick_key = raw_parm[3];

		for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
		{
			if(islower(nick_key[i]))
			{
				nick_key[i] = toupper(nick_key[i]);
			}
		}

		// jeśli nick wszedł po mnie, to jego ZUO jest na liście, wtedy dodaj je do komunikatu
		std::string nick_zuo;

		for(int i = 1; i < CHAN_MAX - 1; ++i)
		{
			if(chan_parm[i] && chan_parm[i]->channel == raw_parm[2])
			{
				auto it = chan_parm[i]->nick_parm.find(nick_key);

				if(it != chan_parm[i]->nick_parm.end())
				{
					nick_zuo = it->second.zuo;
				}

				break;
			}
		}

		if(nick_zuo.size() > 0)
		{
			nick_zuo.insert(0, " [");
			nick_zuo += "]";
		}

		// wyświetl powód wyrzucenia w kanale, w którym wyrzucono nick
		win_buf_add_str(ga, chan_parm, raw_parm[2], xRED "* " + raw_parm[3] + nick_zuo + " zostaje wyrzucony(-na) z pokoju " + raw_parm[2]
				+ " przez " + get_value_from_buf(buffer_irc_raw, ":", "!") + reason);

		// usuń nick z listy
		del_nick_chan(ga, chan_parm, raw_parm[2], raw_parm[3]);

		// aktywność typu 1
		chan_act_add(chan_parm, raw_parm[2], 1);

		// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
		if(chan_parm[ga.current]->channel == raw_parm[2])
		{
			ga.nicklist_refresh = true;
		}
	}
}


/*
	MODE
	:cf1f4.onet MODE ucc_test +b
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc MODE Kernel_Panic :+b
	:zagubiona_miedzy_wierszami!80541395@87edcc.6f9b99.6bd006.aee4fc MODE zagubiona_miedzy_wierszami +W
	:ChanServ!service@service.onet MODE #Towarzyski +b *!12345678@*
	:ChanServ!service@service.onet MODE #Suwałki +b *!*@87edcc.6bc2d5.ee917f.54dae7
	:ChanServ!service@service.onet MODE #ucc +h ucc_test
	:ChanServ!service@service.onet MODE #ucc +qo ucieszony86 ucieszony86
	:cf1f1.onet MODE #Suwałki +oq ucieszony86 ucieszony86
	:ChanServ!service@service.onet MODE #ucc +eh *!76995189@* ucc_test
	:ucc_test!76995189@87edcc.6bc2d5.1917ec.38c71e MODE ucc_test +x
	:NickServ!service@service.onet MODE ucc_test +r
	:Darom!12265854@devel.onet MODE Darom :+O
	:ChanServ!service@service.onet MODE #ucc -b+eh *!76995189@* *!76995189@* ucc_test
	:ChanServ!service@service.onet MODE #ucc -ips
	:GuardServ!service@service.onet MODE #scc -I *!6@*
	:ChanServ!service@service.onet MODE #Towarzyski -m
	:ChanServ!service@service.onet MODE #ucc +c 1
*/
void raw_mode(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw, bool normal_user)
{
	// flagi używane przy wchodzeniu do pokoi, które były otwarte po rozłączeniu, a program nie był zamknięty
	bool my_flag_x = false, my_flag_r = false;

	std::string a, nick_mode, nick_key, chan_join;

	// flaga normal_user jest informacją, że to zwykły użytkownik czata (a nie np. ChanServ) ustawił daną flagę, dlatego wtedy dodaj "(a)" po "ustawił",
	// pojawia się tylko przy niektórych zmianach, dlatego nie wszędzie flaga normal_user jest brana pod uwagę
	if(normal_user)
	{
		a = "(a)";
	}

	// jeśli w raw_parm[2] jest nick, a nie pokój, to go stamtąd pobierz
	if(raw_parm[2].size() > 0 && raw_parm[2][0] != '#')
	{
		nick_mode = raw_parm[2];

		// klucz nicka trzymany jest wielkimi literami
		nick_key = raw_parm[2];

		for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
		{
			if(islower(nick_key[i]))
			{
				nick_key[i] = toupper(nick_key[i]);
			}
		}
	}

	// jeśli zaś to pokój, to sprawdź, czy nick to typowy zapis np. ChanServ!service@service.onet, wtedy pobierz część przed !
	else if(raw_parm[0].find("!") != std::string::npos)
	{
		nick_mode = get_value_from_buf(buffer_irc_raw, ":", "!");
	}

	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	else
	{
		nick_mode = raw_parm[0];
	}

	int s = 0;	// pozycja znaku +/- od początku raw_parm[3]
	int x = -1;	// ile znaków +/- było minus 1

	for(int i = 0; i < static_cast<int>(raw_parm[3].size()); ++i)
	{
		if(raw_parm[3][i] == '+' || raw_parm[3][i] == '-')
		{
			s = i;
			++x;
			continue;	// gdy znaleziono znak + lub -, powróć do początku
		}

/*
	Zmiany flag pokoju.
*/
		// wykrycie bana można rozpoznać po # w trzecim parametrze RAW (i - x + 3 < RAW_PARM_MAX zabezpiecza przed czytaniem poza bufor)
		if(raw_parm[3][i] == 'b' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xRED "* " + raw_parm[i - x + 3]
						+ " otrzymuje bana w pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* " + raw_parm[i - x + 3]
						+ " nie posiada już bana w pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'q' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			// klucz nicka trzymany jest wielkimi literami
			std::string nick_key2 = raw_parm[i - x + 3];

			for(int j = 0; j < static_cast<int>(nick_key2.size()); ++j)
			{
				if(islower(nick_key2[j]))
				{
					nick_key2[j] = toupper(nick_key2[j]);
				}
			}

			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " jest teraz właścicielem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.owner = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " nie jest już właścicielem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.owner = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'o' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			// klucz nicka trzymany jest wielkimi literami
			std::string nick_key2 = raw_parm[i - x + 3];

			for(int j = 0; j < static_cast<int>(nick_key2.size()); ++j)
			{
				if(islower(nick_key2[j]))
				{
					nick_key2[j] = toupper(nick_key2[j]);
				}
			}

			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " jest teraz superoperatorem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.op = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " nie jest już superoperatorem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.op = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'h' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			// klucz nicka trzymany jest wielkimi literami
			std::string nick_key2 = raw_parm[i - x + 3];

			for(int j = 0; j < static_cast<int>(nick_key2.size()); ++j)
			{
				if(islower(nick_key2[j]))
				{
					nick_key2[j] = toupper(nick_key2[j]);
				}
			}

			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " jest teraz operatorem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.halfop = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " nie jest już operatorem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.halfop = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'e' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " posiada teraz wyjątek od bana w pokoju " + raw_parm[2] + " (ustawił " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " nie posiada już wyjątku od bana w pokoju " + raw_parm[2] + " (ustawił " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'v' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			// klucz nicka trzymany jest wielkimi literami
			std::string nick_key2 = raw_parm[i - x + 3];

			for(int j = 0; j < static_cast<int>(nick_key2.size()); ++j)
			{
				if(islower(nick_key2[j]))
				{
					nick_key2[j] = toupper(nick_key2[j]);
				}
			}

			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xBLUE "* " + raw_parm[i - x + 3]
						+ " jest teraz gościem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.voice = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xBLUE "* " + raw_parm[i - x + 3]
						+ " nie jest już gościem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.voice = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'X' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			// klucz nicka trzymany jest wielkimi literami
			std::string nick_key2 = raw_parm[i - x + 3];

			for(int j = 0; j < static_cast<int>(nick_key2.size()); ++j)
			{
				if(islower(nick_key2[j]))
				{
					nick_key2[j] = toupper(nick_key2[j]);
				}
			}

			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " jest teraz moderatorem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.moderator = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i - x + 3]
						+ " nie jest już moderatorem pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");

				// zaktualizuj flagę
				for(int j = 1; j < CHAN_MAX - 1; j++)
				{
					if(chan_parm[j] && chan_parm[j]->channel == raw_parm[2])
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key2);

						if(it != chan_parm[j]->nick_parm.end())
						{
							it->second.flags.moderator = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}

						break;
					}
				}
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'm' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#')
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " jest teraz moderowany (ustawił" + a + " " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " nie jest już moderowany (ustawił" + a + " " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'I' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#' && i - x + 3 < RAW_PARM_MAX)
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* " + raw_parm[i - x + 3]
						+ " jest teraz na liście zaproszonych w pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* " + raw_parm[i - x + 3]
						+ " nie jest już na liście zaproszonych w pokoju " + raw_parm[2] + " (ustawił" + a + " " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'i' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#')
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " jest teraz ukryty (ustawił " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " nie jest już ukryty (ustawił " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 'p' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#')
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " jest teraz prywatny (ustawił " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " nie jest już prywatny (ustawił " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

		else if(raw_parm[3][i] == 's' && raw_parm[2].size() > 0 && raw_parm[2][0] == '#')
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " jest teraz sekretny (ustawił " + nick_mode + ").");
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2],
						xMAGENTA "* Pokój " + raw_parm[2] + " nie jest już sekretny (ustawił " + nick_mode + ").");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}

/*
	Zmiany flag osób.
*/
		else if(raw_parm[3][i] == 'O' && raw_parm[2].size() > 0 && raw_parm[2][0] != '#')
		{
			if(raw_parm[3][s] == '+')
			{
				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
								xMAGENTA "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
								+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
								+ "] jest teraz administratorem czata.");

						// aktywność typu 1
						chan_act_add(chan_parm, chan_parm[j]->channel, 1);
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
								xMAGENTA "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
								+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
								+ "] nie jest już administratorem czata.");

						// aktywność typu 1
						chan_act_add(chan_parm, chan_parm[j]->channel, 1);
					}
				}
			}
		}

		else if(raw_parm[3][i] == 'W' && raw_parm[2].size() > 0 && raw_parm[2][0] != '#')
		{
			if(raw_parm[3][s] == '+')
			{
				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
								xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
								+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] włącza publiczną kamerkę.");

						auto it = chan_parm[j]->nick_parm.find(nick_key);

						// zaktualizuj flagę
						it->second.flags.public_webcam = true;

						// ze względu na obecny brak zmiany flagi V (brak MODE) należy ją wyzerować po włączeniu W
						it->second.flags.private_webcam = false;

						// aktywność typu 1
						chan_act_add(chan_parm, chan_parm[j]->channel, 1);

						// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
						if(j == ga.current)
						{
							ga.nicklist_refresh = true;
						}
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
								xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
								+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wyłącza publiczną kamerkę.");

						auto it = chan_parm[j]->nick_parm.find(nick_key);

						// zaktualizuj flagę
						it->second.flags.public_webcam = false;

						// aktywność typu 1
						chan_act_add(chan_parm, chan_parm[j]->channel, 1);

						// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
						if(j == ga.current)
						{
							ga.nicklist_refresh = true;
						}
					}
				}
			}
		}

		else if(raw_parm[3][i] == 'V' && raw_parm[2].size() > 0 && raw_parm[2][0] != '#')
		{
			if(raw_parm[3][s] == '+')
			{
				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
								xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
								+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] włącza prywatną kamerkę.");

						auto it = chan_parm[j]->nick_parm.find(nick_key);

						// zaktualizuj flagę
						it->second.flags.public_webcam = false;

						// aktywność typu 1
						chan_act_add(chan_parm, chan_parm[j]->channel, 1);

						// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
						if(j == ga.current)
						{
							ga.nicklist_refresh = true;
						}
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
								xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
								+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wyłącza prywatną kamerkę.");

						auto it = chan_parm[j]->nick_parm.find(nick_key);

						// zaktualizuj flagę
						it->second.flags.public_webcam = false;

						// aktywność typu 1
						chan_act_add(chan_parm, chan_parm[j]->channel, 1);

						// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
						if(j == ga.current)
						{
							ga.nicklist_refresh = true;
						}
					}
				}
			}
		}

		// pokazuj tylko moje szyfrowanie IP (w pokoju "Status")
		else if(raw_parm[3][i] == 'x' && raw_parm[2] == buf_iso2utf(ga.zuousername))
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, "Status", xGREEN "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
						+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] - masz teraz szyfrowane IP.");

				my_flag_x = true;
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, "Status", xGREEN "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
						+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] - nie masz już szyfrowanego IP.");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, "Status", 1);
		}

		// pokazuj tylko mój zarejestrowany nick (w pokoju "Status")
		else if(raw_parm[3][i] == 'r' && raw_parm[2] == buf_iso2utf(ga.zuousername))
		{
			if(raw_parm[3][s] == '+')
			{
				win_buf_add_str(ga, chan_parm, "Status", xGREEN "* Jesteś teraz zarejestrowanym użytkownikiem.");

				my_flag_r = true;
			}

			else if(raw_parm[3][s] == '-')
			{
				win_buf_add_str(ga, chan_parm, "Status", xGREEN "* Nie jesteś już zarejestrowanym użytkownikiem.");
			}

			// aktywność typu 1
			chan_act_add(chan_parm, "Status", 1);
		}

		// aktualizacja flagi busy na liście nicków
		else if(raw_parm[3][i] == 'b')
		{
			if(raw_parm[3][s] == '+')
			{
				// pokaż zmianę we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key);
						it->second.flags.busy = true;

						// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
						if(j == ga.current)
						{
							ga.nicklist_refresh = true;
						}
					}
				}
			}

			else if(raw_parm[3][s] == '-')
			{
				// pokaż zmianę we wszystkich pokojach, gdzie jest dany nick
				for(int j = 1; j < CHAN_MAX - 1; ++j)
				{
					if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_key) != chan_parm[j]->nick_parm.end())
					{
						auto it = chan_parm[j]->nick_parm.find(nick_key);
						it->second.flags.busy = false;

						// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
						if(j == ga.current)
						{
							ga.nicklist_refresh = true;
						}
					}
				}
			}
		}

		// niezaimplementowane RAW z MODE wyświetl bez z mian
		else
		{
			if(raw_parm[3][i])
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
						xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}
		}

	}	// for()

	// jeśli wylogowaliśmy się, ale nie zamknęliśmy programu i były otwarte jakieś pokoje, wejdź do nich ponownie po zalogowaniu
	// - po +r dla nicka zarejestrowanego
	// - po +x dla nicka tymczasowego
	if(my_flag_r || (my_flag_x && ga.zuousername.size() > 0 && ga.zuousername[0] == '~'))
	{
		// test - znajdź plik vhost, jeśli jest, wczytaj go i wyślij na serwer, aby zmienić swój host (do poprawy jeszcze)
		std::ifstream vhost_file;
		vhost_file.open("/tmp/vhost.txt");

		if(vhost_file.good())
		{
			// uwaga - nie jest sprawdzana poprawność zapisu pliku (jedynie, że coś jest w nim), zakłada się, że plik zawiera login i hasło
			// (do poprawy)
			std::string vhost_str;
			std::getline(vhost_file, vhost_str);

			if(vhost_str.size() > 0)
			{
				win_buf_add_str(ga, chan_parm, "Status", xWHITE "# Znaleziono plik VHost, wczytuję...");
				irc_send(ga, chan_parm, "VHOST " + vhost_str);
			}

			vhost_file.close();
		}

		for(int i = 1; i < CHAN_MAX - 1; ++i)	// od 1, bo pomijamy "Status" oraz - 1, bo pomijamy "Debug"
		{
			if(chan_parm[i] && chan_parm[i]->channel.size() > 0)
			{
				// pierwszy kanał bez przecinka
				if(chan_join.size() == 0)
				{
					chan_join = chan_parm[i]->channel;
				}

				// do pozostałych dopisz przecinek za wcześniejszym pokojem
				else
				{
					chan_join += "," + chan_parm[i]->channel;
				}
			}
		}
	}

	if(chan_join.size() > 0)
	{
		irc_send(ga, chan_parm, "JOIN " + buf_utf2iso(chan_join));
	}
}


/*
	MODERMSG
	:M_X!36866915@d2d929.646f7c.4180a1.bd429d MODERMSG ucc_test - #Towarzyski :Moderowany?
	:M_X!36866915@d2d929.646f7c.4180a1.bd429d MODERMSG Karolinaaa1 - #Towarzyski :%Fb%%C0f2ab1%u2u2 nie spimy %Ituba%
*/
void raw_modermsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string form_start;

	std::string modermsg = get_value_from_buf(buffer_irc_raw, " :", "\n");

	size_t nick_call = modermsg.find(buf_iso2utf(ga.zuousername));

	// jeśli ktoś mnie woła, jego nick wyświetl w żółtym kolorze
	if(nick_call != std::string::npos)
	{
		form_start = xYELLOW;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		chan_act_add(chan_parm, raw_parm[4], 3);

		// testowa wersja dźwięku, do poprawy jeszcze
		if(system("aplay -q /usr/share/sounds/pop.wav 2>/dev/null &") != 0)
		{
			// coś
		}
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		chan_act_add(chan_parm, raw_parm[4], 2);
	}

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string action_me = get_value_from_buf(buffer_irc_raw, " :\x01" "ACTION ", "\x01\n");

	// jeśli tak, wyświetl inaczej wiadomość
	if(action_me.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm[4], xMAGENTA "* " + form_start + raw_parm[2] + xNORMAL " " + action_me
				+ " " xRED "[Moderowany przez " + get_value_from_buf(buffer_irc_raw, ":", "!") + "]");
	}

	// a jeśli nie było /me wyświetl wiadomość w normalny sposób
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm[4], xCYAN + form_start + "<" + raw_parm[2] + ">" + xNORMAL " " + modermsg
				+ " " xRED "[Moderowany przez " + get_value_from_buf(buffer_irc_raw, ":", "!") + "]");
	}
}


/*
	PART
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc :Bye
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f3561508
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f1552723 :Koniec rozmowy
*/
void raw_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// pobierz powód, jeśli podano
	std::string reason = get_value_from_buf(buffer_irc_raw, " :", "\n");

	if(reason.size() > 0)
	{
		reason.insert(0, " [Powód: ");
		reason += "]";
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	if(raw_parm[2].size() > 0 && raw_parm[2][0] == '^')
	{
		if(reason.size() == 0)
		{
			reason = ".";
		}

		//jeśli to ja opuszczam rozmowę prywatną, komunikat będzie inny, niż jeśli to ktoś opuszcza
		if(get_value_from_buf(buffer_irc_raw, ":", "!") == buf_iso2utf(ga.zuousername))
		{
			win_buf_add_str(ga, chan_parm, raw_parm[2], xCYAN "* Opuszczasz rozmowę prywatną" + reason);
		}

		else
		{
			win_buf_add_str(ga, chan_parm, raw_parm[2], xCYAN "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
					+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] opuszcza rozmowę prywatną" + reason);
		}
	}

	// w przeciwnym razie wyświetl komunikat dla wyjścia z pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2], xCYAN "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
				+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wychodzi z pokoju " + raw_parm[2] + reason);
	}

	// jeśli to ja wychodzę, usuń kanał z programu
	if(get_value_from_buf(buffer_irc_raw, ":", "!") == buf_iso2utf(ga.zuousername))
	{
		del_chan_chat(ga, chan_parm, raw_parm[2]);
	}

	else
	{
		// usuń nick z listy
		del_nick_chan(ga, chan_parm, raw_parm[2], get_value_from_buf(buffer_irc_raw, ":", "!"));

		// aktywność typu 1
		chan_act_add(chan_parm, raw_parm[2], 1);

		// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
		if(chan_parm[ga.current]->channel == raw_parm[2])
		{
			ga.nicklist_refresh = true;
		}
	}
}


/*
	PONG (odpowiedź serwera na wysłany PING)
	:cf1f1.onet PONG cf1f1.onet :1234
*/
void raw_pong(struct global_args &ga)
{
	struct timeval t_pong;

	gettimeofday(&t_pong, NULL);
	ga.pong = t_pong.tv_sec * 1000;
	ga.pong += t_pong.tv_usec / 1000;

	ga.lag = ga.pong - ga.ping;

	ga.lag_timeout = false;
}


/*
	PRIVMSG
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 PRIVMSG #ucc :Hello.
	:Kernel_Panic!78259658@87edcc.6bc2d5.1917ec.38c71e PRIVMSG #ucc :\1ACTION %Cff0000%widzi co się dzieje.\1
*/
void raw_privmsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string form_start;
	size_t nick_call = buffer_irc_raw.find(buf_iso2utf(ga.zuousername));

	// jeśli ktoś mnie woła, pogrub jego nick i wyświetl w żółtym kolorze
	if(nick_call != std::string::npos)
	{
		form_start = xBOLD_ON xYELLOW_BLACK;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		chan_act_add(chan_parm, raw_parm[2], 3);

		// testowa wersja dźwięku, do poprawy jeszcze
		if(system("aplay -q /usr/share/sounds/pop.wav 2>/dev/null &") != 0)
		{
			// coś
		}
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		chan_act_add(chan_parm, raw_parm[2], 2);
	}

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string action_me = get_value_from_buf(buffer_irc_raw, " :\x01" "ACTION ", "\x01\n");

	// jeśli tak, wyświetl inaczej wiadomość
	if(action_me.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2],
				xBOLD_ON xMAGENTA "* " + form_start + get_value_from_buf(buffer_irc_raw, ":", "!") + xNORMAL " " + action_me);
	}

	// a jeśli nie było /me wyświetl wiadomość w normalny sposób
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2], form_start + "<" + get_value_from_buf(buffer_irc_raw, ":", "!") + ">"
				+ xNORMAL " " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
	}
}


/*
	QUIT
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Client exited
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Quit: Do potem
*/
void raw_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	// klucz nicka trzymany jest wielkimi literami
	std::string nick_key = get_value_from_buf(buffer_irc_raw, ":", "!");

	for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
	{
		if(islower(nick_key[i]))
		{
			nick_key[i] = toupper(nick_key[i]);
		}
	}

	// usuń nick ze wszystkich pokoi z listy, gdzie przebywał i wyświetl w tych pokojach komunikat o tym
	for(int i = 1; i < CHAN_MAX - 1; ++i)	// i = 1 oraz i < CHAN_MAX - 1, bo do "Status" oraz "Debug" nie byli wrzucani użytkownicy
	{
		// usuwać można tylko w otwartych pokojach oraz nie usuwaj nicka, jeśli takiego nie było w pokoju
		if(chan_parm[i] && chan_parm[i]->nick_parm.find(nick_key) != chan_parm[i]->nick_parm.end())
		{
			// usuń nick z listy
			chan_parm[i]->nick_parm.erase(nick_key);

			// w pokoju, w którym był nick wyświetl komunikat o jego wyjściu
			win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, xYELLOW "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
					+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wychodzi z czata ["
					+ get_value_from_buf(buffer_irc_raw, " :", "\n") + "]");

			// aktywność typu 1
			chan_act_add(chan_parm, chan_parm[i]->channel, 1);

			// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
			if(i == ga.current)
			{
				ga.nicklist_refresh = true;
			}
		}
	}
}


/*
	TOPIC
	:ucc_test!76995189@87edcc.30c29e.b9c507.d5c6b7 TOPIC #ucc :nowy temat
*/
void raw_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
			+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] zmienił(a) temat pokoju " + raw_parm[2]);

	// wpisz temat również do bufora tematu kanału, aby wyświetlić go na górnym pasku (reszta jest identyczna jak w obsłudze RAW 332,
	// trzeba tylko przestawić parametry w raw_parm)
	raw_parm[3] = raw_parm[2];	// przesuń nazwę pokoju, reszta parametrów w raw_parm dla raw_332() nie jest istotna
	raw_332(ga, chan_parm, raw_parm, buffer_irc_raw);

	// aktywność typu 1
	chan_act_add(chan_parm, raw_parm[2], 1);
}


/*
	Poniżej obsługa RAW z liczbami, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności numerycznej).
*/


/*
	001
	:cf1f2.onet 001 ucc_test :Welcome to the OnetCzat IRC Network ucc_test!76995189@acvy210.neoplus.adsl.tpnet.pl
*/
void raw_001(struct global_args &ga)
{
	ga.busy_state = false;
}


/*
	002
	:cf1f2.onet 002 ucc_test :Your host is cf1f2.onet, running version InspIRCd-1.1
*/
void raw_002()
{
}


/*
	003
	:cf1f2.onet 003 ucc_test :This server was created 02:35:04 Feb 16 2011
*/
void raw_003()
{
}


/*
	004
	:cf1f2.onet 004 ucc_test cf1f2.onet InspIRCd-1.1 BOQRVWbinoqrswx DFIKMRVXYabcehiklmnopqrstuv FIXYabcehkloqv
*/
void raw_004()
{
}


/*
	005
	:cf1f2.onet 005 ucc_test WALLCHOPS WALLVOICES MODES=19 CHANTYPES=^# PREFIX=(qaohXYv)`&@%!=+ MAP MAXCHANNELS=20 MAXBANS=60 VBANLIST NICKLEN=32 CASEMAPPING=rfc1459 STATUSMSG=@%+ CHARSET=ascii :are supported by this server
	:cf1f2.onet 005 ucc_test TOPICLEN=203 KICKLEN=255 MAXTARGETS=20 AWAYLEN=200 CHANMODES=Ibe,k,Fcl,DKMRVimnprstu FNC NETWORK=OnetCzat MAXPARA=32 ELIST=MU OVERRIDE ONETNAMESX INVEX=I EXCEPTS=e :are supported by this server
	:cf1f2.onet 005 ucc_test INVIGNORE=100 USERIP ESILENCE SILENCE=100 WATCH=200 NAMESX :are supported by this server
*/
void raw_005()
{
}


/*
	251 (LUSERS)
	:cf1f2.onet 251 ucc_test :There are 1933 users and 5 invisible on 10 servers
*/
void raw_251()
{
}


/*
	252 (LUSERS)
	:cf1f2.onet 252 ucc_test 8 :operator(s) online
*/
void raw_252()
{
}


/*
	253
	:cf1f1.onet 253 ucc_test 1 :unknown connections
*/
void raw_253()
{
}


/*
	254 (LUSERS)
	:cf1f2.onet 254 ucc_test 2422 :channels formed
*/
void raw_254()
{
}


/*
	255 (LUSERS)
	:cf1f2.onet 255 ucc_test :I have 478 clients and 1 servers
*/
void raw_255()
{
}


/*
	256 (ADMIN)
	:cf1f3.onet 256 ucc_test :Administrative info for cf1f3.onet
*/
void raw_256(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm[0] + xNORMAL "- " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	257 (ADMIN)
	:cf1f3.onet 257 ucc_test :Name     - Czat Admin
*/
void raw_257(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm[0] + xNORMAL "- " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	258 (ADMIN)
	:cf1f3.onet 258 ucc_test :Nickname - czat_admin
*/
void raw_258(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm[0] + xNORMAL "- " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	259 (ADMIN)
	:cf1f3.onet 259 ucc_test :E-Mail   - czat_admin@czat.onet.pl
*/
void raw_259(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm[0] + xNORMAL "- " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	265 (LUSERS)
	:cf1f2.onet 265 ucc_test :Current Local Users: 478  Max: 619
*/
void raw_265()
{
}


/*
	266 (LUSERS)
	:cf1f2.onet 266 ucc_test :Current Global Users: 1938  Max: 2487
*/
void raw_266()
{
}


/*
	301 (WHOIS - away)
	:cf1f2.onet 301 ucc_test ucc_test :test away
*/
void raw_301(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest nieobecny(-na) z powodu: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	303 (ISON - zwraca nick, gdy jest online)
	:cf1f1.onet 303 ucc_test :Kernel_Panic
*/
void raw_303(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* Nicki dostępne dla zapytania ISON: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	304
	:cf1f3.onet 304 ucc_test :SYNTAX PRIVMSG <target>{,<target>} <message>
	:cf1f4.onet 304 ucc_test :SYNTAX NICK <newnick>
*/
void raw_304(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	305 (AWAY - bez wiadomości wyłącza)
	:cf1f1.onet 305 ucc_test :You are no longer marked as being away
*/
void raw_305(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Nie jesteś już oznaczony(-na) jako nieobecny(-na).");
}


/*
	306 (AWAY - z wiadomością włącza)
	:cf1f1.onet 306 ucc_test :You have been marked as being away
*/
void raw_306(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Jesteś teraz oznaczony(-na) jako nieobecny(-na).");
}


/*
	307 (WHOIS)
	:cf1f1.onet 307 ucc_test ucc_test :is a registered nick
*/
void raw_307(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest zarejestrowanym użytkownikiem.");
}


/*
	311 (początek WHOIS, gdy nick jest na czacie)
	:cf1f2.onet 311 ucc_test AT89S8253 70914256 aaa2a7.a7f7a6.88308b.464974 * :tururu!
	:cf1f1.onet 311 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
	:cf1f2.onet 311 ucc_test ucc_test 76995189 87edcc.30c29e.611774.3140a3 * :ucc_test
*/
void raw_311(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// RAW 311 zapoczątkowuje wynik WHOIS
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xGREEN + raw_parm[3] + xTERMC " [" + raw_parm[4] + "@" + raw_parm[5] + "]\n"
			+ "* " xGREEN + raw_parm[3] + xNORMAL " ircname: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	312 (WHOIS)
	:cf1f2.onet 312 ucc_test AT89S8253 cf1f3.onet :cf1f3
	312 (WHOWAS)
	:cf1f2.onet 312 ucc_test ucieszony86 cf1f2.onet :Mon May 26 21:39:35 2014
*/
void raw_312(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// mało elegancki sposób na odróżnienie WHOIS od WHOWAS (we WHOWAS są spacje po wyrażeniu z drugim dwukropkiem)
	std::string whowas = get_value_from_buf(buffer_irc_raw, " :", "\n");

	size_t whois_whowas_type = whowas.find(" ");

	// jeśli WHOIS
	if(whois_whowas_type == std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xGREEN + raw_parm[3] + xNORMAL " używa serwera: " + raw_parm[4] + " [" + raw_parm[5] + "]");
	}

	// jeśli WHOWAS
	else
	{
		// przekształć zwracaną datę i godzinę na formę poprawną dla polskiego zapisu
		std::string time_parm[RAW_PARM_MAX];

		whowas += "\n";		// + "\n", aby zwróciło ostatni znak w buforze
		get_raw_parm(whowas, time_parm);

		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xCYAN + raw_parm[3] + xNORMAL " używał(a) serwera: " + raw_parm[4] + "\n"
				+ "* " xCYAN + raw_parm[3] + xNORMAL " był(a) zalogowany(-na) od: " + dayen2daypl(time_parm[0]) + ", " + time_parm[2]
				+ " " + monthen2monthpl(time_parm[1]) + " " + time_parm[4] + ", " + time_parm[3]);
	}
}


/*
	314 (początek WHOWAS, gdy nick był na czacie)
	:cf1f3.onet 314 ucc_test ucieszony86 50256503 e0c697.bbe735.1b1f7f.56f6ee * :ucieszony86
	:cf1f3.onet 314 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
*/
void raw_314(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xCYAN + raw_parm[3] + xTERMC " [" + raw_parm[4] + "@" + raw_parm[5] + "]\n"
			+ "* " xCYAN + raw_parm[3] + xNORMAL + " ircname: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	317 (WHOIS)
	:cf1f1.onet 317 ucc_test AT89S8253 532 1400636388 :seconds idle, signon time
*/
void raw_317(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
        win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest nieaktywny(-na) przez: " + time_sec2time(raw_parm[4]) + "\n"
			+ "* " xGREEN + raw_parm[3] + xNORMAL " jest zalogowany(-na) od: " + time_unixtimestamp2local_full(raw_parm[5]));
}


/*
	318 (koniec WHOIS)
	:cf1f1.onet 318 ucc_test AT89S8253 :End of /WHOIS list.
*/
void raw_318()
{
}


/*
	319 (WHOIS)
	:cf1f2.onet 319 ucc_test AT89S8253 :@#Learning_English %#Linux %#zua_zuy_zuo @#Computers @#Augustów %#ateizm #scc @#PHP @#ucc @#Suwałki
*/
void raw_319(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest w pokojach: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	332 (temat pokoju)
	:cf1f3.onet 332 ucc_test #ucc :Ucieszony Chat Client
*/
void raw_332(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// najpierw wyświetl temat (tylko, gdy jest ustawiony lub informację o braku tematu)
	std::string topic_tmp = get_value_from_buf(buffer_irc_raw, " :", "\n");

	if(topic_tmp.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm[3], xWHITE "* Temat pokoju " + raw_parm[3] + " - " + topic_tmp);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm[3], xWHITE "* Temat pokoju " + raw_parm[3] + " nie został ustawiony (jest pusty).");
	}

	// teraz znajdź pokój, do którego należy temat, wpisz go do jego bufora "topic" i wyświetl na górnym pasku
	int topic_chan = -1;

	for(int i = 1; i < CHAN_MAX - 1; ++i)	// 1 i - 1, bo pomijamy "Status" i "Debug"
	{
		if(chan_parm[i] && chan_parm[i]->channel == raw_parm[3])	// znajdź pokój, do którego należy temat
		{
			topic_chan = i;
		}
	}

	// jeśli nie znaleziono pokoju, zakończ
	if(topic_chan == -1)
	{
		return;
	}

	std::string topic_tmp1 = get_value_from_buf(buffer_irc_raw, " :", "\n");
	std::string topic_tmp2;

	// usuń z tematu formatowanie fontu i kolorów (na pasku nie jest obsługiwane)
	for(int i = 0; i < static_cast<int>(topic_tmp1.size()); ++i)
	{
		if(topic_tmp1[i] == dCOLOR)
		{
			++i;
			continue;
		}

		else if(topic_tmp1[i] == dBOLD_ON || topic_tmp1[i] == dBOLD_OFF || topic_tmp1[i] == dREVERSE_ON || topic_tmp1[i] == dREVERSE_OFF
			|| topic_tmp1[i] == dUNDERLINE_ON || topic_tmp1[i] == dUNDERLINE_OFF || topic_tmp1[i] == dNORMAL)
		{
                        continue;
		}

		topic_tmp2 += topic_tmp1[i];
	}

	chan_parm[topic_chan]->topic = topic_tmp2;
}


/*
	333 (kto i kiedy ustawił temat)
	:cf1f4.onet 333 ucc_test #ucc ucc_test!76995189@87edcc.30c29e.b9c507.d5c6b7 1400889719
*/
void raw_333(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, raw_parm[3],
			xWHITE "* Temat pokoju " + raw_parm[3] + " ustawiony przez " + get_value_from_buf(buffer_irc_raw, raw_parm[3] + " ", "!")
			+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] (" + time_unixtimestamp2local_full(raw_parm[5]) + ").");
}


/*
	341 (INVITE - bez informowania, bo podobną informację zwraca RAW NOTICE)
	:cf1f3.onet 341 ucc_test Kernel_Panic #ucc
*/
void raw_341()
{
}


/*
	353 (NAMES)
	:cf1f1.onet 353 ucc_test = #scc :%ucc_test|rx,0 AT89S8253|brx,0 %Husar|rx,1 ~Ayindida|x,0 YouTube_Dekoder|rx,0 StyX1|rx,0 %Radowsky|rx,1 fml|rx,0
*/
void raw_353(struct global_args &ga, std::string &buffer_irc_raw)
{
	ga.names += get_value_from_buf(buffer_irc_raw, " :", "\n");
}


/*
	366 (koniec NAMES)
	:cf1f4.onet 366 ucc_test #ucc :End of /NAMES list.
*/
void raw_366(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	std::string nick;

	struct nick_flags flags = {};

	for(int i = 0; i < static_cast<int>(ga.names.size()); ++i)
	{
		if(ga.names[i] == ' ')
		{
			// pobierz statusy nicka
			for(int j = 0; j < static_cast<int>(nick.size()); ++j)
			{
				// pobierz statusy
				if(nick[j] == '`')
				{
					flags.owner = true;
				}

				else if(nick[j] == '@')
				{
					flags.op = true;
				}

				else if(nick[j] == '%')
				{
					flags.halfop = true;
				}

				else if(nick[j] == '!')
				{
					flags.moderator = true;
				}

				else if(nick[j] == '+')
				{
					flags.voice = true;
				}

				// gdy koniec, usuń je z nicka
				else
				{
					nick.erase(0, j);

					// można przerwać
					break;
				}
			}

			// pobierz pozostałe flagi (wybrane)
			size_t rest_flags = nick.find("|");

			if(rest_flags != std::string::npos)
			{
				if(nick.find("W", rest_flags) != std::string::npos)
				{
					flags.public_webcam = true;
				}

				if(nick.find("V", rest_flags) != std::string::npos)
				{
					flags.private_webcam = true;
				}

				if(nick.find("b", rest_flags) != std::string::npos)
				{
					flags.busy = true;
				}

				// po pobraniu reszty flag usuń je z nicka
				nick.erase(rest_flags, nick.size() - rest_flags);
			}

			// wpisz nick na listę
			new_or_update_nick_chan(ga, chan_parm, raw_parm[3], nick, "");

			// wpisz flagi nicka
			update_nick_flags_chan(ga, chan_parm, raw_parm[3], nick, flags);

			// po wczytaniu nicka wyczyść bufory
			nick.clear();
			flags = {};
		}

		else
		{
			nick += ga.names[i];
		}
	}

	// odśwież listę (o ile zmiana dotyczyła aktualnie otwartego pokoju)
	if(chan_parm[ga.current]->channel == raw_parm[3])
	{
		ga.nicklist_refresh = true;
	}

	// po wyświetleniu nicków wyczyść bufor, aby kolejne użycie nie wyświetliło starej zawartości
	ga.names.clear();

	// skasuj ewentualne użycie /names
	ga.command_names = false;
}


/*
	369 (koniec WHOWAS)
	:cf1f3.onet 369 ucc_test devi85 :End of WHOWAS
*/
void raw_369()
{
}


/*
	372 (MOTD - wiadomość dnia, właściwa wiadomość)
	:cf1f2.onet 372 ucc_test :- Onet Czat. Inny Wymiar Czatowania. Witamy!
	:cf1f2.onet 372 ucc_test :- UWAGA - Nie daj się oszukać! Zanim wyślesz jakikolwiek SMS, zapoznaj się z filmem: http://www.youtube.com/watch?v=4skUNAyIN_c
	:cf1f2.onet 372 ucc_test :-
*/
void raw_372(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, "Status", xYELLOW + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	375 (MOTD - wiadomość dnia, początek)
	:cf1f2.onet 375 ucc_test :cf1f2.onet message of the day
*/
void raw_375(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, "Status", xYELLOW "* Wiadomość dnia:");
}


/*
	376(MOTD - wiadomość dnia, koniec)
	:cf1f2.onet 376 ucc_test :End of message of the day.
*/
void raw_376(struct channel_irc *chan_parm[])
{
	// aktywność typu 1
	chan_act_add(chan_parm, "Status", 1);
}


/*
	378 (WHOIS)
	:cf1f1.onet 378 ucc_test ucc_test :is connecting from 76995189@adin155.neoplus.adsl.tpnet.pl 79.184.195.155
*/
void raw_378(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest połączony(-na) z: " + get_value_from_buf(buffer_irc_raw, "from ", "\n"));
}


/*
	391 (TIME)
	:cf1f4.onet 391 ucc_test cf1f4.onet :Tue May 27 08:59:41 2014
*/
void raw_391(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// przekształć zwracaną datę i godzinę na formę poprawną dla polskiego zapisu
	std::string time_serv = get_value_from_buf(buffer_irc_raw, " :", "\n");

	std::string time_parm[RAW_PARM_MAX];

	time_serv += "\n";		// + "\n", aby zwróciło ostatni znak w buforze
	get_raw_parm(time_serv, time_parm);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xYELLOW "* Data i czas na serwerze " + raw_parm[3] + " - "
			+ dayen2daypl(time_parm[0]) + ", " + time_parm[2] + " "	+ monthen2monthpl(time_parm[1]) + " " + time_parm[4] + ", " + time_parm[3]);
}


/*
	396
	:cf1f2.onet 396 ucc_test 87edcc.6bc2d5.1917ec.38c71e :is now your displayed host
*/
void raw_396(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// wyświetl w pokoju "Status" jeśli to odpowiedź po zalogowaniu
	if(! ga.command_vhost)
	{
		win_buf_add_str(ga, chan_parm, "Status", xGREEN "* Twoim wyświetlanym hostem jest teraz " + raw_parm[3]);
	}

	// wyświetl w aktualnym pokoju, jeśli to odpowiedź po użyciu /vhost
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xGREEN "* Twoim wyświetlanym hostem jest teraz " + raw_parm[3]);
	}

	// skasuj ewentualne użycie /vhost
	ga.command_vhost = false;
}



/*
	401 (początek WHOIS, gdy nick/kanał nie został znaleziony, używany również przy podaniu nieprawidłowego pokoju, np. dla TOPIC, NAMES itd.)
	:cf1f4.onet 401 ucc_test abc :No such nick/channel
	:cf1f4.onet 401 ucc_test #ucc: :No such nick/channel
*/
void raw_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// jeśli nick
	if(raw_parm[3].size() > 0 && raw_parm[3][0] != '#')
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON xGREEN + raw_parm[3] + xNORMAL " - nie ma takiego nicka na czacie.");
	}

	// jeśli pokój (niestety przez sposób, w jaki serwer zwraca ten RAW informacja dla WHOIS #pokój również zwraca informację o braku pokoju,
	// co może być mylące)
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[3] + " - nie ma takiego pokoju.");
	}
}


/*
	402 (MOTD nieznany_serwer)
	:cf1f4.onet 402 ucc_test abc :No such server
*/
void raw_402(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[3] + " - nie ma takiego serwera.");
}


/*
	403
	:cf1f4.onet 403 ucc_test sc :Invalid channel name
	:cf1f2.onet 403 ucc_test ^cf1f2123456 :Invalid channel name
*/
void raw_403(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[3] + " - nieprawidłowa nazwa pokoju.");

	// dopisać, aby nieudane dołączenie do rozmowy prywatnej dawało komunikat "<nick> zrezygnował(a) z rozmowy prywatnej."
}


/*
	404 (próba wysłania wiadomości do kanału, w którym nie przebywamy)
	:cf1f1.onet 404 ucc_test #ucc :Cannot send to channel (no external messages)
	404 (próba wysłania wiadomości w pokoju moderowanym, gdzie nie mamy uprawnień)
	:cf1f2.onet 404 ucc_test #Suwałki :Cannot send to channel (+m)
*/
void raw_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	if(buffer_irc_raw.find("Cannot send to channel (no external messages)") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "* Nie można wysłać wiadomości do pokoju " + raw_parm[3] + " (nie przebywasz w nim).");
	}

	else if(buffer_irc_raw.find("Cannot send to channel (+m)") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "* Nie możesz pisać w pokoju " + raw_parm[3] + " (pokój jest moderowany i nie masz uprawnień).");
	}

	// jeśli inny typ wiadomości, pokaż RAW bez zmian
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));
	}
}


/*
	405 (próba wejścia do zbyt dużej liczby pokoi)
	:cf1f2.onet 405 ucieszony86 #abc :You are on too many channels
*/
void raw_405(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* Nie możesz wejść do pokoju " + raw_parm[3] + ", ponieważ jesteś w zbyt dużej liczbie pokoi.");
}


/*
	406 (początek WHOWAS, gdy nie było takiego nicka)
	:cf1f3.onet 406 ucc_test abc :There was no such nickname
*/
void raw_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xCYAN + raw_parm[3] + xNORMAL " - nie było takiego nicka na czacie lub brak go w bazie danych.");
}


/*
	412 (PRIVMSG #pokój :)
	:cf1f2.onet 412 ucc_test :No text to send
*/
void raw_412(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Nie wysłano tekstu.");
}


/*
	421
	:cf1f2.onet 421 ucc_test WHO :This command has been disabled.
	:cf1f2.onet 421 ucc_test ABC :Unknown command
*/
void raw_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli polecenie jest wyłączone
	if(buffer_irc_raw.find("This command has been disabled.") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[3] + " - to polecenie czata zostało wyłączone.");
	}

	// jeśli polecenie jest nieznane
	else if(buffer_irc_raw.find("Unknown command") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[3] + " - nieznane polecenie czata.");
	}

	// gdy inna odpowiedź serwera, wyświetl oryginalny tekst
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "* " + raw_parm[3] + " - " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
	}
}


/*
	433
	:cf1f4.onet 433 * ucc_test :Nickname is already in use.
*/
void raw_433(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Nick " + raw_parm[3] + " jest już w użyciu.");

	ga.irc_ok = false;	// tymczasowo, przerobić to na coś lepszego, np. pytanie o useroverride
}


/*
	451
	:cf1f4.onet 451 PING :You have not registered
*/
void raw_451(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[2] + " - nie jesteś zarejestrowany(-na).");
}


/*
	461
	:cf1f3.onet 461 ucc_test PRIVMSG :Not enough parameters.
	:cf1f4.onet 461 ucc_test NICK :Not enough parameters.
*/
void raw_461(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Brak wystarczającej liczby parametrów dla " + raw_parm[3]);
}


/*
	473
	:cf1f2.onet 473 ucc_test #a :Cannot join channel (Invite only)
*/
void raw_473(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Nie możesz wejść do pokoju " + raw_parm[3] + " (nie masz zaproszenia).");
}


/*
	482 (TOPIC)
	:cf1f1.onet 482 Kernel_Panic #Suwałki :You must be at least a half-operator to change the topic on this channel
*/
void raw_482(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, raw_parm[3], xRED "* Nie masz uprawnień do zmiany tematu w pokoju " + raw_parm[3]);
}


/*
	530
	:cf1f3.onet 530 ucc_test #sc :Only IRC operators may create new channels
*/
void raw_530(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm[3] + " - nie ma takiego pokoju.");
}


/*
	600
	:cf1f2.onet 600 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :arrived online
*/
void raw_600(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// informacja w aktywnym pokoju (o ile to nie "Status")
	if(chan_parm[ga.current] && chan_parm[ga.current]->channel != "Status")
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xMAGENTA "* Twój przyjaciel " + raw_parm[3] + " [" + raw_parm[4] + "@" + raw_parm[5] + "] pojawia się na czacie.");
	}

	// informacja w "Status" wraz z datą i godziną
	win_buf_add_str(ga, chan_parm, "Status",
			xMAGENTA "* Twój przyjaciel " + raw_parm[3] + " [" + raw_parm[4] + "@" + raw_parm[5]
			+ "] pojawia się na czacie (" + time_unixtimestamp2local_full(raw_parm[6]) +  ").");

	// aktywność typu 1 w "Status"
	chan_act_add(chan_parm, "Status", 1);
}


/*
	601
	:cf1f2.onet 601 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337863 :went offline
*/
void raw_601(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// informacja w aktywnym pokoju (o ile to nie "Status")
	if(chan_parm[ga.current] && chan_parm[ga.current]->channel != "Status")
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xWHITE "* Twój przyjaciel " + raw_parm[3] + " [" + raw_parm[4] + "@" + raw_parm[5] + "] wychodzi z czata.");
	}

	// informacja w "Status" wraz z datą i godziną
	win_buf_add_str(ga, chan_parm, "Status",
			xWHITE "* Twój przyjaciel " + raw_parm[3] + " [" + raw_parm[4] + "@" + raw_parm[5]
			+ "] wychodzi z czata (" + time_unixtimestamp2local_full(raw_parm[6]) + ").");

	// aktywność typu 1 w "Status"
	chan_act_add(chan_parm, "Status", 1);
}


/*
	602
	:cf1f2.onet 602 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :stopped watching
	:cf1f2.onet 602 ucc_test ucieszony86 * * 0 :stopped watching
*/
void raw_602()
{
}


/*
	604
	:cf1f2.onet 604 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :is online
*/
void raw_604(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// wyświetl w pokoju "Status"
	win_buf_add_str(ga, chan_parm, "Status", xMAGENTA "* Twój przyjaciel " + raw_parm[3] + " [" + raw_parm[4] + "@" + raw_parm[5]
			+ "] jest na czacie od: " + time_unixtimestamp2local_full(raw_parm[6]));
}


/*
	605
	:cf1f1.onet 605 ucieszony86 devi85 * * 0 :is offline
*/
void raw_605(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// wyświetl w pokoju "Status"
	win_buf_add_str(ga, chan_parm, "Status", xWHITE "* Twojego przyjaciela " + raw_parm[3] + " nie ma na czacie.");
}


/*
	801 (authKey)
	:cf1f4.onet 801 ucc_test :<authKey>
*/
void raw_801(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	std::string authkey = raw_parm[3];

	// konwersja authKey na nowy_authKey
	auth_code(authkey);

	if(authkey.size() == 0)
	{
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# authKey nie zawiera oczekiwanych 16 znaków (możliwa zmiana autoryzacji).");
	}

	else
	{
		// wyślij:
		// AUTHKEY <nowy_authKey>
		irc_send(ga, chan_parm, "AUTHKEY " + authkey);
	}
}


/*
	807 (BUSY 1)
	:cf1f3.onet 807 ucc_test :You are marked as busy
*/
void raw_807(struct global_args &ga, struct channel_irc *chan_parm[])
{
	ga.busy_state = true;

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xWHITE "* Jesteś teraz oznaczony(-na) jako zajęty(-ta) i nie będziesz otrzymywać zaproszeń do rozmów prywatnych.");
}


/*
	808 (BUSY 0)
	:cf1f3.onet 808 ucc_test :You are no longer marked busy
*/
void raw_808(struct global_args &ga, struct channel_irc *chan_parm[])
{
	ga.busy_state = false;

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xWHITE "* Nie jesteś już oznaczony(-na) jako zajęty(-ta) i możesz otrzymywać zaproszenia do rozmów prywatnych.");
}


/*
	809 (WHOIS)
	:cf1f2.onet 809 ucc_test AT89S8253 :is busy
*/
void raw_809(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest zajęty(-ta) i nie przyjmuje zaproszeń do rozmów prywatnych.");
}


/*
	811 (INVIGNORE)
	:cf1f2.onet 811 ucc_test Kernel_Panic :Ignore invites
*/
void raw_811(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Zignorowano zaproszenia od " + raw_parm[3]);
}


/*
	812 (INVREJECT)
	:cf1f4.onet 812 ucc_test Kernel_Panic ^cf1f2754610 :Invite rejected
*/
void raw_812(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// poprawić na odróżnianie pokoi i rozmów prywatnych
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Odrzucono zaproszenie od " + raw_parm[3] + " do " + raw_parm[4]);
}


/*
	815 (WHOIS)
	:cf1f4.onet 815 ucc_test ucieszony86 :Public webcam
*/
void raw_815(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xGREEN + raw_parm[3] + xNORMAL " ma włączoną publiczną kamerkę.");
}


/*
	816 (WHOIS)
	:cf1f4.onet 816 ucc_test ucieszony86 :Private webcam
*/
void raw_816(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xGREEN + raw_parm[3] + xNORMAL " ma włączoną prywatną kamerkę.");
}



/*
	817 (historia)
	:cf1f4.onet 817 ucc_test #scc 1401032138 Damian - :bardzo korzystne oferty maja i w sumie dosc rozwiniety jak na Polskie standardy panel chmury
	:cf1f4.onet 817 ucc_test #ucc 1401176793 ucc_test - :\1ACTION test\1
*/
void raw_817(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// wykryj, gdy ktoś pisze przez użycie /me
	std::string action_me = get_value_from_buf(buffer_irc_raw, " :\x01" "ACTION ", "\x01\n");

	// jeśli tak, wyświetl inaczej wiadomość
	if(action_me.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm[3],
				time_unixtimestamp2local(raw_parm[4]) + xMAGENTA "* " + raw_parm[5] + xNORMAL " " + action_me, false);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm[3],
				time_unixtimestamp2local(raw_parm[4]) + xWHITE "<" + raw_parm[5] + ">" xNORMAL " "
				+ get_value_from_buf(buffer_irc_raw, " :", "\n"), false);
	}
}


/*
	951
	:cf1f1.onet 951 ucieszony86 ucieszony86 :Added Bot!*@* <privatemessages,channelmessages,invites> to silence list
*/
void raw_951(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// informacja w "Status"
	win_buf_add_str(ga, chan_parm, "Status",
			xYELLOW "* Dodano " + raw_parm[5] + " do listy ignorowanych, nie będziesz otrzymywać od niego zaproszeń ani powiadomień.");

	// aktywność typu 1
	chan_act_add(chan_parm, "Status", 1);
}


/*
	Poniżej obsługa RAW NOTICE bez liczb.
*/


/*
	NOTICE nienumeryczne
	:cf1f4.onet NOTICE Auth :*** Looking up your hostname...
	:cf1f4.onet NOTICE ucc_test :Setting your VHost: ucc
	:cf1f1.onet NOTICE ^cf1f1756979 :*** ucc_test invited Kernel_Panic into the channel
	:cf1f2.onet NOTICE #Computers :*** drew_barrymore invited aga271980 into the channel
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 NOTICE #ucc :test
*/
void raw_notice(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string nick_notice;

	// jeśli to typowy nick w stylu AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974, to pobierz część przed !
	if(raw_parm[0].find("!") != std::string::npos)
	{
		nick_notice = get_value_from_buf(buffer_irc_raw, ":", "!");
	}

	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	else
	{
		nick_notice = raw_parm[0];
	}

	// jeśli NOTICE Auth, wyświetl go w "Status"
	if(raw_parm[2] == "Auth")
	{
		win_buf_add_str(ga, chan_parm, "Status",
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
	}

	// jeśli to "Setting your VHost" (ignoruj tę sekwencję dla zwykłych nicków, czyli takich, które mają ! w raw_parm[0])
	else if(buffer_irc_raw.find("Setting your VHost") != std::string::npos && raw_parm[0].find("!") == std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Ustawiam Twój VHost:"
				+ get_value_from_buf(buffer_irc_raw, "VHost:", "\n"));
	}

	// jeśli to zaproszenie do pokoju lub do rozmowy prywatnej, komunikat skieruj do właściwego pokoju
	else if(buffer_irc_raw.find(":*** " +  raw_parm[4] + " invited " + raw_parm[6] + " into the channel") != std::string::npos
		&& raw_parm[0].find("!") == std::string::npos)
	{
		// rozmowa prywatna
		if(raw_parm[2][0] == '^')
		{
			win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* Wysłano zaproszenie do rozmowy prywatnej dla " + raw_parm[6]);
		}

		// pokój
		else
		{
			win_buf_add_str(ga, chan_parm, raw_parm[2],
					xWHITE "* " + raw_parm[6] + " został(a) zaproszony(-na) do pokoju " + raw_parm[2] + " przez " + raw_parm[4]);

			// aktywność typu 1
			chan_act_add(chan_parm, raw_parm[2], 1);
		}
	}

	// jeśli to wiadomość dla pokoju, a nie nicka, komunikat skieruj do właściwego pokoju
	else if(raw_parm[2].size() > 0 && raw_parm[2][0] == '#')
	{
		win_buf_add_str(ga, chan_parm, raw_parm[2],
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
	}

	// jeśli to wiadomość dla nicka (mojego), komunikat skieruj do aktualnie otwartego pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
	}
}


/*
	Poniżej obsługa RAW NOTICE numerycznych.
*/


/*
	NOTICE 100
	:Onet-Informuje!bot@service.onet NOTICE $* :100 #jakis_pokoj 1401386400 :jakis tekst
*/
void raw_notice_100(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// ogłoszenia serwera wrzucaj do "Status"
	win_buf_add_str(ga, chan_parm, "Status",
			"* " xBOLD_ON "-" xMAGENTA + get_value_from_buf(buffer_irc_raw, ":", "!") + xTERMC "-" xNORMAL + " W pokoju " + raw_parm[4]
			+ " (" + time_unixtimestamp2local_full(raw_parm[5]) + "): " + get_value_from_buf(buffer_irc_raw, raw_parm[5] + " :", "\n"));

	// aktywność typu 1
	chan_act_add(chan_parm, "Status", 1);
}


/*
	NOTICE 109
	:GuardServ!service@service.onet NOTICE ucc_test :109 #Suwałki :oszczędzaj enter - pisz w jednej linijce
	:GuardServ!service@service.onet NOTICE ucc_test :109 #Suwałki :rzucanie mięsem nie będzie tolerowane
*/
void raw_notice_109(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, raw_parm[4],
			"* " xBOLD_ON "-" xMAGENTA + get_value_from_buf(buffer_irc_raw, ":", "!")
			+ xTERMC "-" xNORMAL " " + get_value_from_buf(buffer_irc_raw, raw_parm[4] + " :", "\n"));
}


/*
	NOTICE 111 (NS INFO nick - początek)
*/
void raw_notice_111(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 avatar :http://foto0.m.ocdn.eu/_m/3e7c4b7dec69eb13ed9f013f1fa2abd4,1,19,0.jpg
	if(raw_parm[5] == "avatar")
	{
		ga.card_avatar = get_value_from_buf(buffer_irc_raw, "avatar :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 birthdate :1986-02-12
	else if(raw_parm[5] == "birthdate")
	{
		ga.card_birthdate = get_value_from_buf(buffer_irc_raw, "birthdate :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 city :
	else if(raw_parm[5] == "city")
	{
		ga.card_city = get_value_from_buf(buffer_irc_raw, "city :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 country :
	else if(raw_parm[5] == "country")
	{
		ga.card_country = get_value_from_buf(buffer_irc_raw, "country :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 email :
	else if(raw_parm[5] == "email")
	{
		ga.card_email = get_value_from_buf(buffer_irc_raw, "email :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 longDesc :
	else if(raw_parm[5] == "longDesc")
	{
		ga.card_long_desc = get_value_from_buf(buffer_irc_raw, "longDesc :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 offmsg :friend
	else if(raw_parm[5] == "offmsg")
	{
		ga.card_offmsg = get_value_from_buf(buffer_irc_raw, "offmsg :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 prefs :111000001001110100;1|100|100|0;verdana;006699;14
	else if(raw_parm[5] == "prefs")
	{
		ga.card_prefs = get_value_from_buf(buffer_irc_raw, "prefs :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 rank :1.6087
	else if(raw_parm[5] == "rank")
	{
		ga.card_rank = get_value_from_buf(buffer_irc_raw, "rank :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 sex :M
	else if(raw_parm[5] == "sex")
	{
		ga.card_sex = get_value_from_buf(buffer_irc_raw, "sex :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 shortDesc :Timeout.
	else if(raw_parm[5] == "shortDesc")
	{
		ga.card_short_desc = get_value_from_buf(buffer_irc_raw, "shortDesc :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 type :1
	else if(raw_parm[5] == "type")
	{
		ga.card_type = get_value_from_buf(buffer_irc_raw, "type :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 vEmail :0
	else if(raw_parm[5] == "vEmail")
	{
		ga.card_v_email = get_value_from_buf(buffer_irc_raw, "vEmail :", "\n");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 www :
	else if(raw_parm[5] == "www")
	{
		ga.card_www = get_value_from_buf(buffer_irc_raw, "www :", "\n");
	}
}


/*
	NOTICE 112 (NS INFO nick - koniec)
	:NickServ!service@service.onet NOTICE ucieszony86 :112 ucieszony86 :end of user info
*/
void raw_notice_112(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// do poprawy wykrywanie nicka, aby nie zdarzyło się, że zebrane w NOTICE 111 dane należą do innego nicka (ale to dopiero po dodaniu listy nicków)

	// wizytówkę wyświetl tylko po użyciu polecenia /card, natomiast ukryj ją po zalogowaniu na czat
	if(ga.command_card)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xBOLD_ON xMAGENTA + raw_parm[4] + xTERMC " [Wizytówka]");

		if(ga.card_avatar.size() > 0)
		{
			size_t card_avatar_full = ga.card_avatar.find(",1");

			if(card_avatar_full != std::string::npos)
			{
				ga.card_avatar.erase(card_avatar_full + 1, 1);
				ga.card_avatar.insert(card_avatar_full + 1, "0");
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm[4] + xNORMAL " awatar: " + ga.card_avatar);
		}

		if(ga.card_birthdate.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm[4] + xNORMAL " data urodzenia: " + ga.card_birthdate);
		}

		if(ga.card_sex.size() > 0)
		{
			if(ga.card_sex == "M")
			{
				ga.card_sex = "mężczyzna";
			}

			else if(ga.card_sex == "F")
			{
				ga.card_sex = "kobieta";
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm[4] + xNORMAL " płeć: " + ga.card_sex);
		}

		if(ga.card_city.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm[4] + xNORMAL " miasto: " + ga.card_city);
		}

		if(ga.card_country.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,"* " xMAGENTA + raw_parm[4] + xNORMAL " kraj: " + ga.card_country);
		}

		if(ga.card_short_desc.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm[4] + xNORMAL " krótki opis: " + ga.card_short_desc);
		}

		if(ga.card_long_desc.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm[4] + xNORMAL " długi opis: " + ga.card_long_desc);
		}

		if(ga.card_email.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm[4] + xNORMAL " email: " + ga.card_email);
		}

		if(ga.card_v_email.size() > 0)
		{
			/* win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm[4] + xNORMAL " vEmail: " + ga.card_v_email); */
		}

		if(ga.card_www.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm[4] + xNORMAL " www: " + ga.card_www);
		}

		if(ga.card_offmsg.size() > 0)
		{
			if(ga.card_offmsg == "all")
			{
				ga.card_offmsg = "przyjmuje wiadomości offline od wszystkich";
			}

			else if(ga.card_offmsg == "friend")
			{
				ga.card_offmsg = "przyjmuje wiadomości offline od przyjaciół";
			}

			else if(ga.card_offmsg == "none")
			{
				ga.card_offmsg = "nie przyjmuje wiadomości offline";
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm[4] + xNORMAL " " + ga.card_offmsg + ".");
		}

		if(ga.card_type.size() > 0)
		{
			if(ga.card_type == "0")
			{
				ga.card_type = "nowicjusz";
			}

			else if(ga.card_type == "1")
			{
				ga.card_type = "bywalec";
			}

			else if(ga.card_type == "2")
			{
				ga.card_type = "wyjadacz";
			}

			else if(ga.card_type == "3")
			{
				ga.card_type = "guru";
			}

			// dla mojego nicka zwraca dokładniejszą informację o randze, dodaj ją w nawiasie
			if(ga.card_rank.size() > 0)
			{
				ga.card_type += " (" + ga.card_rank + ")";
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm[4] + xNORMAL " ranga: " + ga.card_type);
		}

		if(ga.card_prefs.size() > 0)
		{
			/* win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm[4] + xNORMAL " preferencje: " + ga.card_prefs); */
		}

	}	// if(ga.command_card)

	// dla pewności wyczyść informacje (nie wszystkie muszą pojawić się ponownie dla innego nicka)
	ga.card_avatar.clear();
	ga.card_birthdate.clear();
	ga.card_city.clear();
	ga.card_country.clear();
	ga.card_email.clear();
	ga.card_long_desc.clear();
	ga.card_offmsg.clear();
	ga.card_prefs.clear();
	ga.card_rank.clear();
	ga.card_sex.clear();
	ga.card_short_desc.clear();
	ga.card_type.clear();
	ga.card_v_email.clear();
	ga.card_www.clear();

	// skasuj ewentualne użycie /card
	ga.command_card = false;
}


/*
	NOTICE 121 (ulubione nicki - lista)
	:NickServ!service@service.onet NOTICE ucieszony86 :121 :devi85
*/
void raw_notice_121()
{
	// feature
}


/*
	NOTICE 122 (ulubione nicki - koniec listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :122 :end of friend list
*/
void raw_notice_122()
{
	// feature
}


/*
	NOTICE 131 (ignorowane nicki - lista)
	:NickServ!service@service.onet NOTICE ucieszony86 :131 :Bot
*/
void raw_notice_131()
{
	// feature
}


/*
	NOTICE 132 (ignorowane nicki - koniec listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :132 :end of ignore list
*/
void raw_notice_132()
{
	// feature
}


/*
	NOTICE 141 (ulubione pokoje - lista)
	:NickServ!service@service.onet NOTICE ucieszony86 :141 :#Komputer #Suwałki #Learning_English #Computers #Komputery #scc #2012 #zua_zuy_zuo #Linux #ucc #ateizm #Augustów #PHP
*/
void raw_notice_141(struct global_args &ga, std::string &buffer_irc_raw)
{
	if(ga.my_favourites.size() > 0)
	{
		ga.my_favourites += " ";
	}

	ga.my_favourites += get_value_from_buf(buffer_irc_raw, ":141 :", "\n");
}


/*
	NOTICE 142 (ulubione pokoje - koniec listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :142 :end of favourites list
*/
void raw_notice_142(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// wejdź do ulubionych pokoi (później dodać opcję wyboru)
	std::string chan_join;

	for(int i = 0; i < static_cast<int>(ga.my_favourites.size()); ++i)
	{
		if(ga.my_favourites[i] == ' ')
		{
			// dodaj przecinek za pokojem
			chan_join += ",";

			// usuń ewentualne spacje za tą spacją
			for(int j = i + 1; j < static_cast<int>(ga.my_favourites.size()); ++j)
			{
				if(ga.my_favourites[j] != ' ')
				{
					break;
				}

				++i;
			}
		}

		else
		{
			chan_join += ga.my_favourites[i];
		}
	}

	if(chan_join.size() > 0)
	{
		irc_send(ga, chan_parm, "JOIN " + buf_utf2iso(chan_join));
	}

	// po użyciu wyczyść listę
	ga.my_favourites.clear();
}


/*
	NOTICE 151 (CS HOMES - gdzie mamy opcje)
	:ChanServ!service@service.onet NOTICE ucc_test :151 :h#ucc h#Linux h#Suwałki h#scc
*/
void raw_notice_151(struct global_args &ga, std::string &buffer_irc_raw)
{
	if(ga.cs_homes.size() > 0)
	{
		ga.cs_homes += " ";
	}

	ga.cs_homes += get_value_from_buf(buffer_irc_raw, ":151 :", "\n");
}


/*
	NOTICE 152 (CS HOMES)
	:ChanServ!service@service.onet NOTICE ucc_test :152 :end of homes list
	NOTICE 152 (po zalogowaniu)
	:NickServ!service@service.onet NOTICE ucc_test :152 :end of offline senders list
*/
void raw_notice_152(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	size_t notice_152_type = buffer_irc_raw.find("end of homes list");

	if(notice_152_type != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Pokoje, w których masz uprawnienia: " + ga.cs_homes);

		// po użyciu wyczyść bufor, aby kolejne użycie CS HOMES wpisało wartość od nowa, a nie nadpisało
		ga.cs_homes.clear();
	}

	else
	{
//		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));
	}
}


/*
	NOTICE 220 (NS FRIENDS ADD nick)
	:NickServ!service@service.onet NOTICE ucc_test :220 ucieszony86 :friend added to list
*/
void raw_notice_220(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm[4] + " został(a) dodany(-na) do listy przyjaciół.");
}


/*
	NOTICE 221 (NS FRIENDS DEL nick)
	:NickServ!service@service.onet NOTICE ucc_test :221 ucieszony86 :friend removed from list
*/
void raw_notice_221(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm[4] + " został(a) usunięty(-ta) z listy przyjaciół.");
}


/*
	NOTICE 240 (NS FAVOURITES ADD #pokój)
	:NickServ!service@service.onet NOTICE ucc_test :240 #Linux :favourite added to list
*/
void raw_notice_240(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Pokój " + raw_parm[4] + " został dodany do listy ulubionych.");
}


/*
	NOTICE 241 (NS FAVOURITES DEL #pokój)
	:NickServ!service@service.onet NOTICE ucc_test :241 #ucc :favourite removed from list
*/
void raw_notice_241(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Pokój " + raw_parm[4] + " został usunięty z listy ulubionych.");
}


/*
	NOTICE 255 (pojawia się, gdy to ja zmieniam ustawienia, ale informuje o tym samym co RAW NOTICE 256, z wyjątkiem pokoju zamiast nicka,
	dlatego aby nie dublować informacji, ukryj jego działanie)
	:ChanServ!service@service.onet NOTICE ucc_test :255 #ucc -h ucc_test :channel privilege changed
	:ChanServ!service@service.onet NOTICE ucc_test :255 #ucc +v AT89S8253 :channel privilege changed
*/
void raw_notice_255()
{
}


/*
	NOTICE 256
	:ChanServ!service@service.onet NOTICE #ucc :256 ucieszony86 +b ucc_test :channel privilege changed
	:ChanServ!service@service.onet NOTICE #ucc :256 ucieszony86 -h ucc_test :channel privilege changed
	:ChanServ!service@service.onet NOTICE #ucc :256 ucieszony86 -o AT89S8253 :channel privilege changed
	:ChanServ!service@service.onet NOTICE #ucc :256 ucc_test +v AT89S8253 :channel privilege changed
	:ChanServ!service@service.onet NOTICE #ucc :256 Kernel_Panic +I abc!*@* :channel privilege changed
*/
void raw_notice_256(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// przebuduj parametry tak, aby pasowały do funkcji raw_mode(), aby nie dublować tej samej funkcji, poniżej przykładowy RAW z MODE:
	// :ChanServ!service@service.onet MODE #ucc +h ucc_test

	// w ten sposób należy przebudować parametry, aby pasowały do raw_mode()
	// 0 - tu wrzucić 4
	// 1 - bez znaczenia
	// 2 - pozostaje bez zmian
	// 3 - tu wrzucić 5
	// 4 - tu wrzucić 6

	raw_parm[0] = raw_parm[4];
	raw_parm[3] = raw_parm[5];
	raw_parm[4] = raw_parm[6];

	raw_mode(ga, chan_parm, raw_parm, buffer_irc_raw, true);
}


/*
	NOTICE 257 (np. gdy zmieniam status prywatności pokoju)
	:ChanServ!service@service.onet NOTICE ucieszony86 :257 #ucc * :settings changed
*/
void raw_notice_257()
{
}


/*
	NOTICE 258
	:ChanServ!service@service.onet NOTICE #ucc :258 ucieszony86 * :channel settings changed
	:ChanServ!service@service.onet NOTICE #ucc :258 ucieszony86 i :channel settings changed
*/
void raw_notice_258(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " zmienił(a) ustawienia pokoju " + raw_parm[2] + " (" + raw_parm[5] + ").");

	// aktywność typu 1
	chan_act_add(chan_parm, raw_parm[2], 1);
}


/*
	NOTICE 260 (pojawia się czasami wraz z RAW NOTICE 256 w nieco odmiennej formie, ale informuje o tym samym, dlatego aby nie dublować informacji,
	ukryj jego działanie)
	:ChanServ!service@service.onet NOTICE ucc_test :260 ucc_test #ucc -h :channel privilege changed
	:ChanServ!service@service.onet NOTICE ucc_test :260 AT89S8253 #ucc -h :channel privilege changed
*/
void raw_notice_260()
{
}


/*
	NOTICE 401 (NS INFO nick)
	:NickServ!service@service.onet NOTICE ucc_test :401 t :no such nick
*/
void raw_notice_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xMAGENTA + raw_parm[4] + xNORMAL " - nie ma takiego nicka na czacie.");
}


/*
	NOTICE 403 (RS INFO nick)
	:RankServ!service@service.onet NOTICE ucc_test :403 abc :user is not on-line
*/
void raw_notice_403(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xYELLOW_BLACK + raw_parm[4] + xNORMAL " - nie ma takiego nicka na czacie.");
}


/*
	NOTICE 404 (NS INFO nick - dla nicka tymczasowego)
	:NickServ!service@service.onet NOTICE ucc_test :404 ~złodziej_czasu :user is not registered
*/
void raw_notice_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xMAGENTA + raw_parm[4] + xNORMAL " - użytkownik nie jest zarejestrowany (posiada nick tymczasowy).");
}


/*
	NOTICE 406 (NS/CS/RS/GS nieznane_polecenie)
	:NickServ!service@service.onet NOTICE ucc_test :406 ABC :unknown command
*/
void raw_notice_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* " + raw_parm[4] + " - nieznane polecenie dla " + get_value_from_buf(buffer_irc_raw, ":", "!"));
}


/*
	NOTICE 408 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucc_test :408 abc :no such channel
*/
void raw_notice_408(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xBLUE + raw_parm[4] + xNORMAL " - nie ma takiego pokoju.");
}


/*
	NOTICE 415 (RS INFO nick - gdy jest online i nie mamy dostępu do informacji)
	:RankServ!service@service.onet NOTICE ucc_test :415 ucieszony86 :permission denied
*/
void raw_notice_415(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xYELLOW_BLACK + raw_parm[4] + xNORMAL " - dostęp do informacji zabroniony.");
}


/*
	NOTICE 416 (RS INFO #pokój - gdy nie mamy dostępu do informacji, musimy być w danym pokoju i być minimum opem, aby uzyskać informacje o nim)
	:RankServ!service@service.onet NOTICE ucc_test :416 #ucc :permission denied
*/
void raw_notice_416(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON + raw_parm[4] + xNORMAL " - dostęp do informacji zabroniony.");
}


/*
	NOTICE 420 (NS FRIENDS ADD nick - gdy nick już dodano do listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :420 legionella :is already on your friend list
*/
void raw_notice_420(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm[4] + " jest już na liście przyjaciół.");
}


/*
	NOTICE 421 (NS FRIENDS DEL nick - gdy nicka nie było na liście)
	:NickServ!service@service.onet NOTICE ucieszony86 :421 abc :is not on your friend list
*/
void raw_notice_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm[4] + " nie był(a) dodany(-na) do listy przyjaciół.");
}
