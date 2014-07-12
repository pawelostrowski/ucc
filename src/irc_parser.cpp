#include <sstream>		// std::string, std::stringstream
#include <sys/time.h>		// gettimeofday()

#include "irc_parser.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"


std::string get_value_from_buf(std::string in_buf, std::string expr_before, std::string expr_after)
{
/*
	Znajdź i pobierz wartość pomiędzy wyrażeniem początkowym i końcowym.
*/

	std::string value_from_buf;

	// znajdź pozycję początku szukanego wyrażenia
	size_t pos_expr_before = in_buf.find(expr_before);

	if(pos_expr_before != std::string::npos)
	{
		// znajdź pozycję końca szukanego wyrażenia, zaczynając od znalezionego początku + jego jego długości
		size_t pos_expr_after = in_buf.find(expr_after, pos_expr_before + expr_before.size());

		if(pos_expr_after != std::string::npos)
		{
			// wstaw szukaną wartość
			value_from_buf.insert(0, in_buf, pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());
		}
	}

	// zwróć szukaną wartość między wyrażeniami lub pusty bufor, gdy nie znaleziono początku lub końca
	return value_from_buf;
}


std::string get_rest_from_buf(std::string &in_buf, std::string expr_before)
{
/*
	Znajdź i pobierz resztę bufora od szukanej wartości (z jej pominięciem).
*/

	std::string rest_from_buf;

	// znajdź pozycję początku szukanego wyrażenia
	size_t pos_expr_before = in_buf.find(expr_before);

	if(pos_expr_before != std::string::npos)
	{
		// wstaw szukaną wartość
		rest_from_buf.insert(0, in_buf, pos_expr_before + expr_before.size(), in_buf.size() - pos_expr_before - expr_before.size());
	}

	// zwróć szukaną resztę po wyrażeniu początkowym aż do końca bufora lub pusty bufor, gdy nie znaleziono początku
	return rest_from_buf;
}


std::string get_raw_parm(std::string &raw_buf, int raw_parm_number)
{
/*
	Pobierz parametr RAW o numerze w raw_parm_number (liczonym od zera).
*/

	std::string raw_parm;
	std::stringstream raw_buf_stream;
	int raw_parm_index = 0;

	raw_buf_stream << raw_buf;

	while(raw_parm_index <= raw_parm_number)
	{
		if(! getline(raw_buf_stream, raw_parm, ' '))
		{
			// gdy szukany parametr jest poza buforem, zwróć pusty parametr (normalnie zostałby ostatnio odczytany)
			raw_parm.clear();

			// gdy koniec bufora, przerwij czytanie
			break;
		}

		// nie zwiększaj numeru indeksu odczytanego parametru, gdy odczytana zostanie spacja (ma to miejsce, gdy jest kilka spacji obok siebie)
		if(raw_parm.size() > 0)
		{
			++raw_parm_index;
		}
	}

	// jeśli na początku odczytanego parametru jest dwukropek, usuń go
	if(raw_parm.size() > 0 && raw_parm[0] == ':')
	{
		raw_parm.erase(0, 1);
	}

	return raw_parm;
}


void irc_parser(struct global_args &ga, struct channel_irc *chan_parm[], std::string msg_dbg_irc)
{
	std::string irc_recv_buf, raw_buf;
	std::stringstream irc_recv_buf_stream;

	std::string raw_parm0, raw_parm1;

	int raw_numeric;
	bool raw_unknown;

	// pobierz odpowiedź z serwera
	irc_recv(ga, chan_parm, irc_recv_buf, msg_dbg_irc);

	// w przypadku błędu podczas pobierania danych zakończ
	if(! ga.irc_ok)
	{
		return;
	}

	// konwersja formatowania fontów, kolorów i emotikon
	irc_recv_buf = form_from_chat(irc_recv_buf);

	irc_recv_buf_stream << irc_recv_buf;

	// obsłuż bufor (wiersz po wierszu)
	while(getline(irc_recv_buf_stream, raw_buf))
	{
		// pobierz parametry RAW, aby wykryć dalej, jaki to rodzaj RAW
		raw_parm0 = get_raw_parm(raw_buf, 0);
		raw_parm1 = get_raw_parm(raw_buf, 1);

		// dla RAW numerycznych zamień string na int ("0" zabezpiecza przed niedozwoloną konwersją znaków innych niż cyfry, aby nie wywalić programu)
		raw_numeric = std::stoi("0" + raw_parm1);

/*
	Zależnie od rodzaju RAW wywołaj odpowiednią funkcję.
*/
		raw_unknown = false;

		if(raw_parm0 == "ERROR")
		{
			raw_error(ga, chan_parm, raw_buf);
		}

		else if(raw_parm0 == "PING")
		{
			raw_ping(ga, chan_parm, raw_parm1);
		}

		else if(raw_parm1 == "INVIGNORE")
		{
			raw_invignore(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "INVITE")
		{
			raw_invite(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "INVREJECT")
		{
			raw_invreject(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "JOIN")
		{
			raw_join(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "KICK")
		{
			raw_kick(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "MODE")
		{
			raw_mode(ga, chan_parm, raw_buf);
		}

//		else if(raw_parm1 == "MODERATE")
//		{
//		}

		else if(raw_parm1 == "MODERMSG")
		{
			raw_modermsg(ga, chan_parm, raw_buf);
		}

//		else if(raw_parm1 == "MODERNOTICE")
//		{
//		}

		else if(raw_parm1 == "PART")
		{
			raw_part(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "PONG")
		{
			raw_pong(ga, raw_buf);
		}

		else if(raw_parm1 == "PRIVMSG")
		{
			raw_privmsg(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "QUIT")
		{
			raw_quit(ga, chan_parm, raw_buf);
		}

		else if(raw_parm1 == "TOPIC")
		{
			raw_topic(ga, chan_parm, raw_buf, raw_parm0);
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
				raw_256(ga, chan_parm, raw_buf, raw_parm0);
				break;

			case 257:
				raw_257(ga, chan_parm, raw_buf, raw_parm0);
				break;

			case 258:
				raw_258(ga, chan_parm, raw_buf, raw_parm0);
				break;

			case 259:
				raw_259(ga, chan_parm, raw_buf, raw_parm0);
				break;

			case 265:
				raw_265();
				break;

			case 266:
				raw_266();
				break;

			case 301:
				raw_301(ga, chan_parm, raw_buf);
				break;

			case 303:
				raw_303(ga, chan_parm, raw_buf);
				break;

			case 304:
				raw_304(ga, chan_parm, raw_buf);
				break;

			case 305:
				raw_305(ga, chan_parm);
				break;

			case 306:
				raw_306(ga, chan_parm);
				break;

			case 307:
				raw_307(ga, chan_parm, raw_buf);
				break;

			case 311:
				raw_311(ga, chan_parm, raw_buf);
				break;

			case 312:
				raw_312(ga, chan_parm, raw_buf);
				break;

			case 314:
				raw_314(ga, chan_parm, raw_buf);
				break;

			case 317:
				raw_317(ga, chan_parm, raw_buf);
				break;

			case 318:
				raw_318();
				break;

			case 319:
				raw_319(ga, chan_parm, raw_buf);
				break;

			case 332:
				raw_332(ga, chan_parm, raw_buf, get_raw_parm(raw_buf, 3));
				break;

			case 333:
				raw_333(ga, chan_parm, raw_buf);
				break;

			case 341:
				raw_341();
				break;

			case 353:
				raw_353(ga, raw_buf);
				break;

			case 366:
				raw_366(ga, chan_parm, raw_buf);
				break;

			case 369:
				raw_369();
				break;

			case 372:
				raw_372(ga, chan_parm, raw_buf);
				break;

			case 375:
				raw_375(ga, chan_parm);
				break;

			case 376:
				raw_376();
				break;

			case 378:
				raw_378(ga, chan_parm, raw_buf);
				break;

			case 391:
				raw_391(ga, chan_parm, raw_buf);
				break;

			case 396:
				raw_396(ga, chan_parm, raw_buf);
				break;

			case 401:
				raw_401(ga, chan_parm, raw_buf);
				break;

			case 402:
				raw_402(ga, chan_parm, raw_buf);
				break;

			case 403:
				raw_403(ga, chan_parm, raw_buf);
				break;

			case 404:
				raw_404(ga, chan_parm, raw_buf);
				break;

			case 405:
				raw_405(ga, chan_parm, raw_buf);
				break;

			case 406:
				raw_406(ga, chan_parm, raw_buf);
				break;

			case 412:
				raw_412(ga, chan_parm);
				break;

			case 421:
				raw_421(ga, chan_parm, raw_buf);
				break;

			case 433:
				raw_433(ga, chan_parm, raw_buf);
				break;

			case 441:
				raw_441(ga, chan_parm, raw_buf);
				break;

			case 451:
				raw_451(ga, chan_parm, raw_buf);
				break;

			case 461:
				raw_461(ga, chan_parm, raw_buf);
				break;

			case 473:
				raw_473(ga, chan_parm, raw_buf);
				break;

			case 482:
				raw_482(ga, chan_parm, raw_buf);
				break;

			case 484:
				raw_484(ga, chan_parm, raw_buf);
				break;

			case 530:
				raw_530(ga, chan_parm, raw_buf);
				break;

			case 600:
				raw_600(ga, chan_parm, raw_buf);
				break;

			case 601:
				raw_601(ga, chan_parm, raw_buf);
				break;

			case 602:
				raw_602();
				break;

			case 604:
				raw_604(ga, chan_parm, raw_buf);
				break;

			case 605:
				raw_605(ga, chan_parm, raw_buf);
				break;

			case 801:
				raw_801(ga, chan_parm, raw_buf);
				break;

			case 807:
				raw_807(ga, chan_parm);
				break;

			case 808:
				raw_808(ga, chan_parm);
				break;

			case 809:
				raw_809(ga, chan_parm, raw_buf);
				break;

			case 811:
				raw_811(ga, chan_parm, raw_buf);
				break;

			case 812:
				raw_812(ga, chan_parm, raw_buf);
				break;

			case 815:
				raw_815(ga, chan_parm, raw_buf);
				break;

			case 816:
				raw_816(ga, chan_parm, raw_buf);
				break;

			case 817:
				raw_817(ga, chan_parm, raw_buf);
				break;

			case 951:
				raw_951(ga, chan_parm, raw_buf);
				break;

			// nieznany lub niezaimplementowany jeszcze RAW numeryczny
			default:
				raw_unknown = true;
			}
		}

		// NOTICE ma specjalne znaczenie, bo może również zawierać RAW numeryczny
		else if(raw_parm1 == "NOTICE")
		{
			// dla RAW NOTICE numerycznych zamień string na int
			raw_numeric = std::stoi("0" + get_raw_parm(raw_buf, 3));

			if(raw_numeric == 0)
			{
				raw_notice(ga, chan_parm, raw_buf, raw_parm0);
			}

			else
			{
				switch(raw_numeric)
				{
				case 100:
					raw_notice_100(ga, chan_parm, raw_buf);
					break;

				case 109:
					raw_notice_109(ga, chan_parm, raw_buf);
					break;

				case 111:
					raw_notice_111(ga, chan_parm, raw_buf);
					break;

				case 112:
					raw_notice_112(ga, chan_parm, raw_buf);
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
					raw_notice_141(ga, raw_buf);
					break;

				case 142:
					raw_notice_142(ga, chan_parm);
					break;

				case 151:
					raw_notice_151(ga, raw_buf);
					break;

				case 152:
					raw_notice_152(ga, chan_parm, raw_buf);
					break;

				case 210:
					raw_notice_210(ga, chan_parm, raw_buf);
					break;

				case 211:
					raw_notice_211(ga, chan_parm, raw_buf);
					break;

				case 220:
					raw_notice_220(ga, chan_parm, raw_buf);
					break;

				case 221:
					raw_notice_221(ga, chan_parm, raw_buf);
					break;

				case 240:
					raw_notice_240(ga, chan_parm, raw_buf);
					break;

				case 241:
					raw_notice_241(ga, chan_parm, raw_buf);
					break;

				case 255:
					raw_notice_255();
					break;

				case 256:
					raw_notice_256(ga, chan_parm, raw_buf);
					break;

				case 257:
					raw_notice_257();
					break;

				case 258:
					raw_notice_258(ga, chan_parm, raw_buf);
					break;

				case 259:
					raw_notice_259(ga, chan_parm, raw_buf);
					break;

				case 260:
					raw_notice_260();
					break;

				case 401:
					raw_notice_401(ga, chan_parm, raw_buf);
					break;

				case 402:
					raw_notice_402(ga, chan_parm, raw_buf);
					break;

				case 403:
					raw_notice_403(ga, chan_parm, raw_buf);
					break;

				case 404:
					raw_notice_404(ga, chan_parm, raw_buf);
					break;

				case 406:
					raw_notice_406(ga, chan_parm, raw_buf);
					break;

				case 407:
					raw_notice_407(ga, chan_parm, raw_buf);
					break;

				case 408:
					raw_notice_408(ga, chan_parm, raw_buf);
					break;

				case 409:
					raw_notice_409(ga, chan_parm, raw_buf);
					break;

				case 411:
					raw_notice_411(ga, chan_parm, raw_buf);
					break;

				case 415:
					raw_notice_415(ga, chan_parm, raw_buf);
					break;

				case 416:
					raw_notice_416(ga, chan_parm, raw_buf);
					break;

				case 420:
					raw_notice_420(ga, chan_parm, raw_buf);
					break;

				case 421:
					raw_notice_421(ga, chan_parm, raw_buf);
					break;

				case 440:
					raw_notice_440(ga, chan_parm, raw_buf);
					break;

				case 441:
					raw_notice_441(ga, chan_parm, raw_buf);
					break;

				case 458:
					raw_notice_458(ga, chan_parm, raw_buf);
					break;

				case 461:
					raw_notice_461(ga, chan_parm, raw_buf);
					break;

				case 463:
					raw_notice_463(ga, chan_parm, raw_buf);
					break;

				case 465:
					raw_notice_465(ga, chan_parm, raw_buf);
					break;

				case 468:
					raw_notice_468(ga, chan_parm, raw_buf);
					break;

				// nieznany lub niezaimplementowany jeszcze RAW NOTICE
				default:
					raw_unknown = true;
				}
			}
		}

		// nieznany lub niezaimplementowany jeszcze RAW literowy
		else
		{
			raw_unknown = true;
		}

		// nieznany lub niezaimplementowany RAW (każdego typu) wyświetl bez zmian w oknie "RawUnknown" (zostanie utworzone, jeśli nie jest)
		if(raw_unknown)
		{
			new_chan_raw_unknown(ga, chan_parm);	// jeśli istnieje, funkcja nie utworzy ponownie pokoju
			win_buf_add_str(ga, chan_parm, "RawUnknown", xWHITE + raw_buf, 2, true, false);	// aby zwrócić uwagę, pokaż aktywność typu 2
		}
	}
}


/*
	Poniżej obsługa RAW ERROR i PING, które występują w odpowiedzi serwera są na pierwszej pozycji (w kolejności alfabetycznej).
*/


/*
	ERROR
	ERROR :Closing link (76995189@adei211.neoplus.adsl.tpnet.pl) [Client exited]
	ERROR :Closing link (76995189@adei211.neoplus.adsl.tpnet.pl) [Quit: z/w]
	ERROR :Closing link (unknown@eqw75.neoplus.adsl.tpnet.pl) [Registration timeout]
*/
void raw_error(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	ga.irc_ok = false;

	// wyczyść listy nicków otwartych pokoi oraz wyświetl komunikat we wszystkich otwartych pokojach (poza "Debug" i "RawUnknown")
	for(int i = 0; i < CHAN_NORMAL; ++i)
	{
		if(chan_parm[i])
		{
			chan_parm[i]->nick_parm.clear();
			win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, xYELLOW "* " + get_rest_from_buf(raw_buf, " :"));
		}
	}
}


/*
	PING
	PING :cf1f1.onet
*/
void raw_ping(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_parm1)
{
	// odpowiedz PONG na PING
	irc_send(ga, chan_parm, "PONG :" + raw_parm1);
}


/*
	Poniżej obsługa RAW nienumerycznych, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności alfabetycznej).
*/


/*
	INVIGNORE
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVIGNORE ucc_test ^cf1f2753898
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVIGNORE ucc_test #ucc
*/
void raw_invignore(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// jeśli użytkownik zignorował zaproszenie do rozmowy prywatnej
	if(raw_parm3.size() > 0 && raw_parm3[0] == '^')
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " zignorował(a) Twoje zaproszenie do rozmowy prywatnej.");
	}

	// jeśli użytkownik zignorował zaproszenie do pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " zignorował(a) Twoje zaproszenie do pokoju " + raw_parm3);
	}
}


/*
	INVITE
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc INVITE ucc_test :^cf1f1551082
	:ucieszony86!50256503@87edcc.6bc2d5.ee917f.54dae7 INVITE ucc_test :#ucc
*/
void raw_invite(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// jeśli to zaproszenie do rozmowy prywatnej
	if(raw_parm3.size() > 0 && raw_parm3[0] == '^')
	{
		// informacja w aktywnym pokoju (o ile to nie "Status" i nie "Debug")
		if(chan_parm[ga.current] && ga.current != CHAN_STATUS && ga.current != CHAN_DEBUG_IRC)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xBOLD_ON xYELLOW_BLACK "* "
					+ get_value_from_buf(raw_buf, ":", "!") + " [" + get_value_from_buf(raw_buf, "!", " ")
					+ "] zaprasza Cię do rozmowy prywatnej. Szczegóły w \"Status\" (Lewy Alt + s).");
		}

		// informacja w "Status"
		win_buf_add_str(ga, chan_parm, "Status", xBOLD_ON xYELLOW_BLACK "* "
				+ get_value_from_buf(raw_buf, ":", "!") + " [" + get_value_from_buf(raw_buf, "!", " ")
				+ "] zaprasza Cię do rozmowy prywatnej. Aby dołączyć, wpisz /join " + raw_parm3);
	}

	// jeśli to zaproszenie do pokoju
	else
	{
		// informacja w aktywnym pokoju (o ile to nie "Status" i nie "Debug")
		if(chan_parm[ga.current] && ga.current != CHAN_STATUS && ga.current != CHAN_DEBUG_IRC)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xBOLD_ON xYELLOW_BLACK "* "
					+ get_value_from_buf(raw_buf, ":", "!") + " [" + get_value_from_buf(raw_buf, "!", " ")
					+ "] zaprasza Cię do pokoju " + raw_parm3 + ", szczegóły w \"Status\" (Lewy Alt + s).");
		}

		// informacja w "Status"
		std::string chan_tmp = raw_parm3;	// po /join wytnij #, ale nie wycinaj go w pierwszej części zdania, dlatego użyj innego bufora

		if(chan_tmp.size() > 0)
		{
			chan_tmp.erase(0, 1);
		}

		win_buf_add_str(ga, chan_parm, "Status", xBOLD_ON xYELLOW_BLACK "* "
				+ get_value_from_buf(raw_buf, ":", "!") + " [" + get_value_from_buf(raw_buf, "!", " ")
				+ "] zaprasza Cię do pokoju " + raw_parm3 + ", aby wejść, wpisz /join " + chan_tmp);
	}
}


/*
	INVREJECT
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVREJECT ucc_test ^cf1f1123456
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVREJECT ucc_test #ucc
*/
void raw_invreject(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// jeśli użytkownik odrzucił zaproszenie do rozmowy prywatnej
	if(raw_parm3.size() > 0 && raw_parm3[0] == '^')
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " odrzucił(a) Twoje zaproszenie do rozmowy prywatnej.");
	}

	// jeśli użytkownik odrzucił zaproszenie do pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " odrzucił(a) Twoje zaproszenie do pokoju " + raw_parm3);
	}
}


/*
	JOIN
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c JOIN #ucc :rx,0
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc JOIN ^cf1f1551083 :rx,0
*/
void raw_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	// jeśli to ja wchodzę, utwórz nowy kanał (jeśli wpisano /join, przechodź od razu do tego pokoju - ga.command_join)
	if(get_value_from_buf(raw_buf, ":", "!") == buf_iso2utf(ga.zuousername) && ! new_chan_chat(ga, chan_parm, raw_parm2, ga.command_join))
	{
		// w przypadku błędu opuść pokój (i tak nie byłoby widać jego komunikatów)
		irc_send(ga, chan_parm, "PART " + buf_utf2iso(raw_parm2) + " :Channel index out of range.");

		// przełącz na "Status"
		ga.current = CHAN_STATUS;
		win_buf_refresh(ga, chan_parm);

		// wyświetl ostrzeżenie
		win_buf_add_str(ga, chan_parm, "Status", xRED "# Nie udało się wejść do pokoju " + raw_parm2 + " (brak pamięci w tablicy pokoi).");

		return;
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	else if(raw_parm2.size() > 0 && raw_parm2[0] == '^')
	{
		// jeśli to ja dołączam do rozmowy prywatnej, komunikat będzie inny, niż jeśli to ktoś dołącza
		if(get_value_from_buf(raw_buf, ":", "!") == buf_iso2utf(ga.zuousername))
		{
			win_buf_add_str(ga, chan_parm, raw_parm2, xGREEN "* Dołączasz do rozmowy prywatnej.");
		}

		else
		{
			win_buf_add_str(ga, chan_parm, raw_parm2, xGREEN "* " + get_value_from_buf(raw_buf, ":", "!")
					+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] dołącza do rozmowy prywatnej.");
		}
	}

	// w przeciwnym razie wyświetl komunikat dla wejścia do pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xGREEN "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] wchodzi do pokoju " + raw_parm2);
	}

	// dodaj nick do listy
	new_or_update_nick_chan(ga, chan_parm, raw_parm2, get_value_from_buf(raw_buf, ":", "!"), get_value_from_buf(raw_buf, "!", " "));

	// dodaj flagi nicka
	struct nick_flags flags = {};
	std::string join_flags = get_rest_from_buf(raw_buf, " :");

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

	update_nick_flags_chan(ga, chan_parm, raw_parm2, get_value_from_buf(raw_buf, ":", "!"), flags);

	// jeśli nick ma kamerkę, wyświetl o tym informację
	if(flags.public_webcam)
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] ma włączoną publiczną kamerkę.");
	}

	else if(flags.private_webcam)
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] ma włączoną prywatną kamerkę.");
	}

	// skasuj ewentualne użycie /join
	ga.command_join = false;

	// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
	if(chan_parm[ga.current]->channel == raw_parm2)
	{
		ga.nicklist_refresh = true;
	}
}


/*
	KICK
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :Zachowuj sie!
*/
void raw_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// pobierz powód, jeśli podano
	std::string reason = get_rest_from_buf(raw_buf, " :");

	if(reason.size() > 0)
	{
		reason.insert(0, " [Powód: ");
		reason += "]";
	}

	// jeśli to mnie wyrzucono, pokaż inny komunikat
	if(raw_parm3 == buf_iso2utf(ga.zuousername))
	{
		// usuń kanał z programu
		del_chan_chat(ga, chan_parm, raw_parm2);

		// komunikat o wyrzuceniu pokaż w kanale "Status"
		win_buf_add_str(ga, chan_parm, "Status",
				xRED "* Zostajesz wyrzucony(-na) z pokoju " + raw_parm2 + " przez " + get_value_from_buf(raw_buf, ":", "!") + reason);
	}

	else
	{
		// klucz nicka trzymany jest wielkimi literami
		std::string nick_key = raw_parm3;

		for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
		{
			if(islower(nick_key[i]))
			{
				nick_key[i] = toupper(nick_key[i]);
			}
		}

		// jeśli nick wszedł po mnie, to jego ZUO jest na liście, wtedy dodaj je do komunikatu
		std::string nick_zuo;

		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(chan_parm[i] && chan_parm[i]->channel == raw_parm2)
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
		win_buf_add_str(ga, chan_parm, raw_parm2, xRED "* " + raw_parm3 + nick_zuo + " zostaje wyrzucony(-na) z pokoju " + raw_parm2
				+ " przez " + get_value_from_buf(raw_buf, ":", "!") + reason);

		// usuń nick z listy
		del_nick_chan(ga, chan_parm, raw_parm2, raw_parm3);

		// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
		if(chan_parm[ga.current]->channel == raw_parm2)
		{
			ga.nicklist_refresh = true;
		}
	}
}


/*
	MODE

	Zmiany flag pokoju (przykładowe RAW):
	:ChanServ!service@service.onet MODE #Towarzyski +b *!12345678@*
	:ChanServ!service@service.onet MODE #Suwałki +b *!*@87edcc.6bc2d5.ee917f.54dae7
	:ChanServ!service@service.onet MODE #ucc +qo ucieszony86 ucieszony86
	:cf1f1.onet MODE #Suwałki +oq ucieszony86 ucieszony86
	:ChanServ!service@service.onet MODE #ucc +h ucc_test
	:ChanServ!service@service.onet MODE #ucc -b+eh *!76995189@* *!76995189@* ucc_test
	:ChanServ!service@service.onet MODE #ucc +eh *!76995189@* ucc_test
	:ChanServ!service@service.onet MODE #Towarzyski -m
	:GuardServ!service@service.onet MODE #scc -I *!6@*
	:ChanServ!service@service.onet MODE #ucc -ips

	Zmiany flag nicka (przykładowe RAW):
	:Darom!12265854@devel.onet MODE Darom :+O
	:Panie_kierowniku!57643619@devel.onet MODE Panie_kierowniku :+O
	:zagubiona_miedzy_wierszami!80541395@87edcc.6f9b99.6bd006.aee4fc MODE zagubiona_miedzy_wierszami +W
	:ucc_test!76995189@87edcc.6bc2d5.1917ec.38c71e MODE ucc_test +x
	:NickServ!service@service.onet MODE ucc_test +r
	:cf1f4.onet MODE ucc_test +b
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc MODE Kernel_Panic :+b

	Do zaimplementowania:
	:ChanServ!service@service.onet MODE #ucc +c 1
*/
void raw_mode(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, bool normal_user)
{
	// wyjaśnienie:
	// nick_gives		- nick, który nadaje/odbiera uprawnienia/statusy
	// nick_receives	- nick, który otrzymuje/któremu zabierane są uprawnienia/statusy

	std::string raw_parm0 = get_raw_parm(raw_buf, 0);
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	bool raw_mode_unknown;

	// flagi używane przy wchodzeniu do pokoi, które były otwarte po rozłączeniu, a program nie był zamknięty
	bool my_flag_x = false, my_flag_r = false;

	std::string a, nick_gives, nick_gives_key, chan_join;

	// flaga normal_user jest informacją, że to zwykły użytkownik czata (a nie np. ChanServ) ustawił daną flagę, dlatego wtedy dodaj "(a)" po "ustawił",
	// pojawia się tylko przy niektórych zmianach, dlatego nie wszędzie flaga normal_user jest brana pod uwagę
	if(normal_user)
	{
		a = "(a)";
	}

	// jeśli w raw_parm2 jest nick, a nie pokój, to go stamtąd pobierz
	if(raw_parm2.size() > 0 && raw_parm2[0] != '#')
	{
		nick_gives = raw_parm2;

		// klucz nicka trzymany jest wielkimi literami
		nick_gives_key = raw_parm2;

		for(int i = 0; i < static_cast<int>(nick_gives_key.size()); ++i)
		{
			if(islower(nick_gives_key[i]))
			{
				nick_gives_key[i] = toupper(nick_gives_key[i]);
			}
		}
	}

	// jeśli zaś to pokój, to sprawdź, czy nick to typowy zapis np. ChanServ!service@service.onet, wtedy pobierz część przed !
	else if(raw_parm0.find("!") != std::string::npos)
	{
		nick_gives = get_value_from_buf(raw_buf, ":", "!");
	}

	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	else
	{
		nick_gives = raw_parm0;
	}

	int s = 0;	// pozycja znaku +/- od początku raw_parm[3]
	int x = -1;	// ile znaków +/- było minus 1

	for(int i = 0; i < static_cast<int>(raw_parm3.size()); ++i)
	{
		if(raw_parm3[i] == '+' || raw_parm3[i] == '-')
		{
			s = i;
			++x;
			continue;	// gdy znaleziono znak + lub -, powróć do początku
		}

		raw_mode_unknown = false;

/*
	Zmiany flag pokoju.
*/
		if(raw_parm2.size() > 0 && raw_parm2[0] == '#')
		{
			std::string nick_receives = get_raw_parm(raw_buf, i - x + 3);

			// klucz nicka trzymany jest wielkimi literami, używany podczas aktualizacji wybranych flag
			std::string nick_receives_key = nick_receives;

			for(int j = 0; j < static_cast<int>(nick_receives_key.size()); ++j)
			{
				if(islower(nick_receives_key[j]))
				{
					nick_receives_key[j] = toupper(nick_receives_key[j]);
				}
			}

			if(raw_parm3[i] == 'q')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* " + nick_receives + " jest teraz właścicielem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.owner = true;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie jest już właścicielem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.owner = false;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}
			}

			else if(raw_parm3[i] == 'o')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* " + nick_receives + " jest teraz superoperatorem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.op = true;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie jest już superoperatorem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.op = false;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}
			}

			else if(raw_parm3[i] == 'h')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* " + nick_receives + " jest teraz operatorem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.halfop = true;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie jest już operatorem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.halfop = false;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}
			}

			else if(raw_parm3[i] == 'v')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* " + nick_receives + " jest teraz gościem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.voice = true;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie jest już gościem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.voice = false;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}
			}

			else if(raw_parm3[i] == 'X')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* " + nick_receives + " jest teraz moderatorem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.moderator = true;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie jest już moderatorem pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");

					// zaktualizuj flagę
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->channel == raw_parm2)
						{
							auto it = chan_parm[j]->nick_parm.find(nick_receives_key);

							if(it != chan_parm[j]->nick_parm.end())
							{
								it->second.flags.moderator = false;

								// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym
								// pokoju)
								if(j == ga.current)
								{
									ga.nicklist_refresh = true;
								}
							}

							break;
						}
					}
				}
			}

			else if(raw_parm3[i] == 'b')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xRED "* " + nick_receives + " otrzymuje bana w pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie posiada już bana w pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");
				}
			}

			else if(raw_parm3[i] == 'e')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* " + nick_receives + " posiada teraz wyjątek od bana w pokoju " + raw_parm2
							+ " (ustawił " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie posiada już wyjątku od bana w pokoju " + raw_parm2
							+ " (ustawił " + nick_gives + ").");
				}
			}

			else if(raw_parm3[i] == 'I')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xCYAN "* " + nick_receives + " jest teraz na liście zaproszonych w pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* " + nick_receives + " nie jest już na liście zaproszonych w pokoju " + raw_parm2
							+ " (ustawił" + a + " " + nick_gives + ").");
				}
			}

			else if(raw_parm3[i] == 'm')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* Pokój " + raw_parm2 + " jest teraz moderowany (ustawił" + a + " " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* Pokój " + raw_parm2 + " nie jest już moderowany (ustawił" + a + " " + nick_gives + ").");
				}
			}

			else if(raw_parm3[i] == 'i')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* Pokój " + raw_parm2 + " jest teraz ukryty (ustawił " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* Pokój " + raw_parm2 + " nie jest już ukryty (ustawił " + nick_gives + ").");
				}
			}

			else if(raw_parm3[i] == 'p')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* Pokój " + raw_parm2 + " jest teraz prywatny (ustawił " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* Pokój " + raw_parm2 + " nie jest już prywatny (ustawił " + nick_gives + ").");
				}
			}

			else if(raw_parm3[i] == 's')
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xMAGENTA "* Pokój " + raw_parm2 + " jest teraz sekretny (ustawił " + nick_gives + ").");
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, raw_parm2,
							xWHITE "* Pokój " + raw_parm2 + " nie jest już sekretny (ustawił " + nick_gives + ").");
				}
			}

			else
			{
				raw_mode_unknown = true;
			}
		}

/*
	Zmiany flag osób.
*/
		else
		{
			if(raw_parm3[i] == 'O')
			{
				if(raw_parm3[s] == '+')
				{
					// pokaż informację we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
									xMAGENTA "* " + raw_parm2 + " jest teraz deweloperem czata (ustawił(a) "
									+ nick_gives + ").");
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					// pokaż informację we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
									xWHITE "* " + raw_parm2 + " nie jest już deweloperem czata (ustawił(a) "
									+ nick_gives + ").");
						}
					}
				}
			}

			else if(raw_parm3[i] == 'W')
			{
				if(raw_parm3[s] == '+')
				{
					// pokaż informację we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
									xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
									+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] włącza publiczną kamerkę.");

							auto it = chan_parm[j]->nick_parm.find(nick_gives_key);

							// zaktualizuj flagę
							it->second.flags.public_webcam = true;

							// ze względu na obecny brak zmiany flagi V (brak MODE) należy ją wyzerować po włączeniu W
							it->second.flags.private_webcam = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					// pokaż informację we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
									xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
									+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] wyłącza publiczną kamerkę.");

							auto it = chan_parm[j]->nick_parm.find(nick_gives_key);

							// zaktualizuj flagę
							it->second.flags.public_webcam = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}
					}
				}
			}

			else if(raw_parm3[i] == 'V')
			{
				if(raw_parm3[s] == '+')
				{
					// pokaż informację we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
									xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
									+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] włącza prywatną kamerkę.");

							auto it = chan_parm[j]->nick_parm.find(nick_gives_key);

							// zaktualizuj flagę
							it->second.flags.private_webcam = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					// pokaż informację we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							win_buf_add_str(ga, chan_parm, chan_parm[j]->channel,
									xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
									+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] wyłącza prywatną kamerkę.");

							auto it = chan_parm[j]->nick_parm.find(nick_gives_key);

							// zaktualizuj flagę
							it->second.flags.private_webcam = false;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}
					}
				}
			}

			// aktualizacja flagi busy na liście nicków
			else if(raw_parm3[i] == 'b')
			{
				if(raw_parm3[s] == '+')
				{
					// pokaż zmianę we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							auto it = chan_parm[j]->nick_parm.find(nick_gives_key);

							// zaktualizuj flagę
							it->second.flags.busy = true;

							// odśwież listę w aktualnym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
							if(j == ga.current)
							{
								ga.nicklist_refresh = true;
							}
						}
					}
				}

				else if(raw_parm3[s] == '-')
				{
					// pokaż zmianę we wszystkich pokojach, gdzie jest dany nick
					for(int j = 0; j < CHAN_CHAT; ++j)
					{
						if(chan_parm[j] && chan_parm[j]->nick_parm.find(nick_gives_key) != chan_parm[j]->nick_parm.end())
						{
							auto it = chan_parm[j]->nick_parm.find(nick_gives_key);

							// zaktualizuj flagę
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

			// pokazuj tylko moje szyfrowanie IP (w pokoju "Status")
			else if(raw_parm3[i] == 'x' && raw_parm2 == buf_iso2utf(ga.zuousername))
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, "Status",
							xGREEN "* " + get_value_from_buf(raw_buf, ":", "!")
							+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] - masz teraz szyfrowane IP.");

					my_flag_x = true;
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, "Status",
							xWHITE "* " + get_value_from_buf(raw_buf, ":", "!")
							+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] - nie masz już szyfrowanego IP.");
				}
			}

			// pokazuj tylko mój zarejestrowany nick (w pokoju "Status")
			else if(raw_parm3[i] == 'r' && raw_parm2 == buf_iso2utf(ga.zuousername))
			{
				if(raw_parm3[s] == '+')
				{
					win_buf_add_str(ga, chan_parm, "Status", xGREEN "* Jesteś teraz zarejestrowanym użytkownikiem (ustawił "
							+ get_value_from_buf(raw_buf, ":", "!") + ").");

					my_flag_r = true;
				}

				else if(raw_parm3[s] == '-')
				{
					win_buf_add_str(ga, chan_parm, "Status", xWHITE "* Nie jesteś już zarejestrowanym użytkownikiem (ustawił "
							+ get_value_from_buf(raw_buf, ":", "!") + ").");
				}
			}

			else
			{
				raw_mode_unknown = true;
			}
		}

		// niezaimplementowane RAW z MODE wyświetl bez zmian w oknie "RawUnknown"
		if(raw_mode_unknown)
		{
			new_chan_raw_unknown(ga, chan_parm);	// jeśli istnieje, funkcja nie utworzy ponownie pokoju
			win_buf_add_str(ga, chan_parm, "RawUnknown", xWHITE + raw_buf, 2, true, false);	// aby zwrócić uwagę, pokaż aktywność typu 2
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

		for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "Debug" i "RawUnknown"
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
void raw_modermsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	int act_type;

	std::string form_start;

	std::string modermsg = get_rest_from_buf(raw_buf, " :");

	size_t nick_call = modermsg.find(buf_iso2utf(ga.zuousername));

	// jeśli ktoś mnie woła, jego nick wyświetl w żółtym kolorze
	if(nick_call != std::string::npos)
	{
		form_start = xYELLOW;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		act_type = 3;

		// testowa wersja dźwięku, do poprawy jeszcze
		if(system("aplay -q /usr/share/sounds/pop.wav 2>/dev/null &") != 0)
		{
			// coś
		}
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		act_type = 2;
	}

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string action_me = get_value_from_buf(raw_buf, " :\x01" "ACTION ", "\x01");

	// jeśli tak, wyświetl inaczej wiadomość
	if(action_me.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm4, xMAGENTA "* " + form_start + raw_parm2 + xNORMAL " " + action_me
				+ " " xNORMAL xRED "[Moderowany przez " + get_value_from_buf(raw_buf, ":", "!") + "]", act_type);
	}

	// a jeśli nie było /me wyświetl wiadomość w normalny sposób
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm4, xCYAN + form_start + "<" + raw_parm2 + ">" + xNORMAL " " + modermsg
				+ " " xNORMAL xRED "[Moderowany przez " + get_value_from_buf(raw_buf, ":", "!") + "]", act_type);
	}
}


/*
	PART
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc :Bye
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f3561508
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f1552723 :Koniec rozmowy
*/
void raw_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	// pobierz powód, jeśli podano
	std::string reason = get_rest_from_buf(raw_buf, " :");

	if(reason.size() > 0)
	{
		reason.insert(0, " [Powód: ");
		reason += "]";
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	if(raw_parm2.size() > 0 && raw_parm2[0] == '^')
	{
		if(reason.size() == 0)
		{
			reason = ".";
		}

		//jeśli to ja opuszczam rozmowę prywatną, komunikat będzie inny, niż jeśli to ktoś opuszcza
		if(get_value_from_buf(raw_buf, ":", "!") == buf_iso2utf(ga.zuousername))
		{
			win_buf_add_str(ga, chan_parm, raw_parm2, xCYAN "* Opuszczasz rozmowę prywatną" + reason);
		}

		else
		{
			win_buf_add_str(ga, chan_parm, raw_parm2, xCYAN "* " + get_value_from_buf(raw_buf, ":", "!")
					+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] opuszcza rozmowę prywatną" + reason);
		}
	}

	// w przeciwnym razie wyświetl komunikat dla wyjścia z pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xCYAN "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] wychodzi z pokoju " + raw_parm2 + reason);
	}

	// jeśli to ja wychodzę, usuń kanał z programu
	if(get_value_from_buf(raw_buf, ":", "!") == buf_iso2utf(ga.zuousername))
	{
		del_chan_chat(ga, chan_parm, raw_parm2);
	}

	else
	{
		// usuń nick z listy
		del_nick_chan(ga, chan_parm, raw_parm2, get_value_from_buf(raw_buf, ":", "!"));

		// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
		if(chan_parm[ga.current]->channel == raw_parm2)
		{
			ga.nicklist_refresh = true;
		}
	}
}


/*
	PONG (odpowiedź serwera na wysłany PING)
	:cf1f4.onet PONG cf1f4.onet :1404173770345
*/
void raw_pong(struct global_args &ga, std::string &raw_buf)
{
	// niereagowanie na wpisanie '/raw PING coś' (trzeba znać wysłaną wartość, a ręcznie jest to praktycznie niemożliwe do określenia), aby nie
	// fałszować informacji o lag wyświetlanej na dolnym pasku
	if(std::to_string(ga.ping) == get_raw_parm(raw_buf, 3))
	{
		struct timeval t_pong;

		gettimeofday(&t_pong, NULL);
		ga.pong = t_pong.tv_sec * 1000;
		ga.pong += t_pong.tv_usec / 1000;

		ga.lag = ga.pong - ga.ping;

		ga.lag_timeout = false;
	}
}


/*
	PRIVMSG
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 PRIVMSG #ucc :Hello.
	:Kernel_Panic!78259658@87edcc.6bc2d5.1917ec.38c71e PRIVMSG #ucc :\1ACTION %Cff0000%widzi co się dzieje.\1
*/
void raw_privmsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	int act_type;

	std::string form_start;

	size_t nick_call = raw_buf.find(buf_iso2utf(ga.zuousername));

	// jeśli ktoś mnie woła, pogrub jego nick i wyświetl w żółtym kolorze
	if(nick_call != std::string::npos)
	{
		form_start = xBOLD_ON xYELLOW_BLACK;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		act_type = 3;

		// testowa wersja dźwięku, do poprawy jeszcze
		if(system("aplay -q /usr/share/sounds/pop.wav 2>/dev/null &") != 0)
		{
			// coś
		}
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		act_type = 2;
	}

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string action_me = get_value_from_buf(raw_buf, " :\x01" "ACTION ", "\x01");

	// jeśli tak, wyświetl inaczej wiadomość
	if(action_me.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm2,
				xBOLD_ON xMAGENTA "* " + form_start + get_value_from_buf(raw_buf, ":", "!") + xNORMAL " " + action_me, act_type);
	}

	// a jeśli nie było /me wyświetl wiadomość w normalny sposób
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, form_start + "<" + get_value_from_buf(raw_buf, ":", "!") + ">"
				+ xNORMAL " " + get_rest_from_buf(raw_buf, " :"), act_type);
	}
}


/*
	QUIT
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Client exited
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Quit: Do potem
*/
void raw_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	// klucz nicka trzymany jest wielkimi literami
	std::string nick_key = get_value_from_buf(raw_buf, ":", "!");

	for(int i = 0; i < static_cast<int>(nick_key.size()); ++i)
	{
		if(islower(nick_key[i]))
		{
			nick_key[i] = toupper(nick_key[i]);
		}
	}

	// usuń nick ze wszystkich pokoi z listy, gdzie przebywał i wyświetl w tych pokojach komunikat o tym
	for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "Debug" i "RawUnknown"
	{
		// usuwać można tylko w otwartych pokojach oraz nie usuwaj nicka, jeśli takiego nie było w pokoju
		if(chan_parm[i] && chan_parm[i]->nick_parm.find(nick_key) != chan_parm[i]->nick_parm.end())
		{
			// usuń nick z listy
			chan_parm[i]->nick_parm.erase(nick_key);

			// w pokoju, w którym był nick wyświetl komunikat o jego wyjściu
			win_buf_add_str(ga, chan_parm, chan_parm[i]->channel, xYELLOW "* " + get_value_from_buf(raw_buf, ":", "!")
					+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] wychodzi z czata [" + get_rest_from_buf(raw_buf, " :") + "]");

			// odśwież listę w aktualnie otwartym pokoju (o ile zmiana dotyczyła nicka, który też jest w tym pokoju)
			if(i == ga.current)
			{
				ga.nicklist_refresh = true;
			}
		}
	}
}


/*
	TOPIC (CS SET #pokój TOPIC ...)
	:cf1f3.onet TOPIC #ucc :Ucieszony Chat Client
	TOPIC (TOPIC #pokój :...)
	:ucc_test!76995189@87edcc.30c29e.b9c507.d5c6b7 TOPIC #ucc :nowy temat
*/
void raw_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string &raw_parm0)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	// gdy ustawiono temat przez (tak robi program): CS SET #pokój TOPIC ...
	// wtedy inaczej zwracany jest nick
	if(raw_parm0.find("!") == std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xMAGENTA "* " + raw_parm0 + " zmienił temat pokoju " + raw_parm2);
	}

	// w przeciwnym razie temat ustawiony przez: TOPIC #pokój :...
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xMAGENTA "* " + get_value_from_buf(raw_buf, ":", "!")
				+ " [" + get_value_from_buf(raw_buf, "!", " ") + "] zmienił(a) temat pokoju " + raw_parm2);
	}

	// wyświetl temat oraz wpisz go również do bufora tematu kanału, aby wyświetlić go na górnym pasku
	// (reszta jest identyczna jak w obsłudze RAW 332, trzeba tylko zamiast raw_parm3 wysłać raw_parm2)
	raw_332(ga, chan_parm, raw_buf, raw_parm2);
}


/*
	Poniżej obsługa RAW numerycznych, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności numerycznej).
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
void raw_256(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm0 + xNORMAL "- " + get_rest_from_buf(raw_buf, " :"));
}


/*
	257 (ADMIN)
	:cf1f3.onet 257 ucc_test :Name     - Czat Admin
*/
void raw_257(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm0 + xNORMAL "- " + get_rest_from_buf(raw_buf, " :"));
}


/*
	258 (ADMIN)
	:cf1f3.onet 258 ucc_test :Nickname - czat_admin
*/
void raw_258(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm0 + xNORMAL "- " + get_rest_from_buf(raw_buf, " :"));
}


/*
	259 (ADMIN)
	:cf1f3.onet 259 ucc_test :E-Mail   - czat_admin@czat.onet.pl
*/
void raw_259(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* -" xMAGENTA + raw_parm0 + xNORMAL "- " + get_rest_from_buf(raw_buf, " :"));
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
void raw_301(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm3 + xNORMAL " jest nieobecny(-na) z powodu: " + get_rest_from_buf(raw_buf, " :"));
}


/*
	303 (ISON - zwraca nick, gdy jest online)
	:cf1f1.onet 303 ucc_test :Kernel_Panic
*/
void raw_303(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Nicki dostępne dla zapytania ISON: " + get_rest_from_buf(raw_buf, " :"));
}


/*
	304
	:cf1f3.onet 304 ucc_test :SYNTAX PRIVMSG <target>{,<target>} <message>
	:cf1f4.onet 304 ucc_test :SYNTAX NICK <newnick>
*/
void raw_304(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	// Zamień 'SYNTAX' na 'Składnia:'
	if(raw_buf.find(" :SYNTAX ") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Składnia: " + get_rest_from_buf(raw_buf, " :SYNTAX "));
	}

	// jeśli nie było ciągu 'SYNTAX', wyświetl komunikat bez zmian
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_rest_from_buf(raw_buf, " :"));
	}
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
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Jesteś teraz oznaczony(-na) jako nieobecny(-na) z powodu: " + ga.msg_away);

	ga.msg_away.clear();
}


/*
	307 (WHOIS)
	:cf1f1.onet 307 ucc_test ucc_test :is a registered nick
*/
void raw_307(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm3 + xNORMAL " jest zarejestrowanym użytkownikiem.");
}


/*
	311 (początek WHOIS, gdy nick jest na czacie)
	:cf1f2.onet 311 ucc_test AT89S8253 70914256 aaa2a7.a7f7a6.88308b.464974 * :tururu!
	:cf1f1.onet 311 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
	:cf1f2.onet 311 ucc_test ucc_test 76995189 87edcc.30c29e.611774.3140a3 * :ucc_test
*/
void raw_311(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xGREEN + raw_parm3 + xTERMC " [" + raw_parm4 + "@" + raw_parm5 + "]\n"
			+ "* " xGREEN + raw_parm3 + xNORMAL " ircname: " + get_rest_from_buf(raw_buf, " :"));
}


/*
	312 (WHOIS)
	:cf1f2.onet 312 ucc_test AT89S8253 cf1f3.onet :cf1f3
	312 (WHOWAS)
	:cf1f2.onet 312 ucc_test ucieszony86 cf1f2.onet :Mon May 26 21:39:35 2014
*/
void raw_312(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// mało elegancki sposób na odróżnienie WHOIS od WHOWAS (we WHOWAS są spacje po wyrażeniu z drugim dwukropkiem)
	std::string whowas = get_rest_from_buf(raw_buf, " :");

	size_t whois_whowas_type = whowas.find(" ");

	// jeśli WHOIS
	if(whois_whowas_type == std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xGREEN + raw_parm3 + xNORMAL " używa serwera: " + raw_parm4 + " [" + raw_parm5 + "]");
	}

	// jeśli WHOWAS
	else
	{
		std::string raw_parm6 = get_raw_parm(raw_buf, 6);
		std::string raw_parm7 = get_raw_parm(raw_buf, 7);
		std::string raw_parm8 = get_raw_parm(raw_buf, 8);
		std::string raw_parm9 = get_raw_parm(raw_buf, 9);

		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xCYAN + raw_parm3 + xNORMAL " używał(a) serwera: " + raw_parm4 + "\n"
				+ "* " xCYAN + raw_parm3 + xNORMAL " był(a) zalogowany(-na) od: "
				+ dayen2daypl(raw_parm5) + ", " + raw_parm7 + " " + monthen2monthpl(raw_parm6) + " " + raw_parm9 + ", " + raw_parm8);
	}
}


/*
	314 (początek WHOWAS, gdy nick był na czacie)
	:cf1f3.onet 314 ucc_test ucieszony86 50256503 e0c697.bbe735.1b1f7f.56f6ee * :ucieszony86
	:cf1f3.onet 314 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
*/
void raw_314(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xCYAN + raw_parm3 + xTERMC " [" + raw_parm4 + "@" + raw_parm5 + "]\n"
			+ "* " xCYAN + raw_parm3 + xNORMAL + " ircname: " + get_rest_from_buf(raw_buf, " :"));
}


/*
	317 (WHOIS)
	:cf1f1.onet 317 ucc_test AT89S8253 532 1400636388 :seconds idle, signon time
*/
void raw_317(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

        win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm3 + xNORMAL " jest nieaktywny(-na) przez: " + time_sec2time(raw_parm4) + "\n"
			+ "* " xGREEN + raw_parm3 + xNORMAL " jest zalogowany(-na) od: " + time_unixtimestamp2local_full(raw_parm5));
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
void raw_319(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm3 + xNORMAL " jest w pokojach: " + get_rest_from_buf(raw_buf, " :"));
}


/*
	332 (temat pokoju)
	:cf1f3.onet 332 ucc_test #ucc :Ucieszony Chat Client
*/
void raw_332(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string raw_parm3)
{
	// najpierw wyświetl temat (tylko, gdy jest ustawiony lub informację o braku tematu)
	std::string topic_tmp = get_rest_from_buf(raw_buf, " :");

	if(topic_tmp.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xWHITE "* Temat pokoju " xGREEN + raw_parm3 + xWHITE ": " xNORMAL + topic_tmp);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xWHITE "* Temat pokoju " xGREEN + raw_parm3 + xWHITE " nie został ustawiony (jest pusty).");
	}

	// teraz znajdź pokój, do którego należy temat, wpisz go do jego bufora "topic" i wyświetl na górnym pasku
	for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "Debug" i "RawUnknown"
	{
		if(chan_parm[i] && chan_parm[i]->channel == raw_parm3)	// znajdź pokój, do którego należy temat
		{
			// usuń z tematu formatowanie fontu i kolorów (na pasku nie jest obsługiwane)
			chan_parm[i]->topic = remove_form(topic_tmp);
			break;
		}
	}
}


/*
	333 (kto i kiedy ustawił temat)
	:cf1f4.onet 333 ucc_test #ucc ucc_test!76995189 1404519605
	:cf1f4.onet 333 ucc_test #ucc ucc_test!76995189@87edcc.30c29e.b9c507.d5c6b7 1400889719
*/
void raw_333(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, chan_parm, raw_parm3,
			xWHITE "* Temat pokoju " xGREEN + raw_parm3 + xWHITE " ustawiony przez " xCYAN + get_value_from_buf(raw_buf, raw_parm3 + " ", "!")
			+ " [" + get_value_from_buf(raw_buf, "!", " ") + "]" xWHITE " (" + time_unixtimestamp2local_full(raw_parm5) + ").");
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
void raw_353(struct global_args &ga, std::string &raw_buf)
{
	ga.names += get_rest_from_buf(raw_buf, " :");
}


/*
	366 (koniec NAMES)
	:cf1f4.onet 366 ucc_test #ucc :End of /NAMES list.
*/
void raw_366(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

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
			new_or_update_nick_chan(ga, chan_parm, raw_parm3, nick, "");

			// wpisz flagi nicka
			update_nick_flags_chan(ga, chan_parm, raw_parm3, nick, flags);

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
	if(chan_parm[ga.current]->channel == raw_parm3)
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
void raw_372(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	win_buf_add_str(ga, chan_parm, "Status", xYELLOW + get_rest_from_buf(raw_buf, " :"));
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
void raw_376()
{
}


/*
	378 (WHOIS)
	:cf1f1.onet 378 ucc_test ucc_test :is connecting from 76995189@adin155.neoplus.adsl.tpnet.pl 79.184.195.155
*/
void raw_378(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm3 + xNORMAL " jest połączony(-na) z: " + get_rest_from_buf(raw_buf, "from "));
}


/*
	391 (TIME)
	:cf1f4.onet 391 ucc_test cf1f4.onet :Tue May 27 08:59:41 2014
*/
void raw_391(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);
	std::string raw_parm7 = get_raw_parm(raw_buf, 7);
	std::string raw_parm8 = get_raw_parm(raw_buf, 8);

	// przekształć zwracaną datę i godzinę na formę poprawną dla polskiego zapisu
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xWHITE "* Data i czas na serwerze " + raw_parm3 + " - "
			+ dayen2daypl(raw_parm4) + ", " + raw_parm6 + " " + monthen2monthpl(raw_parm5) + " " + raw_parm8 + ", " + raw_parm7);
}


/*
	396
	:cf1f2.onet 396 ucc_test 87edcc.6bc2d5.1917ec.38c71e :is now your displayed host
*/
void raw_396(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string chan;

	// wyświetl w pokoju "Status" jeśli to odpowiedź po zalogowaniu
	if(! ga.command_vhost)
	{
		chan = "Status";
	}

	// wyświetl w aktualnym pokoju, jeśli to odpowiedź po użyciu /vhost
	else
	{
		chan = chan_parm[ga.current]->channel;
	}

	win_buf_add_str(ga, chan_parm, chan, xGREEN "* Twoim wyświetlanym hostem jest teraz " + raw_parm3);

	// skasuj ewentualne użycie /vhost
	ga.command_vhost = false;
}



/*
	401 (początek WHOIS, gdy nick/kanał nie został znaleziony, używany również przy podaniu nieprawidłowego pokoju, np. dla TOPIC, NAMES itd.)
	:cf1f4.onet 401 ucc_test abc :No such nick/channel
	:cf1f4.onet 401 ucc_test #ucc: :No such nick/channel
*/
void raw_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// jeśli nick
	if(raw_parm3.size() > 0 && raw_parm3[0] != '#')
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON xGREEN + raw_parm3 + xNORMAL " - nie ma takiego nicka na czacie.");
	}

	// jeśli pokój (niestety przez sposób, w jaki serwer zwraca ten RAW informacja dla WHOIS #pokój również zwraca informację o braku pokoju,
	// co może być mylące)
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - nie ma takiego pokoju.");
	}
}


/*
	402 (MOTD nieznany_serwer)
	:cf1f4.onet 402 ucc_test abc :No such server
*/
void raw_402(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - nie ma takiego serwera.");
}


/*
	403
	:cf1f4.onet 403 ucc_test sc :Invalid channel name
	:cf1f2.onet 403 ucc_test ^cf1f2123456 :Invalid channel name
*/
void raw_403(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - nieprawidłowa nazwa pokoju.");

	// dopisać, aby nieudane dołączenie do rozmowy prywatnej dawało komunikat "<nick> zrezygnował(a) z rozmowy prywatnej."
}


/*
	404 (próba wysłania wiadomości do kanału, w którym nie przebywamy)
	:cf1f1.onet 404 ucc_test #ucc :Cannot send to channel (no external messages)
	404 (próba wysłania wiadomości w pokoju moderowanym, gdzie nie mamy uprawnień)
	:cf1f2.onet 404 ucc_test #Suwałki :Cannot send to channel (+m)
*/
void raw_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	if(raw_buf.find(" :Cannot send to channel (no external messages)") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "* Nie można wysłać wiadomości do pokoju " + raw_parm3 + " (nie przebywasz w nim).");
	}

	else if(raw_buf.find(" :Cannot send to channel (+m)") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3,
				xRED "* Nie możesz pisać w pokoju " + raw_parm3 + " (pokój jest moderowany i nie posiadasz uprawnień).");
	}

	// jeśli inny typ wiadomości, pokaż ją bez zmian wraz z dodaniem pokoju, którego dotyczy ta wiadomość
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - " + get_rest_from_buf(raw_buf, " :"));
	}
}


/*
	405 (próba wejścia do zbyt dużej liczby pokoi)
	:cf1f2.onet 405 ucieszony86 #abc :You are on too many channels
*/
void raw_405(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* Nie możesz wejść do pokoju " + raw_parm3 + ", ponieważ jesteś w zbyt dużej liczbie pokoi.");
}


/*
	406 (początek WHOWAS, gdy nie było takiego nicka)
	:cf1f3.onet 406 ucc_test abc :There was no such nickname
*/
void raw_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xCYAN + raw_parm3 + xNORMAL " - nie było takiego nicka na czacie lub brak go w bazie danych.");
}


/*
	412 (PRIVMSG #pokój :)
	:cf1f2.onet 412 ucc_test :No text to send
*/
void raw_412(struct global_args &ga, struct channel_irc *chan_parm[])
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Nie wysłano tekstu.");
}


/*
	421
	:cf1f2.onet 421 ucc_test WHO :This command has been disabled.
	:cf1f2.onet 421 ucc_test ABC :Unknown command
*/
void raw_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// jeśli polecenie jest wyłączone
	if(raw_buf.find(" :This command has been disabled.") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - to polecenie czata zostało wyłączone.");
	}

	// jeśli polecenie jest nieznane
	else if(raw_buf.find(" :Unknown command") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - nieznane polecenie czata.");
	}

	// gdy inna odpowiedź serwera, wyświetl oryginalny tekst
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - " + get_rest_from_buf(raw_buf, " :"));
	}
}


/*
	433
	:cf1f4.onet 433 * ucc_test :Nickname is already in use.
*/
void raw_433(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Nick " + raw_parm3 + " jest już w użyciu.");

	ga.irc_ok = false;	// tymczasowo, przerobić to na coś lepszego, np. pytanie o useroverride
}


/*
	441 (KICK #pokój nick :<...> - gdy nie ma nicka w pokoju)
	:cf1f1.onet 441 ucieszony86 Kernel_Panic #ucc :They are not on that channel
*/
void raw_441(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, raw_parm4, xRED "* Nie możesz wyrzucić " + raw_parm3 + ", ponieważ nie przebywa w pokoju " + raw_parm4);
}


/*
	451
	:cf1f4.onet 451 PING :You have not registered
*/
void raw_451(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm2 + " - nie jesteś zarejestrowany(-na).");
}


/*
	461
	:cf1f3.onet 461 ucc_test PRIVMSG :Not enough parameters.
	:cf1f4.onet 461 ucc_test NICK :Not enough parameters.
*/
void raw_461(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - brak wystarczającej liczby parametrów.");
}


/*
	473
	:cf1f2.onet 473 ucc_test #a :Cannot join channel (Invite only)
*/
void raw_473(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* Nie możesz wejść do pokoju " + raw_parm3 + " (nie posiadasz zaproszenia).");
}


/*
	482
*/
void raw_482(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// TOPIC
	// :cf1f1.onet 482 Kernel_Panic #Suwałki :You must be at least a half-operator to change the topic on this channel
	if(raw_buf.find(" :You must be at least a half-operator to change the topic on this channel") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* Nie posiadasz uprawnień do zmiany tematu w pokoju " + raw_parm3);
	}

	// KICK sopa, będąc opem
	// :cf1f2.onet 482 ucc_test #ucc :You must be a channel operator
	else if(raw_buf.find(" :You must be a channel operator") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* Musisz być przynajmniej superoperatorem pokoju " + raw_parm3);
	}

	// KICK sopa lub opa, nie mając żadnych uprawnień
	// :cf1f3.onet 482 ucc_test #irc :You must be a channel half-operator
	else if(raw_buf.find(" :You must be a channel half-operator") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* Musisz być przynajmniej operatorem pokoju " + raw_parm3);
	}

	// nieznany lub niezaimplementowany powód wyświetl bez zmian
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* " + get_rest_from_buf(raw_buf, " :"));
	}
}


/*
	484
*/
void raw_484(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// KICK #pokój nick :<...> - próba wyrzucenia właściciela
	// :cf1f1.onet 484 ucieszony86 #ucc :Can't kick ucieszony86 as they're a channel founder
	if(raw_buf.find(" :Can't kick " + get_value_from_buf(raw_buf, "kick ", " as") + " as they're a channel founder")
		!= std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3,
				xRED "* Nie możesz wyrzucić " + get_value_from_buf(raw_buf, "kick ", " as")
				+ ", ponieważ jest właścicielem pokoju " + raw_parm3);
	}

	// KICK #pokój nick :<...> - próba wyrzucenia sopa przez innego sopa, opa przez innego opa lub nicka bez uprawnień, gdy sami ich nie posiadamy
	// :cf1f1.onet 484 ucieszony86 #Computers :Can't kick AT89S8253 as your spells are not good enough
	else if(raw_buf.find(" :Can't kick " + get_value_from_buf(raw_buf, "kick ", " as") + " as your spells are not good enough") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3,
				xRED "* Nie posiadasz wystarczających uprawnień, aby wyrzucić " + get_value_from_buf(raw_buf, "kick ", " as")
				+ " z pokoju " + raw_parm3);
	}

	// nieznany lub niezaimplementowany powód wyświetl bez zmian
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm3, xRED "* " + get_rest_from_buf(raw_buf, " :"));
	}
}


/*
	530 (JOIN #sc - nieistniejący pokój)
	:cf1f3.onet 530 ucc_test #sc :Only IRC operators may create new channels
*/
void raw_530(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm3 + " - nie ma takiego pokoju.");
}


/*
	600
	:cf1f2.onet 600 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :arrived online
*/
void raw_600(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// informacja w aktywnym pokoju (o ile to nie "Status" i nie "Debug")
	if(chan_parm[ga.current] && ga.current != CHAN_STATUS && ga.current != CHAN_DEBUG_IRC)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xMAGENTA "* Twój przyjaciel " + raw_parm3 + " [" + raw_parm4 + "@" + raw_parm5 + "] pojawia się na czacie.");
	}

	// informacja w "Status" wraz z datą i godziną
	win_buf_add_str(ga, chan_parm, "Status",
			xMAGENTA "* Twój przyjaciel " + raw_parm3 + " [" + raw_parm4 + "@" + raw_parm5
			+ "] pojawia się na czacie (" + time_unixtimestamp2local_full(raw_parm6) +  ").");
}


/*
	601
	:cf1f2.onet 601 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337863 :went offline
*/
void raw_601(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// informacja w aktywnym pokoju (o ile to nie "Status" i nie "Debug")
	if(chan_parm[ga.current] && ga.current != CHAN_STATUS && ga.current != CHAN_DEBUG_IRC)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xWHITE "* Twój przyjaciel " + raw_parm3 + " [" + raw_parm4 + "@" + raw_parm5 + "] wychodzi z czata.");
	}

	// informacja w "Status" wraz z datą i godziną
	win_buf_add_str(ga, chan_parm, "Status",
			xWHITE "* Twój przyjaciel " + raw_parm3 + " [" + raw_parm4 + "@" + raw_parm5
			+ "] wychodzi z czata (" + time_unixtimestamp2local_full(raw_parm6) + ").");
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
void raw_604(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// wyświetl w pokoju "Status"
	win_buf_add_str(ga, chan_parm, "Status", xMAGENTA "* Twój przyjaciel " + raw_parm3 + " [" + raw_parm4 + "@" + raw_parm5
			+ "] jest na czacie od: " + time_unixtimestamp2local_full(raw_parm6));
}


/*
	605
	:cf1f1.onet 605 ucieszony86 devi85 * * 0 :is offline
*/
void raw_605(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// wyświetl w pokoju "Status"
	win_buf_add_str(ga, chan_parm, "Status", xWHITE "* Twojego przyjaciela " + raw_parm3 + " nie ma na czacie.");
}


/*
	801 (authKey)
	:cf1f4.onet 801 ucc_test :authKey
*/
void raw_801(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// konwersja authKey
	std::string authkey = auth_code(raw_parm3);

	if(authkey.size() != 16)
	{
		ga.irc_ok = false;
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xRED "# authKey nie zawiera oczekiwanych 16 znaków (możliwa zmiana autoryzacji).");
	}

	else
	{
		// wyślij przekonwertowany authKey:
		// AUTHKEY authKey
		irc_send(ga, chan_parm, "AUTHKEY " + authkey, "authIrc3b: ");	// to trzecia b część autoryzacji, dlatego dodano informację o tym przy bł.
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
void raw_809(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xGREEN + raw_parm3 + xNORMAL " jest zajęty(-ta) i nie przyjmuje zaproszeń do rozmów prywatnych.");
}


/*
	811 (INVIGNORE)
	:cf1f2.onet 811 ucc_test Kernel_Panic :Ignore invites
*/
void raw_811(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Zignorowano wszelkie zaproszenia od " + raw_parm3);
}


/*
	812 (INVREJECT)
	:cf1f4.onet 812 ucc_test Kernel_Panic ^cf1f2754610 :Invite rejected
*/
void raw_812(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	// poprawić na odróżnianie pokoi od rozmów prywatnych

	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Odrzucono zaproszenie od " + raw_parm3 + " do " + raw_parm4);
}


/*
	815 (WHOIS)
	:cf1f4.onet 815 ucc_test ucieszony86 :Public webcam
*/
void raw_815(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xGREEN + raw_parm3 + xNORMAL " ma włączoną publiczną kamerkę.");
}


/*
	816 (WHOIS)
	:cf1f4.onet 816 ucc_test ucieszony86 :Private webcam
*/
void raw_816(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xGREEN + raw_parm3 + xNORMAL " ma włączoną prywatną kamerkę.");
}


/*
	817 (historia)
	:cf1f4.onet 817 ucc_test #scc 1401032138 Damian - :bardzo korzystne oferty maja i w sumie dosc rozwiniety jak na Polskie standardy panel chmury
	:cf1f4.onet 817 ucc_test #ucc 1401176793 ucc_test - :\1ACTION test\1
*/
void raw_817(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string action_me = get_value_from_buf(raw_buf, " :\x01" "ACTION ", "\x01");

	// jeśli tak, wyświetl inaczej wiadomość
	if(action_me.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, raw_parm3,
				time_unixtimestamp2local(raw_parm4) + xMAGENTA "* " + raw_parm5 + xNORMAL " " + action_me, 1, false);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm3,
				time_unixtimestamp2local(raw_parm4) + xWHITE "<" + raw_parm5 + ">" xNORMAL " " + get_rest_from_buf(raw_buf, " :"), 1, false);
	}
}


/*
	951
	:cf1f1.onet 951 ucieszony86 ucieszony86 :Added Bot!*@* <privatemessages,channelmessages,invites> to silence list
*/
void raw_951(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// informacja w "Status"
	win_buf_add_str(ga, chan_parm, "Status",
			xYELLOW "* Dodano " + raw_parm5 + " do listy ignorowanych, nie będziesz otrzymywać od niego zaproszeń ani powiadomień.");
}


/*
	Poniżej obsługa RAW NOTICE nienumerycznych.
*/


/*
	NOTICE nienumeryczne
*/
void raw_notice(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf, std::string &raw_parm0)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	std::string nick_notice;

	// jeśli to typowy nick w stylu AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974, to pobierz część przed !
	if(raw_parm0.find("!") != std::string::npos)
	{
		nick_notice = get_value_from_buf(raw_buf, ":", "!");
	}

	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	else
	{
		nick_notice = raw_parm0;
	}

	// :cf1f4.onet NOTICE Auth :*** Looking up your hostname...
	if(raw_parm2 == "Auth" && raw_buf.find(" :*** Looking up your hostname...") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, "Status",
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Wyszukiwanie Twojej nazwy hosta...");
	}

	// :cf1f1.onet NOTICE Auth :*** Found your hostname (eik220.neoplus.adsl.tpnet.pl) -- cached
	else if(raw_parm2 == "Auth" && raw_buf.find(" :*** Found your hostname (" + get_value_from_buf(raw_buf, "hostname (", ") ") + ") -- cached")
		!= std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, "Status",
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Znaleziono Twoją nazwę hosta "
				+ get_value_from_buf(raw_buf, "hostname ", " ") + " -- była zbuforowana na serwerze.");
	}

	// :cf1f3.onet NOTICE Auth :*** Found your hostname (eik220.neoplus.adsl.tpnet.pl)
	else if(raw_parm2 == "Auth" && raw_buf.find(" :*** Found your hostname (" + get_value_from_buf(raw_buf, "hostname (", ")") + ")")
		!= std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, "Status",
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Znaleziono Twoją nazwę hosta "
				+ get_rest_from_buf(raw_buf, "hostname ") + ".");
	}

	// :cf1f2.onet NOTICE Auth :*** Could not resolve your hostname: Domain name not found; using your IP address (93.159.185.10) instead.
	else if(raw_parm2 == "Auth" && raw_buf.find(" :*** Could not resolve your hostname: Domain name not found; using your IP address ("
		+ get_value_from_buf(raw_buf, " address (", ") instead.") + ") instead.") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, "Status",
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL
				" Nie można rozwiązać Twojej nazwy hosta: Nie znaleziono nazwy domeny; zamiast tego użyto Twojego adresu IP "
				+ get_value_from_buf(raw_buf, " address ", " instead.") + ".");
	}

	// :cf1f3.onet NOTICE Auth :Welcome to OnetCzat!
	else if(raw_parm2 == "Auth" && raw_buf.find(" :Welcome to OnetCzat!") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, "Status", "* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Witaj na Onet Czacie!");
	}

	// :cf1f4.onet NOTICE ucc_test :Setting your VHost: ucc
	// ignoruj tę sekwencję dla zwykłych nicków, czyli takich, które mają ! w raw_parm0
	else if(raw_parm0.find("!") == std::string::npos && raw_buf.find(" :Setting your VHost:" + get_rest_from_buf(raw_buf, "VHost:")) != std::string::npos)
	{
		std::string chan;

		// wyświetl w pokoju "Status" jeśli to odpowiedź po zalogowaniu
		if(! ga.command_vhost)
		{
			chan = "Status";
		}

		// wyświetl w aktualnym pokoju, jeśli to odpowiedź po użyciu /vhost
		else
		{
			chan = chan_parm[ga.current]->channel;
		}

		win_buf_add_str(ga, chan_parm, chan,
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Ustawiam Twój VHost:" + get_rest_from_buf(raw_buf, "VHost:"));
	}

	// :cf1f1.onet NOTICE ucieszony86 :Invalid username or password.
	// np. dla VHost
	else if(raw_parm0.find("!") == std::string::npos && raw_buf.find(" :Invalid username or password.") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " Nieprawidłowa nazwa użytkownika lub hasło.");
	}

	// :cf1f1.onet NOTICE ^cf1f1756979 :*** ucc_test invited Kernel_Panic into the channel
	// jeśli to zaproszenie do rozmowy prywatnej, komunikat skieruj do pokoju z tą rozmową (pojawia się po wysłaniu zaproszenia dla nicka)
	else if(raw_parm0.find("!") == std::string::npos && raw_parm2.size() > 0 && raw_parm2[0] == '^'
		&& raw_buf.find(":*** " +  raw_parm4 + " invited " + raw_parm6 + " into the channel") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm2, xWHITE "* Wysłano zaproszenie do rozmowy prywatnej dla " + raw_parm6);
	}

	// :cf1f2.onet NOTICE #Computers :*** drew_barrymore invited aga271980 into the channel
	// jeśli to zaproszenie do pokoju, komunikat skieruj do właściwego pokoju
	else if(raw_parm0.find("!") == std::string::npos && raw_parm2.size() > 0 && raw_parm2[0] == '#'
		&& raw_buf.find(":*** " +  raw_parm4 + " invited " + raw_parm6 + " into the channel") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, raw_parm2,
				xWHITE "* " + raw_parm6 + " został(a) zaproszony(-na) do pokoju " + raw_parm2 + " przez " + raw_parm4);
	}

	// :AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 NOTICE #ucc :test
	// jeśli to wiadomość dla pokoju, a nie nicka, komunikat skieruj do właściwego pokoju
	else if(raw_parm2.size() > 0 && raw_parm2[0] == '#')
	{
		win_buf_add_str(ga, chan_parm, raw_parm2,
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " " + get_rest_from_buf(raw_buf, " :"));
	}

	// jeśli to wiadomość dla nicka (mojego), komunikat skieruj do aktualnie otwartego pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON "-" xMAGENTA + nick_notice + xTERMC "-" xNORMAL " " + get_rest_from_buf(raw_buf, " :"));
	}
}


/*
	Poniżej obsługa RAW NOTICE numerycznych.
*/


/*
	NOTICE 100
	:Onet-Informuje!bot@service.onet NOTICE $* :100 #jakis_pokoj 1401386400 :jakis tekst
*/
void raw_notice_100(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// ogłoszenia serwera wrzucaj do "Status"
	win_buf_add_str(ga, chan_parm, "Status",
			"* " xBOLD_ON "-" xMAGENTA + get_value_from_buf(raw_buf, ":", "!") + xTERMC "-" xNORMAL + " W pokoju " + raw_parm4
			+ " (" + time_unixtimestamp2local_full(raw_parm5) + "): " + get_rest_from_buf(raw_buf, raw_parm5 + " :"));
}


/*
	NOTICE 109
	:GuardServ!service@service.onet NOTICE ucc_test :109 #Suwałki :oszczędzaj enter - pisz w jednej linijce
	:GuardServ!service@service.onet NOTICE ucc_test :109 #Suwałki :rzucanie mięsem nie będzie tolerowane
*/
void raw_notice_109(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, raw_parm4,
			"* " xBOLD_ON "-" xMAGENTA + get_value_from_buf(raw_buf, ":", "!") + xTERMC "-" xNORMAL " "
			+ get_rest_from_buf(raw_buf, raw_parm4 + " :"));
}


/*
	NOTICE 111 (NS INFO nick - początek)
*/
void raw_notice_111(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 avatar :http://foto0.m.ocdn.eu/_m/3e7c4b7dec69eb13ed9f013f1fa2abd4,1,19,0.jpg
	if(raw_parm5 == "avatar")
	{
		ga.card_avatar = get_rest_from_buf(raw_buf, "avatar :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 birthdate :1986-02-12
	else if(raw_parm5 == "birthdate")
	{
		ga.card_birthdate = get_rest_from_buf(raw_buf, "birthdate :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 city :
	else if(raw_parm5 == "city")
	{
		ga.card_city = get_rest_from_buf(raw_buf, "city :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 country :
	else if(raw_parm5 == "country")
	{
		ga.card_country = get_rest_from_buf(raw_buf, "country :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 email :
	else if(raw_parm5 == "email")
	{
		ga.card_email = get_rest_from_buf(raw_buf, "email :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 longDesc :
	else if(raw_parm5 == "longDesc")
	{
		ga.card_long_desc = get_rest_from_buf(raw_buf, "longDesc :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 offmsg :friend
	else if(raw_parm5 == "offmsg")
	{
		ga.card_offmsg = get_rest_from_buf(raw_buf, "offmsg :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 prefs :111000001001110100;1|100|100|0;verdana;006699;14
	else if(raw_parm5 == "prefs")
	{
		ga.card_prefs = get_rest_from_buf(raw_buf, "prefs :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 rank :1.6087
	else if(raw_parm5 == "rank")
	{
		ga.card_rank = get_rest_from_buf(raw_buf, "rank :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 sex :M
	else if(raw_parm5 == "sex")
	{
		ga.card_sex = get_rest_from_buf(raw_buf, "sex :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 shortDesc :Timeout.
	else if(raw_parm5 == "shortDesc")
	{
		ga.card_short_desc = get_rest_from_buf(raw_buf, "shortDesc :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 type :1
	else if(raw_parm5 == "type")
	{
		ga.card_type = get_rest_from_buf(raw_buf, "type :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 vEmail :0
	else if(raw_parm5 == "vEmail")
	{
		ga.card_v_email = get_rest_from_buf(raw_buf, "vEmail :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 www :
	else if(raw_parm5 == "www")
	{
		ga.card_www = get_rest_from_buf(raw_buf, "www :");
	}
}


/*
	NOTICE 112 (NS INFO nick - koniec)
	:NickServ!service@service.onet NOTICE ucieszony86 :112 ucieszony86 :end of user info
*/
void raw_notice_112(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	// wizytówkę wyświetl tylko po użyciu polecenia /card, natomiast ukryj ją po zalogowaniu na czat
	if(ga.command_card)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xBOLD_ON xMAGENTA + raw_parm4 + xTERMC " [Wizytówka]");

		if(ga.card_avatar.size() > 0)
		{
			size_t card_avatar_full = ga.card_avatar.find(",1");

			if(card_avatar_full != std::string::npos)
			{
				ga.card_avatar.replace(card_avatar_full + 1, 1, "0");
			}

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " awatar: " + ga.card_avatar);
		}

		if(ga.card_birthdate.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm4 + xNORMAL " data urodzenia: " + ga.card_birthdate);

			// oblicz wiek (wersja uproszczona zakłada, że data zapisana jest za pomocą 10 znaków łącznie z separatorami)
			if(ga.card_birthdate.size() == 10)
			{
				std::string y_bd_str, m_bd_str, d_bd_str;
				int y_bd, m_bd, d_bd;

				y_bd_str.insert(0, ga.card_birthdate, 0, 4);
				y_bd = std::stoi("0" + y_bd_str);

				m_bd_str.insert(0, ga.card_birthdate, 5, 2);
				m_bd = std::stoi("0" + m_bd_str);

				d_bd_str.insert(0, ga.card_birthdate, 8, 2);
				d_bd = std::stoi("0" + d_bd_str);

				// żadna z liczb nie może być zerem
				if(y_bd != 0 && m_bd != 0 && d_bd != 0)
				{
					int y, m, d, age;

					// pobierz aktualną datę
					time_t time_g;
					struct tm *time_l;

					time(&time_g);
					time_l = localtime(&time_g);

					y = time_l->tm_year + 1900;	// + 1900, bo rok jest liczony od 1900
					m = time_l->tm_mon + 1;		// + 1, bo miesiąc jest od zera
					d = time_l->tm_mday;

					age = y - y_bd;

					if(m <= m_bd && d < d_bd)
					{
						--age;		// wykryj urodziny, jeśli ich jeszcze nie było w danym roku, trzeba odjąć rok
					}

					win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
							"* " xMAGENTA + raw_parm4 + xNORMAL " wiek: " + std::to_string(age));
				}
			}
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

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " płeć: " + ga.card_sex);
		}

		if(ga.card_city.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " miasto: " + ga.card_city);
		}

		if(ga.card_country.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,"* " xMAGENTA + raw_parm4 + xNORMAL " kraj: " + ga.card_country);
		}

		if(ga.card_short_desc.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm4 + xNORMAL " krótki opis: " + ga.card_short_desc);
		}

		if(ga.card_long_desc.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm4 + xNORMAL " długi opis: " + ga.card_long_desc);
		}

		if(ga.card_email.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " email: " + ga.card_email);
		}

		if(ga.card_v_email.size() > 0)
		{
			/* win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm4 + xNORMAL " vEmail: " + ga.card_v_email); */
		}

		if(ga.card_www.size() > 0)
		{
			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " www: " + ga.card_www);
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

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " " + ga.card_offmsg + ".");
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

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xMAGENTA + raw_parm4 + xNORMAL " ranga: " + ga.card_type);
		}

		if(ga.card_prefs.size() > 0)
		{
			/* win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
					"* " xMAGENTA + raw_parm4 + xNORMAL " preferencje: " + ga.card_prefs); */
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
void raw_notice_141(struct global_args &ga, std::string &raw_buf)
{
	if(ga.my_favourites.size() > 0)
	{
		ga.my_favourites += " ";
	}

	ga.my_favourites += get_rest_from_buf(raw_buf, ":141 :");
}


/*
	NOTICE 142 (ulubione pokoje - koniec listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :142 :end of favourites list
*/
void raw_notice_142(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// wejdź do ulubionych pokoi w kolejności alfabetycznej (później dodać opcję wyboru)

	std::string chan, chan_key;
	size_t pos_chan_start, pos_chan_end;

	std::map<std::string, std::string> chanlist;

	std::string chanlist_join;
	bool was_chan;

	// wykryj pierwszy pokój z listy
	pos_chan_start = ga.my_favourites.find("#");

	// jeśli jest pokój na liście, przejdź do pętli wpisującej go na listę w std::map oraz szukającej kolejnych pokoi
	while(pos_chan_start != std::string::npos)
	{
		// znajdź koniec pokoju
		pos_chan_end = ga.my_favourites.find(" ", pos_chan_start);

		// jeśli to był ostatni pokój na liście, nie będzie zawierał spacji za nazwą, wtedy uznaje się, że koniec pokoju to koniec bufora
		if(pos_chan_end == std::string::npos)
		{
			pos_chan_end = ga.my_favourites.size();
		}

		// wyczyść bufor pomocniczy
		chan.clear();

		// wstaw pokój do bufora pomocniczego
		chan.insert(0, ga.my_favourites, pos_chan_start, pos_chan_end - pos_chan_start);

		// w kluczu trzymaj pokój zapisany wielkimi literami (w celu poprawienia sortowania zapewnianego przez std::map)
		chan_key = chan;

		for(int i = 0; i < static_cast<int>(chan_key.size()); ++i)
		{
			if(islower(chan_key[i]))
			{
				chan_key[i] = toupper(chan_key[i]);
			}
		}

		// dodaj pokój do listy w std::map
		chanlist[chan_key] = chan;

		// znajdź kolejny pokój
		pos_chan_start = ga.my_favourites.find("#", pos_chan_start + chan.size());
	}

	// pomiń te pokoje, które już były (po wylogowaniu i ponownym zalogowaniu się, nie dotyczy pierwszego logowania po uruchomieniu programu)
	for(auto it = chanlist.begin(); it != chanlist.end(); ++it)
	{
		was_chan = false;

		for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "Debug" i "RawUnknown"
		{
			if(chan_parm[i] && chan_parm[i]->channel == it->second)
			{
				was_chan = true;
				break;
			}
		}

		if(chanlist_join.size() == 0 && ! was_chan)
		{
			chanlist_join = it->second;
		}

		else if(! was_chan)
		{
			chanlist_join += "," + it->second;	// kolejne pokoje muszą być rozdzielone przecinkiem
		}
	}

	// wejdź do ulubionych po pominięciu ewentualnych pokoi, w których program już był (jeśli jakieś zostały do wejścia)
	if(chanlist_join.size() > 0)
	{
		irc_send(ga, chan_parm, "JOIN " + buf_utf2iso(chanlist_join));
	}

	// po użyciu wyczyść listę
	ga.my_favourites.clear();
}


/*
	NOTICE 151 (CS HOMES - gdzie mamy opcje)
	:ChanServ!service@service.onet NOTICE ucc_test :151 :h#ucc h#Linux h#Suwałki h#scc
*/
void raw_notice_151(struct global_args &ga, std::string &raw_buf)
{
	if(ga.cs_homes.size() > 0)
	{
		ga.cs_homes += " ";
	}

	ga.cs_homes += get_rest_from_buf(raw_buf, ":151 :");
}


/*
	NOTICE 152 (CS HOMES)
	:ChanServ!service@service.onet NOTICE ucc_test :152 :end of homes list
	NOTICE 152 (po zalogowaniu)
	:NickServ!service@service.onet NOTICE ucc_test :152 :end of offline senders list
*/
void raw_notice_152(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	if(raw_buf.find(" :end of homes list") != std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* Pokoje, w których posiadasz uprawnienia: " + ga.cs_homes);

		// po użyciu wyczyść bufor, aby kolejne użycie CS HOMES wpisało wartość od nowa, a nie nadpisało
		ga.cs_homes.clear();
	}

	else if(raw_buf.find(" :end of offline senders list") != std::string::npos)
	{
		// feature
	}

	// gdyby pojawiła się nieoczekiwana wartość, wyświetl ją bez zmian
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				xWHITE "* " + get_value_from_buf(raw_buf, ":", "!") + ": " + get_rest_from_buf(raw_buf, " :152 :"));
	}
}


/*
	NOTICE 210 (NS SET LONGDESC - gdy nie podano opcji do zmiany)
	:NickServ!service@service.onet NOTICE ucieszony86 :210 :nothing changed
*/
void raw_notice_210(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xWHITE "* " + get_value_from_buf(raw_buf, ":", "!") + ": niczego nie zmieniono.");
}


/*
	NOTICE 211 (NS SET SHORTDESC - gdy nie podano opcji do zmiany, a wcześniej była ustawiona jakaś wartość)
	:NickServ!service@service.onet NOTICE ucieszony86 :211 shortDesc :value unset
*/
void raw_notice_211(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xWHITE "* " + get_value_from_buf(raw_buf, ":", "!") + ": " + raw_parm4 + " - wartość wyłączona.");
}


/*
	NOTICE 220 (NS FRIENDS ADD nick)
	:NickServ!service@service.onet NOTICE ucc_test :220 ucieszony86 :friend added to list
*/
void raw_notice_220(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm4 + " został(a) dodany(-na) do listy przyjaciół.");
}


/*
	NOTICE 221 (NS FRIENDS DEL nick)
	:NickServ!service@service.onet NOTICE ucc_test :221 ucieszony86 :friend removed from list
*/
void raw_notice_221(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm4 + " został(a) usunięty(-ta) z listy przyjaciół.");
}


/*
	NOTICE 240 (NS FAVOURITES ADD #pokój)
	:NickServ!service@service.onet NOTICE ucc_test :240 #Linux :favourite added to list
*/
void raw_notice_240(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Pokój " + raw_parm4 + " został dodany do listy ulubionych.");
}


/*
	NOTICE 241 (NS FAVOURITES DEL #pokój)
	:NickServ!service@service.onet NOTICE ucc_test :241 #ucc :favourite removed from list
*/
void raw_notice_241(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Pokój " + raw_parm4 + " został usunięty z listy ulubionych.");
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
void raw_notice_256(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	// przebuduj parametry tak, aby pasowały do funkcji raw_mode(), aby nie dublować tej samej funkcji, poniżej przykładowy RAW z MODE:
	// :ChanServ!service@service.onet MODE #ucc +h ucc_test

	// w ten sposób należy przebudować parametry, aby pasowały do raw_mode()
	// 0 - tu wrzucić 4
	// 1 - bez znaczenia
	// 2 - pozostaje bez zmian
	// 3 - tu wrzucić 5
	// 4 - tu wrzucić 6

	std::string raw_parm0 = get_raw_parm(raw_buf, 4);
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm3 = get_raw_parm(raw_buf, 5);
	std::string raw_parm4 = get_raw_parm(raw_buf, 6);

	std::string raw_buf_new = raw_parm0 + " MODE " + raw_parm2 + " " + raw_parm3 + " " + raw_parm4;	// w miejscu raw_parm1 można wpisać cokolwiek

	raw_mode(ga, chan_parm, raw_buf_new, true);
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
void raw_notice_258(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, chan_parm, raw_parm2, xMAGENTA "* " + raw_parm4 + " zmienił(a) ustawienia pokoju " + raw_parm2 + " (" + raw_parm5 + ").");
}


/*
	NOTICE 259 (np. wpisanie 2x tego samego tematu: CS SET #ucc TOPIC abc)
	:ChanServ!service@service.onet NOTICE ucieszony86 :259 #ucc :nothing changed
*/
void raw_notice_259(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, raw_parm4, xWHITE "* Niczego nie zmieniono w pokoju " + raw_parm4);
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
	:NickServ!service@service.onet NOTICE ucc_test :401  :no such nick
*/
void raw_notice_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string nick_info = get_value_from_buf(raw_buf, " :401 ", " :");

	if(nick_info.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON xMAGENTA + nick_info + xNORMAL " - nie ma takiego nicka na czacie.");
	}

	// drugi RAW powyżej pojawia się po wysłaniu po INFO przynajmniej czterech spacji bez podania dalej nicka
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka.");
	}
}


/*
	NOTICE 402 (CS BAN #pokój ADD anonymous@IP - nieprawidłowa maska)
	:ChanServ!service@service.onet NOTICE ucieszony86 :402 anonymous@IP!*@* :invalid mask
*/
void raw_notice_402(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + raw_parm4 + " - nieprawidłowa maska.");
}


/*
	NOTICE 403 (RS INFO nick)
	:RankServ!service@service.onet NOTICE ucc_test :403 abc :user is not on-line
	:RankServ!service@service.onet NOTICE ucc_test :403  :user is not on-line
*/
void raw_notice_403(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string nick_info = get_value_from_buf(raw_buf, " :403 ", " :");

	if(nick_info.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
				"* " xBOLD_ON xYELLOW_BLACK + nick_info + xNORMAL " - nie ma takiego nicka na czacie.");
	}

	// drugi RAW powyżej pojawia się po wysłaniu po INFO przynajmniej czterech spacji bez podania dalej nicka
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka.");
	}
}


/*
	NOTICE 404 (NS INFO nick - dla nicka tymczasowego)
	:NickServ!service@service.onet NOTICE ucc_test :404 ~złodziej_czasu :user is not registered
*/
void raw_notice_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xMAGENTA + raw_parm4 + xNORMAL " - użytkownik nie jest zarejestrowany (posiada nick tymczasowy).");
}


/*
	NOTICE 406 (NS/CS/RS/GS nieznane_polecenie)
	:NickServ!service@service.onet NOTICE ucc_test :406 ABC :unknown command
*/
void raw_notice_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": " + raw_parm4 + " - nieznane polecenie.");
}


/*
	NOTICE 407 (NS/CS/RS SET)
	:ChanServ!service@service.onet NOTICE ucieszony86 :407 SET :not enough parameters
*/
void raw_notice_407(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": " + raw_parm4 + " - brak wystarczającej liczby parametrów.");
}


/*
	NOTICE 408 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucc_test :408 abc :no such channel
	:ChanServ!service@service.onet NOTICE ucc_test :408  :no such channel
*/
void raw_notice_408(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string chan_info = get_value_from_buf(raw_buf, " :408 ", " :");

	if(chan_info.size() > 0)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xBOLD_ON xBLUE + chan_info + xNORMAL " - nie ma takiego pokoju.");
	}

	// drugi RAW powyżej pojawia się po wysłaniu po INFO przynajmniej czterech spacji bez podania dalej pokoju
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": nie podano pokoju.");
	}
}


/*
	NOTICE 409 (NS SET opcja_do_zmiany - brak argumentu)
	:NickServ!service@service.onet NOTICE ucieszony86 :409 OFFMSG :invalid argument
*/
void raw_notice_409(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": nieprawidłowy argument dla " + raw_parm4);
}


/*
	NOTICE 411 (NS SET ABC)
	:NickServ!service@service.onet NOTICE ucieszony86 :411 ABC :no such setting
*/
void raw_notice_411(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": " + raw_parm4 + " - nie ma takiego ustawienia.");
}


/*
	NOTICE 415 (RS INFO nick - gdy jest online i nie mamy dostępu do informacji)
	:RankServ!service@service.onet NOTICE ucc_test :415 ucieszony86 :permission denied
*/
void raw_notice_415(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			"* " xBOLD_ON xYELLOW_BLACK + raw_parm4 + xNORMAL " - dostęp do informacji zabroniony.");
}


/*
	NOTICE 416 (RS INFO #pokój - gdy nie mamy dostępu do informacji, musimy być w danym pokoju i być minimum opem, aby uzyskać informacje o nim)
	:RankServ!service@service.onet NOTICE ucc_test :416 #ucc :permission denied
*/
void raw_notice_416(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, "* " xBOLD_ON + raw_parm4 + xNORMAL " - dostęp do informacji zabroniony.");
}


/*
	NOTICE 420 (NS FRIENDS ADD nick - gdy nick już dodano do listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :420 legionella :is already on your friend list
*/
void raw_notice_420(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm4 + " jest już na liście przyjaciół.");
}


/*
	NOTICE 421 (NS FRIENDS DEL nick - gdy nicka nie było na liście)
	:NickServ!service@service.onet NOTICE ucieszony86 :421 abc :is not on your friend list
*/
void raw_notice_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* " + raw_parm4 + " nie był(a) dodany(-na) do listy przyjaciół.");
}


/*
	NOTICE 440 (NS FAVOURITES ADD #pokój - gdy pokój już jest na liście ulubionych)
	:NickServ!service@service.onet NOTICE ucieszony86 :440 #Towarzyski :is already on your favourite list
*/
void raw_notice_440(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Pokój " + raw_parm4 + " jest już na liście ulubionych.");
}


/*
	NOTICE 441 (NS FAVOURITES DEL #pokój - gdy pokój nie został dodany do listy ulubionych)
	:NickServ!service@service.onet NOTICE ucieszony86 :441 #Towarzyski :is not on your favourite list
*/
void raw_notice_441(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel, xWHITE "* Pokój " + raw_parm4 + " nie był dodany do listy ulubionych.");
}


/*
	NOTICE 458 (CS BAN #pokój DEL nick@IP - zdjęcie bana w pokoju z uprawnieniami, gdzie nie istnieje taki ban)
	:ChanServ!service@service.onet NOTICE ucieszony86 :458 #pokój b anonymous@IP!*@* :unable to remove non-existent privilege

*/
void raw_notice_458(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	// poprawić na rozróżnianie uprawnień

	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	win_buf_add_str(ga, chan_parm, raw_parm4, xRED "* Nie można usunąć nienadanego uprawnienia dla " + raw_parm6 + " (" + raw_parm5 + ").");
}


/*
	NOTICE 461 (CS BAN #pokój ADD nick - ban na superoperatora/operatora)
	:ChanServ!service@service.onet NOTICE ucieszony86 :461 #ucc ucc :channel operators cannot be banned
*/
void raw_notice_461(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, chan_parm, raw_parm4,
			xRED "* " + raw_parm5 + " jest operatorem lub superoperatorem pokoju " + raw_parm4 + ", nie może zostać zbanowany(-na).");
}


/*
	NOTICE 463
*/
void raw_notice_463(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// CS SET #pokój TOPIC ... - w pokoju bez uprawnień
	// :ChanServ!service@service.onet NOTICE ucc_test :463 #zua_zuy_zuo TOPIC :permission denied, insufficient privileges
	if(raw_parm5 == "TOPIC")
	{
		win_buf_add_str(ga, chan_parm, raw_parm4, xRED "* Nie posiadasz uprawnień do zmiany tematu w pokoju " + raw_parm4);
	}

	// nieznany lub niezaimplementowany powód zmiany ustawień (należy dodać jeszcze te popularne)
	else
	{
		win_buf_add_str(ga, chan_parm, raw_parm4, xRED "* " + raw_parm5 + ": nie posiadasz uprawnień do zmiany tego ustawienia.");
	}
}


/*
	NOTICE 465 (CS SET #pokój nieznane_ustawienie)
	:ChanServ!service@service.onet NOTICE ucc_test :465 ABC :no such setting
*/
void raw_notice_465(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current]->channel,
			xRED "* " + get_value_from_buf(raw_buf, ":", "!") + ": " + raw_parm4 + " - nie ma takiego ustawienia.");
}

/*
	NOTICE 468 (CS BAN #Learning_English ADD anonymous@IP)
	:ChanServ!service@service.onet NOTICE ucieszony86 :468 #Learning_English :permission denied, insufficient privileges
*/
void raw_notice_468(struct global_args &ga, struct channel_irc *chan_parm[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, chan_parm, raw_parm4, xRED "* Dostęp zabroniony, nie posiadasz wystarczających uprawnień w pokoju " + raw_parm4);
}
