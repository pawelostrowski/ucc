/*
	Ucieszony Chat Client
	Copyright (C) 2013-2015 Paweł Ostrowski

	This file is part of Ucieszony Chat Client.

	Ucieszony Chat Client is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	Ucieszony Chat Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Ucieszony Chat Client (in the file LICENSE); if not,
	see <http://www.gnu.org/licenses/gpl-2.0.html>.
*/


#include <sstream>		// std::string, std::stringstream
#include <algorithm>		// std::find
#include <sys/time.h>		// gettimeofday()

// -std=c++11 - std::system(), std::to_string()

#include "irc_parser.hpp"
#include "chat_utils.hpp"
#include "window_utils.hpp"
#include "form_conv.hpp"
#include "enc_str.hpp"
#include "network.hpp"
#include "auth.hpp"
#include "ucc_global.hpp"

#include "kbd_parser.hpp"


std::string get_value_from_buf(std::string &in_buf, std::string expr_before, std::string expr_after)
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

	std::stringstream raw_buf_stream(raw_buf);
	std::string raw_parm;
	int raw_parm_index = 0;

	while(raw_parm_index <= raw_parm_number)
	{
		if(! std::getline(raw_buf_stream, raw_parm, ' '))
		{
			// gdy szukany parametr jest poza buforem, zwróć pusty parametr (normalnie zostałby ostatnio odczytany) i zakończ pętlę
			raw_parm.clear();
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


void irc_parser(struct global_args &ga, struct channel_irc *ci[], std::string dbg_irc_msg, bool test_parser)
{
	std::string irc_recv_buf;

	// normalne użycie parsera najpierw pobiera dane z serwera
	if(! test_parser)
	{
		// pobierz odpowiedź z serwera
		irc_recv(ga, ci, irc_recv_buf, dbg_irc_msg);

		// w przypadku błędu podczas pobierania danych zakończ
		if(! ga.irc_ok)
		{
			return;
		}
	}

	// testowanie parsera pobiera dane z dbg_irc_msg
	else
	{
		irc_recv_buf = dbg_irc_msg;
	}

	std::stringstream irc_recv_buf_stream(irc_recv_buf);

	std::string raw_buf, raw_parm0, raw_parm1;

	bool raw_unknown;

	// obsłuż bufor (wiersz po wierszu)
	while(std::getline(irc_recv_buf_stream, raw_buf))
	{
		// pobierz parametry RAW, aby wykryć dalej, jaki to rodzaj RAW
		raw_parm0 = get_raw_parm(raw_buf, 0);
		raw_parm1 = get_raw_parm(raw_buf, 1);

/*
	Zależnie od rodzaju RAW wywołaj odpowiednią funkcję.
*/
		raw_unknown = false;

/*
	RAW zwykłe nienumeryczne oraz numeryczne.
*/
/*
	RAW zwykłe nienumeryczne, gdzie nazwa RAW jest jako pierwsza.
*/
		if(raw_parm0 == "ERROR")
		{
			raw_error(ga, ci, raw_buf);
		}

		else if(raw_parm0 == "PING")
		{
			raw_ping(ga, ci, raw_parm1);
		}
/*
	Koniec RAW zwykłych nienumerycznych, gdzie nazwa RAW jest jako pierwsza.
*/

		else if(raw_parm1 != "NOTICE")
		{
			switch(std::stoi("0" + raw_parm1))	// drugi parametr RAW
			{

/*
	RAW zwykłe nienumeryczne.
*/
			case 0:
				{
					if(raw_parm1 == "INVIGNORE")
					{
						raw_invignore(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "INVITE")
					{
						raw_invite(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "INVREJECT")
					{
						raw_invreject(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "JOIN")
					{
						raw_join(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "KICK")
					{
						raw_kick(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "MODE")
					{
						raw_mode(ga, ci, raw_buf, raw_parm0);
					}

//					else if(raw_parm1 == "MODERATE")
//					{
//					}

					else if(raw_parm1 == "MODERMSG")
					{
						raw_modermsg(ga, ci, raw_buf);
					}

//					else if(raw_parm1 == "MODERNOTICE")
//					{
//					}

					else if(raw_parm1 == "PART")
					{
						raw_part(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "PONG")
					{
						raw_pong(ga, raw_buf);
					}

					else if(raw_parm1 == "PRIVMSG")
					{
						raw_privmsg(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "QUIT")
					{
						raw_quit(ga, ci, raw_buf);
					}

					else if(raw_parm1 == "TOPIC")
					{
						raw_topic(ga, ci, raw_buf, raw_parm0);
					}

					// nieznany lub niezaimplementowany jeszcze RAW zwykły nienumeryczny
					else
					{
						raw_unknown = true;
					}

					break;

				}	//case 0
/*
	Koniec RAW zwykłych nienumerycznych.
*/

/*
	RAW zwykłe numeryczne.
*/
			case 001:
				raw_001(ga, ci, raw_buf);
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
				raw_251(ga, ci, raw_buf, raw_parm0);
				break;

			case 252:
				raw_252(ga, ci, raw_buf, raw_parm0);
				break;

			case 253:
				raw_253(ga, ci, raw_buf, raw_parm0);
				break;

			case 254:
				raw_254(ga, ci, raw_buf, raw_parm0);
				break;

			case 255:
				raw_255(ga, ci, raw_buf, raw_parm0);
				break;

			case 256:
				raw_256(ga, ci, raw_buf, raw_parm0);
				break;

			case 257:
				raw_257(ga, ci, raw_buf, raw_parm0);
				break;

			case 258:
				raw_258(ga, ci, raw_buf, raw_parm0);
				break;

			case 259:
				raw_259(ga, ci, raw_buf, raw_parm0);
				break;

			case 265:
				raw_265(ga, ci, raw_buf, raw_parm0);
				break;

			case 266:
				raw_266(ga, ci, raw_buf, raw_parm0);
				break;

			case 301:
				raw_301(ga, ci, raw_buf);
				break;

			case 303:
				raw_303(ga, ci, raw_buf);
				break;

			case 304:
				raw_304(ga, ci, raw_buf);
				break;

			case 305:
				raw_305(ga, ci);
				break;

			case 306:
				raw_306(ga, ci);
				break;

			case 307:
				raw_307(ga, ci, raw_buf);
				break;

			case 311:
				raw_311(ga, ci, raw_buf);
				break;

			case 312:
				raw_312(ga, ci, raw_buf);
				break;

			case 313:
				raw_313(ga, ci, raw_buf);
				break;

			case 314:
				raw_314(ga, ci, raw_buf);
				break;

			case 317:
				raw_317(ga, ci, raw_buf);
				break;

			case 318:
				raw_318(ga, ci, raw_buf);
				break;

			case 319:
				raw_319(ga, ci, raw_buf);
				break;

			case 332:
				raw_332(ga, ci, raw_buf, get_raw_parm(raw_buf, 3));
				break;

			case 333:
				raw_333(ga, ci, raw_buf);
				break;

			case 335:
				raw_335(ga, ci, raw_buf);
				break;

			case 341:
				raw_341();
				break;

			case 353:
				raw_353(ga, ci, raw_buf);
				break;

			case 366:
				raw_366(ga, ci, raw_buf);
				break;

			case 369:
				raw_369(ga, ci, raw_buf);
				break;

			case 371:
				raw_371(ga, ci, raw_buf, raw_parm0);
				break;

			case 372:
				raw_372(ga, ci, raw_buf);
				break;

			case 374:
				raw_374();
				break;

			case 375:
				raw_375(ga, ci);
				break;

			case 376:
				raw_376();
				break;

			case 378:
				raw_378(ga, ci, raw_buf);
				break;

			case 391:
				raw_391(ga, ci, raw_buf);
				break;

			case 396:
				raw_396(ga, ci, raw_buf);
				break;

			case 401:
				raw_401(ga, ci, raw_buf);
				break;

			case 402:
				raw_402(ga, ci, raw_buf);
				break;

			case 403:
				raw_403(ga, ci, raw_buf);
				break;

			case 404:
				raw_404(ga, ci, raw_buf);
				break;

			case 405:
				raw_405(ga, ci, raw_buf);
				break;

			case 406:
				raw_406(ga, ci, raw_buf);
				break;

			case 412:
				raw_412(ga, ci);
				break;

			case 421:
				raw_421(ga, ci, raw_buf);
				break;

			case 433:
				raw_433(ga, ci, raw_buf);
				break;

			case 441:
				raw_441(ga, ci, raw_buf);
				break;

			case 442:
				raw_442(ga, ci, raw_buf);
				break;

			case 443:
				raw_443(ga, ci, raw_buf);
				break;

			case 445:
				raw_445(ga, ci, raw_buf);
				break;

			case 446:
				raw_446(ga, ci, raw_buf);
				break;

			case 451:
				raw_451(ga, ci, raw_buf);
				break;

			case 461:
				raw_461(ga, ci, raw_buf);
				break;

			case 462:
				raw_462(ga, ci);
				break;

			case 471:
				raw_471(ga, ci, raw_buf);
				break;

			case 473:
				raw_473(ga, ci, raw_buf);
				break;

			case 474:
				raw_474(ga, ci, raw_buf);
				break;

			case 475:
				raw_475(ga, ci, raw_buf);
				break;

			case 480:
				raw_480(ga, ci, raw_buf);
				break;

			case 481:
				raw_481(ga, ci, raw_buf);
				break;

			case 482:
				raw_482(ga, ci, raw_buf);
				break;

			case 484:
				raw_484(ga, ci, raw_buf);
				break;

			case 492:
				raw_492(ga, ci, raw_buf);
				break;

			case 495:
				raw_495(ga, ci, raw_buf);
				break;

			case 530:
				raw_530(ga, ci, raw_buf);
				break;

			case 531:
				raw_531(ga, ci, raw_buf);
				break;

			case 600:
				raw_600(ga, ci, raw_buf);
				break;

			case 601:
				raw_601(ga, ci, raw_buf);
				break;

			case 602:
				raw_602(ga, raw_buf);
				break;

			case 604:
				raw_604(ga, ci, raw_buf);
				break;

			case 605:
				raw_605(ga, ci, raw_buf);
				break;

			case 607:
				raw_607();
				break;

			case 666:
				raw_666(ga, ci, raw_buf);
				break;

			case 801:
				raw_801(ga, ci, raw_buf);
				break;

			case 807:
				raw_807(ga, ci);
				break;

			case 808:
				raw_808(ga, ci);
				break;

			case 809:
				raw_809(ga, ci, raw_buf);
				break;

			case 811:
				raw_811(ga, ci, raw_buf);
				break;

			case 812:
				raw_812(ga, ci, raw_buf);
				break;

			case 815:
				raw_815(ga, ci, raw_buf);
				break;

			case 816:
				raw_816(ga, ci, raw_buf);
				break;

			case 817:
				raw_817(ga, ci, raw_buf);
				break;

			case 942:
				raw_942(ga, ci, raw_buf);
				break;

			case 950:
				raw_950(ga, ci, raw_buf);
				break;

			case 951:
				raw_951(ga, ci, raw_buf);
				break;

			// nieznany lub niezaimplementowany jeszcze RAW zwykły numeryczny
			default:
				raw_unknown = true;
/*
	Koniec RAW zwykłych numerycznych.
*/

			}	// switch(std::stoi("0" + raw_parm1))

		}	// else if(raw_parm1 != "NOTICE")
/*
	Koniec RAW zwykłych nienumerycznych oraz numerycznych.
*/

/*
	RAW NOTICE nienumeryczne oraz numeryczne.
*/
		else
		{
			switch(std::stoi("0" + get_raw_parm(raw_buf, 3)))	// czwarty parametr RAW
			{

/*
	RAW NOTICE nienumeryczne.
*/
			case 0:
				raw_notice(ga, ci, raw_buf, raw_parm0);
				break;
/*
	Koniec RAW NOTICE nienumerycznych.
*/

/*
	RAW NOTICE numeryczne.
*/
			case 100:
				raw_notice_100(ga, ci, raw_buf);
				break;

			case 109:
				raw_notice_109(ga, ci, raw_buf);
				break;

			case 111:
				raw_notice_111(ga, ci, raw_buf);
				break;

			case 112:
				raw_notice_112(ga, ci, raw_buf);
				break;

			case 121:
				raw_notice_121(ga, raw_buf);
				break;

			case 122:
				raw_notice_122(ga, ci);
				break;

			case 131:
				raw_notice_131(ga, raw_buf);
				break;

			case 132:
				raw_notice_132(ga, ci);
				break;

			case 141:
				raw_notice_141(ga, raw_buf);
				break;

			case 142:
				raw_notice_142(ga, ci);
				break;

			case 151:
				raw_notice_151(ga, raw_buf);
				break;

			case 152:
				raw_notice_152(ga, ci, raw_buf);
				break;

			case 160:
				raw_notice_160(ga, raw_buf);
				break;

			case 161:
				raw_notice_161(ga, raw_buf);
				break;

			case 162:
				raw_notice_162(ga, raw_buf);
				break;

			case 163:
				raw_notice_163(ga, raw_buf);
				break;

			case 164:
				raw_notice_164(ga, ci, raw_buf);
				break;

			case 165:
				raw_notice_165(ga, raw_buf);
				break;

			case 210:
				raw_notice_210(ga, ci, raw_buf);
				break;

			case 211:
				raw_notice_211(ga, ci, raw_buf);
				break;

			case 220:
				raw_notice_220(ga, ci, raw_buf);
				break;

			case 221:
				raw_notice_221(ga, ci, raw_buf);
				break;

			case 230:
				raw_notice_230(ga, ci, raw_buf);
				break;

			case 231:
				raw_notice_231(ga, ci, raw_buf);
				break;

			case 240:
				raw_notice_240(ga, ci, raw_buf);
				break;

			case 241:
				raw_notice_241(ga, ci, raw_buf);
				break;

			case 250:
				raw_notice_250(ga, ci, raw_buf);
				break;

			case 251:
				raw_notice_251(ga, ci, raw_buf);
				break;

			case 252:
				raw_notice_252();
				break;

			case 253:
				raw_notice_253();
				break;

			case 254:
				raw_notice_254(ga, ci, raw_buf);
				break;

			case 255:
				raw_notice_255();
				break;

			case 256:
				raw_notice_256(ga, ci, raw_buf);
				break;

			case 257:
				raw_notice_257();
				break;

			case 258:
				raw_notice_258(ga, ci, raw_buf);
				break;

			case 259:
				raw_notice_259(ga, ci, raw_buf);
				break;

			case 260:
				raw_notice_260(ga, ci, raw_buf);
				break;

			case 261:
				raw_notice_261();
				break;

			case 400:
				raw_notice_400(ga, ci);
				break;

			case 401:
				raw_notice_401(ga, ci, raw_buf);
				break;

			case 402:
				raw_notice_402(ga, ci, raw_buf);
				break;

			case 403:
				raw_notice_403(ga, ci, raw_buf);
				break;

			case 404:
				raw_notice_404(ga, ci, raw_buf);
				break;

			case 406:
				raw_notice_406(ga, ci, raw_buf);
				break;

			case 407:
				raw_notice_407(ga, ci, raw_buf);
				break;

			case 408:
				raw_notice_408(ga, ci, raw_buf);
				break;

			case 409:
				raw_notice_409(ga, ci, raw_buf);
				break;

			case 411:
				raw_notice_411(ga, ci, raw_buf);
				break;

			case 415:
				raw_notice_415(ga, ci, raw_buf);
				break;

			case 416:
				raw_notice_416(ga, ci, raw_buf);
				break;

			case 420:
				raw_notice_420(ga, ci, raw_buf);
				break;

			case 421:
				raw_notice_421(ga, ci, raw_buf);
				break;

			case 430:
				raw_notice_430(ga, ci, raw_buf);
				break;

			case 431:
				raw_notice_431(ga, ci, raw_buf);
				break;

			case 440:
				raw_notice_440(ga, ci, raw_buf);
				break;

			case 441:
				raw_notice_441(ga, ci, raw_buf);
				break;

			case 452:
				raw_notice_452(ga, ci, raw_buf);
				break;

			case 453:
				raw_notice_453(ga, ci, raw_buf);
				break;

			case 454:
				raw_notice_454(ga, ci, raw_buf);
				break;

			case 458:
				raw_notice_458(ga, ci, raw_buf);
				break;

			case 459:
				raw_notice_459(ga, ci, raw_buf);
				break;

			case 461:
				raw_notice_461(ga, ci, raw_buf);
				break;

			case 463:
				raw_notice_463(ga, ci, raw_buf);
				break;

			case 464:
				raw_notice_464(ga, ci, raw_buf);
				break;

			case 465:
				raw_notice_465(ga, ci, raw_buf);
				break;

			case 467:
				raw_notice_467(ga, ci, raw_buf);
				break;

			case 468:
				raw_notice_468(ga, ci, raw_buf);
				break;

			case 470:
				raw_notice_470(ga, ci, raw_buf);
				break;

			case 472:
				raw_notice_472(ga, ci, raw_buf);
				break;

			// nieznany lub niezaimplementowany jeszcze RAW NOTICE numeryczny
			default:
				raw_unknown = true;
/*
	Koniec RAW NOTICE numerycznych.
*/

			}	// switch(std::stoi("0" + get_raw_parm(raw_buf, 3)))

		}	// else
/*
	Koniec RAW NOTICE nienumerycznych oraz numerycznych.
*/

		// nieznany lub niezaimplementowany RAW (każdego typu) wyświetl bez zmian w oknie "RawUnknown" (zostanie utworzone, jeśli nie jest)
		if(raw_unknown)
		{
			new_chan_raw_unknown(ga, ci);	// jeśli istnieje, funkcja nie utworzy ponownie pokoju
			win_buf_add_str(ga, ci, "RawUnknown", xWHITE + raw_buf, true, 2, true, false);		// aby zwrócić uwagę, pokaż aktywność typu 2
		}

	}	// while(std::getline(irc_recv_buf_stream, raw_buf))
}


/*
	Poniżej obsługa RAW ERROR i PING, które występują w odpowiedzi serwera na pierwszej pozycji (w kolejności alfabetycznej).
*/


/*
	ERROR
	ERROR :Closing link (76995189@adei211.neoplus.adsl.tpnet.pl) [Client exited]
	ERROR :Closing link (76995189@adei211.neoplus.adsl.tpnet.pl) [Quit: z/w]
	ERROR :Closing link (unknown@eqw75.neoplus.adsl.tpnet.pl) [Registration timeout]
*/
void raw_error(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	ga.irc_ok = false;

	if(ga.ucc_quit_time)
	{
		ga.ucc_quit = true;
	}

	std::string srv_msg = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	// dodaj przed "]" formatowanie wyłączające ewentualnie użyte w komunikacie formatowanie tekstu (przywracające żółty kolor dla nawiasu zamykającego)
	if(srv_msg.size() > 0 && srv_msg[srv_msg.size() - 1] == ']')
	{
		srv_msg.insert(srv_msg.size() - 1, xNORMAL xYELLOW);
	}

	win_buf_all_chan_msg(ga, ci, (get_value_from_buf(raw_buf, "ERROR :", " (") == "Closing link"
				? oOUTn xYELLOW "Zamykanie połączenia " + get_rest_from_buf(srv_msg, " link ")
				: oOUTn xYELLOW + srv_msg));
}


/*
	PING
	PING :cf1f1.onet
*/
void raw_ping(struct global_args &ga, struct channel_irc *ci[], std::string &raw_parm1)
{
	// odpowiedz PONG na PING
	irc_send(ga, ci, "PONG :" + raw_parm1);

	// normalnie przy wysyłaniu przez ucc PING, serwer sam nie wysyła PING, ale zdarzyło się, że tak zrobił (brak PONG), dlatego na wszelki wypadek
	// odebranie z serwera PING kasuje lag, aby nie zerwało połączenia (aby to działało, timeout w ucc musi być większy z zapasem od PING z serwera)
	ga.lag = 0;
	ga.lag_timeout = false;
}


/*
	Poniżej obsługa RAW nienumerycznych, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności alfabetycznej).
*/


/*
	INVIGNORE
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVIGNORE ucc_test ^cf1f2753898
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVIGNORE ucc_test #ucc
*/
void raw_invignore(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
	std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

	// jeśli użytkownik zignorował zaproszenia do rozmów prywatnych i do pokoi, ale odpowiedź dotyczyła rozmowy prywatnej
	if(raw_parm3.size() > 0 && raw_parm3[0] == '^')
	{
		// informacja w pokoju z rozmową prywatną
		win_buf_add_str(ga, ci, raw_parm3,
				oINFOn xRED + nick_who + " [" + nick_zuo_ip + "] zignorował(a) Twoje wszelkie zaproszenia do rozmów prywatnych oraz do "
				"pokoi.");
	}

	// jeśli użytkownik zignorował zaproszenia do rozmów prywatnych i do pokoi, ale odpowiedź dotyczyła pokoju
	else
	{
		// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
		if(ga.current < CHAN_CHAT)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xRED + nick_who + " [" + nick_zuo_ip + "] zignorował(a) Twoje wszelkie zaproszenia do rozmów prywatnych oraz do "
				"pokoi, w tym do " + raw_parm3);
		}

		// ta sama informacja w "Status"
		win_buf_add_str(ga, ci, "Status",
				oINFOn xRED + nick_who + " [" + nick_zuo_ip + "] zignorował(a) Twoje wszelkie zaproszenia do rozmów prywatnych oraz do "
				"pokoi, w tym do " + raw_parm3);
	}
}


/*
	INVITE
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc INVITE ucc_test :^cf1f1551082
	:ucieszony86!50256503@87edcc.6bc2d5.ee917f.54dae7 INVITE ucc_test :#ucc
*/
void raw_invite(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
	std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

	// jeśli to zaproszenie do rozmowy prywatnej
	if(raw_parm3.size() > 0 && raw_parm3[0] == '^')
	{
		// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
		if(ga.current < CHAN_CHAT)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOb xYELLOW_BLACK + nick_who
					+ " [" + nick_zuo_ip + "] zaprasza Cię do rozmowy prywatnej, aby dołączyć, wpisz " xCYAN "/join " + raw_parm3);
		}

		// ta sama informacja w "Status"
		win_buf_add_str(ga, ci, "Status",
				oINFOb xYELLOW_BLACK + nick_who
				+ " [" + nick_zuo_ip + "] zaprasza Cię do rozmowy prywatnej, aby dołączyć, wpisz " xCYAN "/join " + raw_parm3);
	}

	// jeśli to zaproszenie do pokoju
	else
	{
		// po /join wytnij #, ale nie wycinaj go w pierwszej części zdania, dlatego użyj innego bufora
		std::string chan_tmp = raw_parm3;

		if(chan_tmp.size() > 0)
		{
			chan_tmp.erase(0, 1);
		}

		// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
		if(ga.current < CHAN_CHAT)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOb xYELLOW_BLACK + nick_who
					+ " [" + nick_zuo_ip + "] zaprasza Cię do pokoju " + raw_parm3 + ", aby dołączyć, wpisz " xCYAN "/join " + chan_tmp);
		}

		// ta sama informacja w "Status"
		win_buf_add_str(ga, ci, "Status",
				oINFOb xYELLOW_BLACK + nick_who
				+ " [" + nick_zuo_ip + "] zaprasza Cię do pokoju " + raw_parm3 + ", aby dołączyć, wpisz " xCYAN "/join " + chan_tmp);
	}
}


/*
	INVREJECT
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVREJECT ucc_test ^cf1f1123456
	:Kernel_Panic!78259658@87edcc.6bc2d5.f4e8a2.b9d18c INVREJECT ucc_test #ucc
*/
void raw_invreject(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
	std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

	// jeśli użytkownik odrzucił zaproszenie do rozmowy prywatnej
	if(raw_parm3.size() > 0 && raw_parm3[0] == '^')
	{
		// informacja w pokoju z rozmową prywatną
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED + nick_who + " [" + nick_zuo_ip + "] odrzucił(a) Twoje zaproszenie do rozmowy prywatnej.");
	}

	// jeśli użytkownik odrzucił zaproszenie do pokoju
	else
	{
		// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
		if(ga.current < CHAN_CHAT)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn xRED + nick_who + " [" + nick_zuo_ip + "] odrzucił(a) Twoje zaproszenie do pokoju " + raw_parm3);
		}

		// ta sama informacja w "Status"
		win_buf_add_str(ga, ci, "Status", oINFOn xRED + nick_who + " [" + nick_zuo_ip + "] odrzucił(a) Twoje zaproszenie do pokoju " + raw_parm3);
	}
}


/*
	JOIN
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c JOIN #ucc :rx,0
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc JOIN ^cf1f1551083 :rx,0
*/
void raw_join(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
	std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

	// jeśli to ja wchodzę, utwórz nowy kanał (jeśli wpisano /join, przechodź od razu do tego pokoju - ga.cf.join_priv)
	if(nick_who == ga.zuousername && ! new_chan_chat(ga, ci, raw_parm2, ga.cf.join_priv))
	{
		// w przypadku błędu opuść pokój (i tak nie byłoby widać jego komunikatów)
		irc_send(ga, ci, "PART " + raw_parm2 + " :Channel index out of range.");

		// przełącz na "Status"
		ga.current = CHAN_STATUS;
		ga.win_chat_refresh = true;

		// wyświetl ostrzeżenie
		win_buf_add_str(ga, ci, "Status", uINFOn xRED "Nie udało się wejść do pokoju " + raw_parm2 + " (brak pamięci w tablicy pokoi).");

		return;
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	else if(raw_parm2.size() > 0 && raw_parm2[0] == '^')
	{
		// jeśli to ja dołączam do rozmowy prywatnej, komunikat będzie inny, niż jeśli to ktoś dołącza
		win_buf_add_str(ga, ci, raw_parm2, (nick_who == ga.zuousername
				? oINn xGREEN "Dołączasz do rozmowy prywatnej."
				: oINn xGREEN + nick_who + " [" + nick_zuo_ip + "] dołącza do rozmowy prywatnej."));
	}

	// w przeciwnym razie wyświetl komunikat dla wejścia do pokoju
	else
	{
		win_buf_add_str(ga, ci, raw_parm2, oINn xGREEN + nick_who + " [" + nick_zuo_ip + "] wchodzi do pokoju " + raw_parm2);
	}

	// pobierz wybrane flagi nicka
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

	// dodaj nick do listy
	new_or_update_nick_chan(ga, ci, raw_parm2, nick_who, nick_zuo_ip, flags);

	// jeśli nick ma kamerkę, wyświetl o tym informację
	if(flags.public_webcam)
	{
		win_buf_add_str(ga, ci, raw_parm2, oINFOn xWHITE + nick_who + " [" + nick_zuo_ip + "] posiada włączoną publiczną kamerkę.");
	}

	else if(flags.private_webcam)
	{
		win_buf_add_str(ga, ci, raw_parm2, oINFOn xWHITE + nick_who + " [" + nick_zuo_ip + "] posiada włączoną prywatną kamerkę.");
	}

	// odśwież listę w aktualnie otwartym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła nicka, który też jest w tym pokoju)
	if(ga.win_info_state && ci[ga.current]->channel == raw_parm2)
	{
		ga.win_info_refresh = true;
	}
}


/*
	KICK
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :Zachowuj się!
*/
void raw_kick(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	// pobierz powód, jeśli podano
	std::string reason = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	if(reason.size() > 0)
	{
		reason.insert(0, " [");
		reason += xNORMAL xRED "]";
	}

	// jeśli to mnie wyrzucono, pokaż inny komunikat
	if(raw_parm3 == ga.zuousername)
	{
		// usuń kanał z programu
		del_chan_chat(ga, ci, raw_parm2);

		// komunikat o wyrzuceniu pokaż w "Status"
		win_buf_add_str(ga, ci, "Status", oOUTn xRED "Zostajesz wyrzucony(-na) z pokoju " + raw_parm2 + " przez " + nick_who + reason);
	}

	else
	{
		// klucz nicka trzymany jest wielkimi literami
		std::string nick_key = buf_lower2upper(raw_parm3);

		// jeśli nick wszedł po mnie, to jego ZUO i IP jest na liście, wtedy dodaj je do komunikatu
		std::string nick_zuo_ip;

		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm2)
			{
				auto it = ci[i]->ni.find(nick_key);

				if(it != ci[i]->ni.end())
				{
					nick_zuo_ip = it->second.zuo_ip;
				}

				break;
			}
		}

		// wyświetl powód wyrzucenia w pokoju, w którym wyrzucono nick
		win_buf_add_str(ga, ci, raw_parm2,
				// początek komunikatu (nick)
				oOUTn xRED + raw_parm3 + (nick_zuo_ip.size() > 0
				// jeśli było ZUO i IP
				? " [" + nick_zuo_ip + "] zostaje wyrzucony(-na) z pokoju "
				// jeśli nie było ZUO i IP
				: " zostaje wyrzucony(-na) z pokoju ")
				// reszta komunikatu oraz powód wyrzucenia (jeśli podano)
				+ raw_parm2 + " przez " + nick_who + reason);

		// usuń nick z listy
		del_nick_chan(ga, ci, raw_parm2, raw_parm3);

		// odśwież listę w aktualnie otwartym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła nicka, który też jest w tym pokoju)
		if(ga.win_info_state && ci[ga.current]->channel == raw_parm2)
		{
			ga.win_info_refresh = true;
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
	:Panie_kierowniku!57643619@devel.onet MODE #scc +J 120
	:Panie_kierowniku!57643619@devel.onet MODE #ucc +J 1
	:Panie_kierowniku!57643619@devel.onet MODE #ucc -J
	:GuardServ!service@service.onet MODE #ucc +V
	:ChanServ!service@service.onet MODE #ucc -ips
	:ChanServ!service@service.onet MODE #ucc -e+e-oq+qo *!50256503@* *!80810606@* ucieszony86 ucieszony86 ucc ucc
	:ChanServ!service@service.onet MODE #ucc +ks abc
	:ChanServ!service@service.onet MODE #ucc +l 300
	:ChanServ!service@service.onet MODE #pokój +F 1
	:ChanServ!service@service.onet MODE #nowy_test +il-e 1 *!50256503@*
	:ChanServ!service@service.onet MODE #nowy_test +il-ee 1 *!70914256@* *!50256503@*

	Zmiany flag nicka (przykładowe RAW):
	:Darom!12265854@devel.onet MODE Darom :+O
	:Panie_kierowniku!57643619@devel.onet MODE Panie_kierowniku :+O
	:nick1!80541395@87edcc.6f9b99.6bd006.aee4fc MODE nick1 +W
	:ucc_test!76995189@87edcc.6bc2d5.1917ec.38c71e MODE ucc_test +x
	:NickServ!service@service.onet MODE ucc_test +r
	:cf1f4.onet MODE ucc_test +b
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc MODE Kernel_Panic :+b

	Do zaimplementowania:
	:ChanServ!service@service.onet MODE #ucc +c 1
*/
void raw_mode(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	// wyjaśnienie:
	// nick_gives		- nick, który nadaje/odbiera uprawnienia/statusy
	// nick_receives	- nick, który otrzymuje/któremu zabierane są uprawnienia/statusy

	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	int raw_parm3_len = raw_parm3.size();

	// flagi używane przy wchodzeniu do pokoi, które były otwarte po rozłączeniu, a program nie był zamknięty
	bool my_flag_x = false, my_flag_r = false;

	std::string a, nick_gives, chan_join;

	// sprawdź, czy nick to typowy zapis np. ChanServ!service@service.onet, wtedy pobierz część przed !
	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	nick_gives = (raw_parm0.find("!") != std::string::npos ? get_value_from_buf(raw_buf, ":", "!") : raw_parm0);

	// wykryj, że to zwykły użytkownik czata (a nie np. ChanServ) ustawił daną flagę, dodaj wtedy "(a) " po "ustawił",
	// nick_gives.find(".") == std::string::npos - nie dodawaj "(a) " dla nicka w stylu cf1f4.onet
	if(nick_gives != "ChanServ" && nick_gives != "GuardServ" && nick_gives != "NickServ" && nick_gives.find(".") == std::string::npos)
	{
		a = "(a) ";
	}

	else
	{
		a = " ";
	}

/*
	Zmiany flag pokoju (grupowe, wybrane pozycje).
*/
	if(raw_parm2.size() > 0 && raw_parm2[0] == '#' && (raw_parm3 == "+qo" || raw_parm3 == "+oq") && raw_parm4 == raw_parm5)
	{
		std::string nick_receives_key = buf_lower2upper(raw_parm4);

		win_buf_add_str(ga, ci, raw_parm2,
				oINFOn xMAGENTA + raw_parm4 + " jest teraz właścicielem i superoperatorem pokoju " + raw_parm2
				+ " (ustawił" + a + nick_gives + ").");

		// zaktualizuj flagi
		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm2)
			{
				auto it = ci[i]->ni.find(nick_receives_key);

				if(it != ci[i]->ni.end())
				{
					it->second.nf.owner = true;
					it->second.nf.op = true;

					// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła nicka,
					// który też jest w tym pokoju)
					if(ga.win_info_state && i == ga.current)
					{
						ga.win_info_refresh = true;
					}
				}

				break;
			}
		}
	}

	else if(raw_parm2.size() > 0 && raw_parm2[0] == '#' && (raw_parm3 == "-qo" || raw_parm3 == "-oq") && raw_parm4 == raw_parm5)
	{
		std::string nick_receives_key = buf_lower2upper(raw_parm4);

		win_buf_add_str(ga, ci, raw_parm2,
				oINFOn xMAGENTA + raw_parm4 + " nie jest już właścicielem i superoperatorem pokoju " + raw_parm2
				+ " (ustawił" + a + nick_gives + ").");

		// zaktualizuj flagi
		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm2)
			{
				auto it = ci[i]->ni.find(nick_receives_key);

				if(it != ci[i]->ni.end())
				{
					it->second.nf.owner = false;
					it->second.nf.op = false;

					// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła nicka,
					// który też jest w tym pokoju)
					if(ga.win_info_state && i == ga.current)
					{
						ga.win_info_refresh = true;
					}
				}

				break;
			}
		}
	}

	else if(raw_parm2.size() > 0 && raw_parm2[0] == '#' && raw_parm3 == "+ips")
	{
		win_buf_add_str(ga, ci, raw_parm2,
				oINFOn xMAGENTA "Pokój " + raw_parm2 + " jest teraz niewidoczny, prywatny i sekretny (ustawił" + a + nick_gives + ").");
	}

	else if(raw_parm2.size() > 0 && raw_parm2[0] == '#' && raw_parm3 == "-ips")
	{
		win_buf_add_str(ga, ci, raw_parm2,
				oINFOn xWHITE "Pokój " + raw_parm2 + " nie jest już niewidoczny, prywatny i sekretny (ustawił" + a + nick_gives + ").");
	}

	else
	{
		int s = 0;	// pozycja znaku +/- od początku raw_parm3
		int o = 3;	// offset argumentu względem początku flag (licząc od zera)

		for(int f = 0; f < raw_parm3_len; ++f)
		{
			if(raw_parm3[f] == '+' || raw_parm3[f] == '-')
			{
				s = f;
				continue;	// gdy znaleziono znak + lub -, powróć do początku
			}

/*
	Zmiany flag pokoju (nie trzeba sprawdzać, czy raw_parm2 ma rozmiar większy od zera, bo wejście do tej pętli zawdzięczamy raw_parm3 większemu od zera).
*/
			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'q' && raw_parm3[s] == '+')
			{
				++o;	// flaga z argumentem

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA + nick_receives + " jest teraz właścicielem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.owner = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'q' && raw_parm3[s] == '-')
			{
				++o;	// flaga z argumentem

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie jest już właścicielem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.owner = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'o' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA + nick_receives + " jest teraz superoperatorem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.op = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'o' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie jest już superoperatorem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.op = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'h' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA + nick_receives + " jest teraz operatorem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.halfop = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'h' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie jest już operatorem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.halfop = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'v' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xBLUE + nick_receives + " jest teraz gościem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.voice = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'v' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie jest już gościem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.voice = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'X' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA + nick_receives + " jest teraz moderatorem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.moderator = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'X' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie jest już moderatorem pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");

				// zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->channel == raw_parm2)
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							it->second.nf.moderator = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}

						break;
					}
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'b' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xRED + nick_receives + " otrzymuje bana w pokoju " + raw_parm2 + " (ustawił" + a + nick_gives + ").");

				// wykryj, czy serwer odpowiedział na /kban lub /kbanip, po którym należy wysłać KICK
				std::string kban_nick_key = buf_lower2upper(nick_receives);

				auto it = ga.kb.find(kban_nick_key);

				// it != ga.kb.end()			- czy to nick, który otrzymał ode mnie /kban lub /kbanip
				// ga.zuousername == nick_gives		- czy to ja mu dałem /kban lub /kbanip
				// it->second.chan == raw_parm2		- czy to pokój, w którym otrzymał /kban lub /kbanip
				if(it != ga.kb.end() && ga.zuousername == nick_gives && it->second.chan == raw_parm2)
				{
					irc_send(ga, ci, "KICK " + it->second.chan + " " + it->second.nick + " :" + it->second.reason);

					// po użyciu wyczyść użycie /kban lub /kbanip
					ga.kb.erase(kban_nick_key);
				}
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'b' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie posiada już bana w pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'e' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA + nick_receives + " posiada teraz wyjątek od bana w pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'e' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie posiada już wyjątku od bana w pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'I' && raw_parm3[s] == '+')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA + nick_receives + " jest teraz na liście zaproszonych w pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'I' && raw_parm3[s] == '-')
			{
				++o;

				std::string nick_receives = get_raw_parm(raw_buf, o);

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE + nick_receives + " nie jest już na liście zaproszonych w pokoju " + raw_parm2
						+ " (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'J' && raw_parm3[s] == '+')
			{
				++o;

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " posiada teraz blokadę, ustawioną na "
						+ get_raw_parm(raw_buf, o) + "s, uniemożliwiającą użytkownikom automatyczny powrót po wyrzuceniu "
						"ich z pokoju (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'J' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie posiada już blokady, uniemożliwiającej użytkownikom automatyczny "
						"powrót po wyrzuceniu ich z pokoju (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'V' && raw_parm3[s] == '+')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xRED "Pokój " + raw_parm2 + " posiada teraz blokadę zaproszeń (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'V' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie posiada już blokady zaproszeń (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'm' && raw_parm3[s] == '+')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " jest teraz moderowany (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'm' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie jest już moderowany (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'k' && raw_parm3[s] == '+')
			{
				++o;

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " posiada teraz hasło dostępu: " xBOLD_ON
						+ get_raw_parm(raw_buf, o) + xBOLD_OFF " (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'k' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie posiada już hasła dostępu (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'l' && raw_parm3[s] == '+')
			{
				++o;

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " posiada teraz limit osób jednocześnie przebywających w pokoju "
						"(" + get_raw_parm(raw_buf, o) + ") (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'l' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie posiada już limitu osób jednocześnie przebywających w pokoju "
						"(ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'F' && raw_parm3[s] == '+')
			{
				++o;

				std::string rank = get_raw_parm(raw_buf, o);

				// bez 0, bo + oznacza zwiększanie rangi
				if(rank == "1")
				{
					rank = "Oswojony";
				}

				else if(rank == "2")
				{
					rank = "Z klasą";
				}

				else if(rank == "3")
				{
					rank = "Kultowy";
				}

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " posiada teraz rangę \"" + rank
						+ "\" (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'F' && raw_parm3[s] == '-')
			{
				++o;

				std::string rank = get_raw_parm(raw_buf, o);

				// bez 3, bo - oznacza zmniejszenie rangi
				if(rank == "2")
				{
					rank = "Z klasą";
				}

				else if(rank == "1")
				{
					rank = "Oswojony";
				}

				else if(rank == "0")
				{
					rank = "Dziki";
				}

				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie posiada już rangi \"" + rank
						+ "\" (ustawił" + a + nick_gives + ").");
			}

			// czasem poniższe flagi (ips) pojawiają się oddzielnie, dlatego zostawiono je, mimo wcześniejszego wykrycia +/-ips)
			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'i' && raw_parm3[s] == '+')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " jest teraz niewidoczny (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'i' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie jest już niewidoczny (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'p' && raw_parm3[s] == '+')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " jest teraz prywatny (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 'p' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie jest już prywatny (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 's' && raw_parm3[s] == '+')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xMAGENTA "Pokój " + raw_parm2 + " jest teraz sekretny (ustawił" + a + nick_gives + ").");
			}

			else if(raw_parm2[0] == '#' && raw_parm3[f] == 's' && raw_parm3[s] == '-')
			{
				win_buf_add_str(ga, ci, raw_parm2,
						oINFOn xWHITE "Pokój " + raw_parm2 + " nie jest już sekretny (ustawił" + a + nick_gives + ").");
			}

/*
	Zmiany flag osób.
*/
			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'O' && raw_parm3[s] == '+')
			{
				std::string nick_receives = raw_parm2;
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->ni.find(nick_receives_key) != ci[i]->ni.end())
					{
						win_buf_add_str(ga, ci, ci[i]->channel,
								oINFOn xMAGENTA + nick_receives + " jest teraz netadministratorem czata (ustawił"
								+ a + nick_gives + ").");
					}
				}
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'O' && raw_parm3[s] == '-')
			{
				std::string nick_receives = raw_parm2;
				std::string nick_receives_key = buf_lower2upper(nick_receives);

				// pokaż informację we wszystkich pokojach, gdzie jest dany nick
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i] && ci[i]->ni.find(nick_receives_key) != ci[i]->ni.end())
					{
						win_buf_add_str(ga, ci, ci[i]->channel,
								oINFOn xWHITE + nick_receives + " nie jest już netadministratorem czata (ustawił"
								+ a + nick_gives + ").");
					}
				}
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'W' && raw_parm3[s] == '+')
			{
				std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
				std::string nick_who_key = buf_lower2upper(nick_who);
				std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

				// pokaż informację we wszystkich pokojach, gdzie jest dany nick oraz zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i])
					{
						auto it = ci[i]->ni.find(nick_who_key);

						if(it != ci[i]->ni.end())
						{
							win_buf_add_str(ga, ci, ci[i]->channel,
									oINFOn xWHITE + nick_who + " [" + nick_zuo_ip + "] włącza publiczną kamerkę.");

							// zaktualizuj flagę
							it->second.nf.public_webcam = true;

							// ze względu na obecny brak zmiany flagi V (brak MODE) należy ją wyzerować po włączeniu W
							it->second.nf.private_webcam = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}
					}
				}
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'W' && raw_parm3[s] == '-')
			{
				std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
				std::string nick_who_key = buf_lower2upper(nick_who);
				std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

				// pokaż informację we wszystkich pokojach, gdzie jest dany nick oraz zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i])
					{
						auto it = ci[i]->ni.find(nick_who_key);

						if(it != ci[i]->ni.end())
						{
							win_buf_add_str(ga, ci, ci[i]->channel,
									oINFOn xWHITE + nick_who + " [" + nick_zuo_ip + "] wyłącza publiczną kamerkę.");

							// zaktualizuj flagę
							it->second.nf.public_webcam = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}
					}
				}
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'V' && raw_parm3[s] == '+')
			{
				std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
				std::string nick_who_key = buf_lower2upper(nick_who);
				std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

				// pokaż informację we wszystkich pokojach, gdzie jest dany nick oraz zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i])
					{
						auto it = ci[i]->ni.find(nick_who_key);

						if(it != ci[i]->ni.end())
						{
							win_buf_add_str(ga, ci, ci[i]->channel,
									oINFOn xWHITE + nick_who + " [" + nick_zuo_ip + "] włącza prywatną kamerkę.");

							// zaktualizuj flagę
							it->second.nf.private_webcam = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}
					}
				}
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'V' && raw_parm3[s] == '-')
			{
				std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
				std::string nick_who_key = buf_lower2upper(nick_who);
				std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

				// pokaż informację we wszystkich pokojach, gdzie jest dany nick oraz zaktualizuj flagę
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i])
					{
						auto it = ci[i]->ni.find(nick_who_key);

						if(it != ci[i]->ni.end())
						{
							win_buf_add_str(ga, ci, ci[i]->channel,
									oINFOn xWHITE + nick_who + " [" + nick_zuo_ip + "] wyłącza prywatną kamerkę.");

							// zaktualizuj flagę
							it->second.nf.private_webcam = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}
					}
				}
			}

			// aktualizacja flagi busy na liście nicków
			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'b' && raw_parm3[s] == '+')
			{
				std::string nick_receives_key = buf_lower2upper(raw_parm2);

				// zaktualizuj flagę we wszystkich pokojach, gdzie jest dany nick
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i])
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							// zaktualizuj flagę
							it->second.nf.busy = true;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}
					}
				}
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'b' && raw_parm3[s] == '-')
			{
				std::string nick_receives_key = buf_lower2upper(raw_parm2);

				// zaktualizuj flagę we wszystkich pokojach, gdzie jest dany nick
				for(int i = 0; i < CHAN_CHAT; ++i)
				{
					if(ci[i])
					{
						auto it = ci[i]->ni.find(nick_receives_key);

						if(it != ci[i]->ni.end())
						{
							// zaktualizuj flagę
							it->second.nf.busy = false;

							// odśwież listę w aktualnym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła
							// nicka, który też jest w tym pokoju)
							if(ga.win_info_state && i == ga.current)
							{
								ga.win_info_refresh = true;
							}
						}
					}
				}
			}

			// pokazuj tylko informację o mnie, że mam szyfrowanie IP (w pokoju "Status")
			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'x' && raw_parm3[s] == '+' && raw_parm2 == ga.zuousername)
			{
				std::string nick_ip = get_value_from_buf(raw_buf, "@", " ");

				win_buf_add_str(ga, ci, "Status", oINFOn xGREEN "Twój adres IP jest teraz wyświetlany w formie zaszyfrowanej ("
						+ nick_ip + ").");

				my_flag_x = true;
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'x' && raw_parm3[s] == '-' && raw_parm2 == ga.zuousername)
			{
				std::string nick_ip = get_value_from_buf(raw_buf, "@", " ");

				win_buf_add_str(ga, ci, "Status", oINFOn xRED "Twój adres IP nie jest już wyświetlany w formie zaszyfrowanej ("
						+ nick_ip + ").");
			}

			// pokazuj tylko mój zarejestrowany nick (w pokoju "Status")
			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'r' && raw_parm3[s] == '+' && raw_parm2 == ga.zuousername)
			{
				win_buf_add_str(ga, ci, "Status", oINFOn xGREEN "Jesteś teraz zarejestrowanym użytkownikiem (ustawił"
						+ a + nick_gives + ").");

				my_flag_r = true;
			}

			else if(raw_parm2[0] != '#' && raw_parm3[f] == 'r' && raw_parm3[s] == '-' && raw_parm2 == ga.zuousername)
			{
				win_buf_add_str(ga, ci, "Status", oINFOn xRED "Nie jesteś już zarejestrowanym użytkownikiem (ustawił"
						+ a + nick_gives + ").");
			}

			// nieznane lub niezaimplementowane RAW MODE wyświetl bez zmian w oknie "RawUnknown"
			else
			{
				new_chan_raw_unknown(ga, ci);	// jeśli istnieje, funkcja nie utworzy ponownie pokoju
				win_buf_add_str(ga, ci, "RawUnknown", xWHITE + raw_buf, true, 2, true, false);	// aby zwrócić uwagę, pokaż aktywność typu 2
			}
		}
	}

	// jeśli wylogowaliśmy się, ale nie zamknęliśmy programu i były otwarte jakieś pokoje, wejdź do nich ponownie po zalogowaniu
	// - po +r dla nicka zarejestrowanego
	// - po +x dla nicka tymczasowego
	if(my_flag_r || (my_flag_x && ga.zuousername.size() > 0 && ga.zuousername[0] == '~'))
	{
		for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "DebugIRC" i "RawUnknown"
		{
			if(ci[i] && ci[i]->channel.size() > 0)
			{
				// pierwszy pokój przepisz bez zmian, kolejne pokoje muszą być rozdzielone przecinkiem
				chan_join += (chan_join.size() == 0 ? "" : ",") + ci[i]->channel;
			}
		}
	}

	if(chan_join.size() > 0)
	{
		irc_send(ga, ci, "JOIN " + chan_join);
	}
}


/*
	MODERMSG
	:M_X!36866915@d2d929.646f7c.4180a1.bd429d MODERMSG ucc_test - #Towarzyski :Moderowany?
	:M_X!36866915@d2d929.646f7c.4180a1.bd429d MODERMSG nick1 - #Towarzyski :%Fb%text
*/
void raw_modermsg(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	// wiadomość użytkownika
	std::string user_msg = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	std::string form_start;
	int act_type;

	// jeśli ktoś mnie woła, jego nick wyświetl w żółtym kolorze
	if(user_msg.find(ga.zuousername) != std::string::npos)
	{
		form_start = xYELLOW;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		act_type = 3;

		// testowa wersja dźwięku, do poprawy jeszcze
		if(std::system("aplay -q /usr/share/sounds/pop.wav 2>/dev/null &") != 0) {}
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		act_type = 2;
	}

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string user_msg_action = get_value_from_buf(user_msg, "\x01" "ACTION", "\x01");

	win_buf_add_str(ga, ci, raw_parm4, (user_msg_action.size() > 0 && user_msg_action[0] == ' '
			// tekst pisany z użyciem /me
			? xMAGENTA "* " + form_start + raw_parm2 + xNORMAL + user_msg_action
			// tekst normalny
			: xCYAN + form_start + "<" + raw_parm2 + ">" + xNORMAL " " + user_msg)
			// reszta komunikatu
			+ " " xNORMAL xUNDERLINE_ON xRED "[Moderowany przez " + nick_who + "]", true, act_type);
}


/*
	PART
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc :Bye
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f3561508
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f1552723 :Koniec rozmowy
*/
void raw_part(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
	std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

	// pobierz powód, jeśli podano
	std::string reason = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	if(reason.size() > 0)
	{
		reason.insert(0, " [");
		reason += xNORMAL xCYAN "]";
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	if(raw_parm2.size() > 0 && raw_parm2[0] == '^')
	{
		if(reason.size() == 0)
		{
			reason = ".";
		}

		win_buf_add_str(ga, ci, raw_parm2, (nick_who == ga.zuousername
				// jeśli to ja opuszczam rozmowę prywatną
				? oOUTn xCYAN "Opuszczasz rozmowę prywatną"
				// jeśli osoba, z którą pisałem, opuszcza rozmowę prywatną
				: oOUTn xCYAN + nick_who + " [" + nick_zuo_ip + "] opuszcza rozmowę prywatną")
				// powód wyjścia (gdy podano)
				+ reason);
	}

	// w przeciwnym razie wyświetl komunikat dla wyjścia z pokoju
	else
	{
		win_buf_add_str(ga, ci, raw_parm2, oOUTn xCYAN + nick_who + " [" + nick_zuo_ip + "] wychodzi z pokoju " + raw_parm2 + reason);
	}

	// jeśli to ja wychodzę, usuń kanał z programu
	if(nick_who == ga.zuousername)
	{
		del_chan_chat(ga, ci, raw_parm2);
	}

	else
	{
		// usuń nick z listy
		del_nick_chan(ga, ci, raw_parm2, nick_who);

		// odśwież listę w aktualnie otwartym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła nicka, który też jest w tym pokoju)
		if(ga.win_info_state && ci[ga.current]->channel == raw_parm2)
		{
			ga.win_info_refresh = true;
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
		struct timeval tv_pong;
		int64_t pong_sec, pong_usec;

		gettimeofday(&tv_pong, NULL);
		pong_sec = tv_pong.tv_sec;
		pong_usec = tv_pong.tv_usec;
		ga.pong = (pong_sec * 1000) + (pong_usec / 1000);

		ga.lag = ga.pong - ga.ping;

		ga.lag_timeout = false;
	}
}


/*
	PRIVMSG
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 PRIVMSG #ucc :Hello.
	:Kernel_Panic!78259658@87edcc.6bc2d5.1917ec.38c71e PRIVMSG #ucc :\1ACTION %Cff0000%widzi co się dzieje.\1
*/
void raw_privmsg(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	// wiadomość użytkownika
	std::string user_msg = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	std::string form_start;
	int act_type;

	// jeśli ktoś mnie woła, pogrub jego nick i wyświetl w żółtym kolorze
	if(user_msg.find(ga.zuousername) != std::string::npos)
	{
		form_start = xBOLD_ON xYELLOW_BLACK;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		act_type = 3;

		// testowa wersja dźwięku, do poprawy jeszcze
		if(std::system("aplay -q /usr/share/sounds/pop.wav 2>/dev/null &") != 0) {}
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		act_type = 2;
	}

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string user_msg_action = get_value_from_buf(user_msg, "\x01" "ACTION", "\x01");

	// tekst pisany z użyciem /me
	if(user_msg_action.size() > 0 && user_msg_action[0] == ' ')
	{
		win_buf_add_str(ga, ci, raw_parm2, xBOLD_ON xMAGENTA "* " + form_start + nick_who + xNORMAL + user_msg_action, true, act_type);
	}

	// tekst normalny
	else
	{
		std::string nick_stat;

		// jeśli pokazywanie statusu nicka jest włączone, dodaj je do nicka (busy również ma wpływ na nick)
		if(ga.show_stat_in_win_chat)
		{
			for(int i = 0; i < CHAN_CHAT; ++i)
			{
				if(ci[i] && ci[i]->channel == raw_parm2)
				{
					auto it = ci[i]->ni.find(buf_lower2upper(nick_who));

					if(it != ci[i]->ni.end())
					{
						nick_stat = xNORMAL + get_flags_nick(ga, ci, i, it->first);
					}

					break;
				}
			}
		}

		win_buf_add_str(ga, ci, raw_parm2,
				form_start + "<" + nick_stat + form_start + nick_who
				+ (form_start.size() > 0 ? form_start + ">" xNORMAL " " : xNORMAL "> ") + user_msg, true, act_type);

		// przy włączonym away pokazuj w "Status" odpowiednie powiadomienia, gdy ktoś pisze do mnie
		if(act_type == 3 && ga.my_away)
		{
			win_buf_add_str(ga, ci, "Status",
					"[" + get_time_full() + "] " xGREEN + raw_parm2 + xWHITE ":" xTERMC " " xBOLD_ON xYELLOW_BLACK
					"<" + nick_who + ">" xNORMAL " " + user_msg, true, 3, false);
		}
	}
}


/*
	QUIT
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Client exited
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Quit: Do potem
*/
void raw_quit(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
	std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

	// klucz nicka trzymany jest wielkimi literami
	std::string nick_who_key = buf_lower2upper(nick_who);

	// usuń nick ze wszystkich pokoi z listy, gdzie przebywał i wyświetl w tych pokojach komunikat o tym
	for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "DebugIRC" i "RawUnknown"
	{
		// usuwać można tylko w otwartych pokojach oraz nie usuwaj nicka, jeśli takiego nie było w pokoju
		if(ci[i] && ci[i]->ni.find(nick_who_key) != ci[i]->ni.end())
		{
			// w pokoju, w którym był nick wyświetl komunikat o jego wyjściu
			win_buf_add_str(ga, ci, ci[i]->channel,
					oOUTn xYELLOW + nick_who + " [" + nick_zuo_ip + "] wychodzi z czata ["
					+ form_from_chat(get_rest_from_buf(raw_buf, " :")) + xNORMAL xYELLOW "]");

			// usuń nick z listy
			ci[i]->ni.erase(nick_who_key);

			// odśwież listę w aktualnie otwartym pokoju (o ile włączone jest okno informacyjne oraz zmiana dotyczyła nicka,
			// który też jest w tym pokoju)
			if(ga.win_info_state && i == ga.current)
			{
				ga.win_info_refresh = true;
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
void raw_topic(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	// gdy ustawiono temat przez (tak robi program): CS SET #pokój TOPIC ...
	// wtedy inaczej zwracany jest nick
	if(raw_parm0.find("!") == std::string::npos)
	{
		win_buf_add_str(ga, ci, raw_parm2, oINFOn xMAGENTA + raw_parm0 + " zmienił temat pokoju " + raw_parm2);
	}

	// w przeciwnym razie temat ustawiony przez: TOPIC #pokój :...
	else
	{
		std::string nick_who = get_value_from_buf(raw_buf, ":", "!");
		std::string nick_zuo_ip = get_value_from_buf(raw_buf, "!", " ");

		win_buf_add_str(ga, ci, raw_parm2, oINFOn xMAGENTA + nick_who + " [" + nick_zuo_ip + "] zmienił(a) temat pokoju " + raw_parm2);
	}

	// sam temat również wyświetl i wpisz do bufora tematu kanału, aby wyświetlić go na górnym pasku
	// (reszta jest identyczna jak w obsłudze RAW 332, trzeba tylko zamiast raw_parm3 wysłać raw_parm2)
	raw_332(ga, ci, raw_buf, raw_parm2);
}


/*
	Poniżej obsługa RAW numerycznych, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności numerycznej).
*/


/*
	001
	:cf1f2.onet 001 ucc_test :Welcome to the OnetCzat IRC Network ucc_test!76995189@acvy210.neoplus.adsl.tpnet.pl
*/
void raw_001(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	// pobierz mój numer ZUO
	ga.my_zuo = get_value_from_buf(raw_buf, ga.zuousername + "!", "@");

	// test - znajdź plik vhost.txt, jeśli jest, wczytaj go i wyślij na serwer, aby zmienić swój host (do poprawy jeszcze)
	std::ifstream vhost_file;
	vhost_file.open(ga.user_dir + "/vhost.txt");

	if(vhost_file.good())
	{
		// uwaga - nie jest sprawdzana poprawność zapisu pliku (jedynie, że coś jest w nim), zakłada się, że plik zawiera login i hasło (do poprawy)
		std::string vhost_str;
		std::getline(vhost_file, vhost_str);

		if(vhost_str.size() > 0)
		{
			irc_send(ga, ci, "VHOST " + vhost_str);
		}

		vhost_file.close();
	}
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
void raw_251(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm5 = get_raw_parm(raw_buf, 5);
		std::string raw_parm8 = get_raw_parm(raw_buf, 8);
		std::string raw_parm11 = get_raw_parm(raw_buf, 11);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "There are " + raw_parm5 + " users and " + raw_parm8 + " invisible on " + raw_parm11 + " servers"
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "- Liczba użytkowników: " + raw_parm5 + ", niewidoczni: " + raw_parm8 + ", liczba serwerów: " + raw_parm11
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "- " + srv_msg));
	}
}


/*
	252 (LUSERS)
	:cf1f2.onet 252 ucc_test 8 :operator(s) online
*/
void raw_252(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm3 = get_raw_parm(raw_buf, 3);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "operator(s) online"
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "-" xNORMAL " Zalogowani operatorzy: " + raw_parm3
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "-" xNORMAL " " + raw_parm3 + " " + srv_msg));
	}
}


/*
	253 (LUSERS)
	:cf1f1.onet 253 ucc_test 1 :unknown connections
*/
void raw_253(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm3 = get_raw_parm(raw_buf, 3);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "unknown connections"
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "-" xNORMAL " Nieznane połączenia: " + raw_parm3
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "-" xNORMAL " " + raw_parm3 + " " + srv_msg));
	}
}


/*
	254 (LUSERS)
	:cf1f2.onet 254 ucc_test 2422 :channels formed
*/
void raw_254(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm3 = get_raw_parm(raw_buf, 3);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "channels formed"
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "-" xNORMAL " Utworzone pokoje: " + raw_parm3
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "-" xNORMAL " " + raw_parm3 + " " + srv_msg));
	}
}


/*
	255 (LUSERS)
	:cf1f2.onet 255 ucc_test :I have 478 clients and 1 servers
*/
void raw_255(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm5 = get_raw_parm(raw_buf, 5);
		std::string raw_parm8 = get_raw_parm(raw_buf, 8);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "I have " + raw_parm5 + " clients and " + raw_parm8 + " servers"
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "-" xNORMAL " Posiadana liczba klientów: " + raw_parm5 + ", serwerów: " + raw_parm8
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "-" xNORMAL " " + srv_msg));
	}
}


/*
	256 (ADMIN)
	:cf1f3.onet 256 ucc_test :Administrative info for cf1f3.onet
*/
void raw_256(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, xBOLD_ON "-" xBLUE + raw_parm0 + xTERMC "- " + get_rest_from_buf(raw_buf, " :"));
}


/*
	257 (ADMIN)
	:cf1f3.onet 257 ucc_test :Name     - Czat Admin
*/
void raw_257(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, xBOLD_ON "-" xBLUE + raw_parm0 + xTERMC "-" xNORMAL " " + get_rest_from_buf(raw_buf, " :"));
}


/*
	258 (ADMIN)
	:cf1f3.onet 258 ucc_test :Nickname - czat_admin
*/
void raw_258(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, xBOLD_ON "-" xBLUE + raw_parm0 + xTERMC "-" xNORMAL " " + get_rest_from_buf(raw_buf, " :"));
}


/*
	259 (ADMIN)
	:cf1f3.onet 259 ucc_test :E-Mail   - czat_admin@czat.onet.pl
*/
void raw_259(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, xBOLD_ON "-" xBLUE + raw_parm0 + xTERMC "-" xNORMAL " " + get_rest_from_buf(raw_buf, " :"));
}


/*
	265 (LUSERS)
	:cf1f2.onet 265 ucc_test :Current Local Users: 478  Max: 619
*/
void raw_265(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm6 = get_raw_parm(raw_buf, 6);
		std::string raw_parm8 = get_raw_parm(raw_buf, 8);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "Current Local Users: " + raw_parm6 + "  Max: " + raw_parm8
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "-" xNORMAL " Użytkownicy obecni lokalnie: " + raw_parm6 + ", maksymalnie: " + raw_parm8
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "-" xNORMAL " " + srv_msg));
	}
}


/*
	266 (LUSERS)
	:cf1f2.onet 266 ucc_test :Current Global Users: 1938  Max: 2487
*/
void raw_266(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	if(ga.cf.lusers)
	{
		std::string raw_parm6 = get_raw_parm(raw_buf, 6);
		std::string raw_parm8 = get_raw_parm(raw_buf, 8);

		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				// początek komunikatu (nazwa serwera)
				xBOLD_ON "-" xBLUE + raw_parm0 +
				// wykryj ciąg z RAW powyżej
				(srv_msg == "Current Global Users: " + raw_parm6 + "  Max: " + raw_parm8
				// jeśli szukany ciąg istnieje, przetłumacz go
				? xTERMC "-" xNORMAL " Użytkownicy obecni globalnie: " + raw_parm6 + ", maksymalnie: " + raw_parm8
				// jeśli szukany ciąg nie istnieje (np. został zmieniony), wyświetl komunikat w formie oryginalnej
				: xTERMC "-" xNORMAL " " + srv_msg));
	}
}


/*
	301 (WHOIS - away)
	:cf1f2.onet 301 ucc_test ucc_test :test away
*/
void raw_301(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string away_msg = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	// jeśli użyto /whois, dodaj wynik do bufora
	if(ga.cf.whois)
	{
		// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
		if(ga.whois_short != buf_lower2upper(raw_parm3))
		{
			ga.whois[raw_parm3].push_back(oINFOn "  Jest nieobecny(-na) z powodu: " + away_msg);
		}
	}

	// w przeciwnym razie wyświetl od razu (ma znaczenie np. podczas pisania na priv, gdy osoba ma away)
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm3 + " jest nieobecny(-na) z powodu: " + away_msg);
	}
}


/*
	303 (ISON nick1 nick2 - zwraca nicki, które są online)
	:cf1f1.onet 303 ucc_test :Kernel_Panic
*/
void raw_303(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string ison_list = get_rest_from_buf(raw_buf, " :");

	if(ison_list.size() > 0)
	{
		std::stringstream ison_list_stream(ison_list);
		std::string nick, nicklist_show;

		while(std::getline(ison_list_stream, nick, ' '))
		{
			if(nick.size() > 0)
			{
				nicklist_show += (nicklist_show.size() == 0 ? xCYAN "" : xTERMC ", " xCYAN) + nick;
			}
		}

		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "Użytkownicy dostępni dla zapytania ISON: " + nicklist_show);
	}

	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Brak dostępnych użytkowników dla zapytania ISON.");
	}
}


/*
	304
	:cf1f3.onet 304 ucc_test :SYNTAX PRIVMSG <target>{,<target>} <message>
	:cf1f4.onet 304 ucc_test :SYNTAX NICK <newnick>
*/
void raw_304(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			(raw_buf.find(" :SYNTAX ") != std::string::npos
			// jeśli był ciąg 'SYNTAX', zamień go na 'Składnia:'
			? oINFOn xRED "Składnia:" + get_rest_from_buf(raw_buf, " :SYNTAX")
			// jeśli nie było ciągu 'SYNTAX', wyświetl komunikat bez zmian
			: oINFOn xRED + get_rest_from_buf(raw_buf, " :")));
}


/*
	305 (AWAY - bez wiadomości wyłącza)
	:cf1f1.onet 305 ucc_test :You are no longer marked as being away
*/
void raw_305(struct global_args &ga, struct channel_irc *ci[])
{
	ga.my_away = false;

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Nie jesteś już oznaczony(-na) jako nieobecny(-na).");
}


/*
	306 (AWAY - z wiadomością włącza)
	:cf1f1.onet 306 ucc_test :You have been marked as being away
*/
void raw_306(struct global_args &ga, struct channel_irc *ci[])
{
	ga.my_away = true;

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Jesteś teraz oznaczony(-na) jako nieobecny(-na) z powodu:" xNORMAL " " + ga.away_msg);

	ga.away_msg.clear();
}


/*
	307 (WHOIS)
	:cf1f1.onet 307 ucc_test ucc_test :is a registered nick
*/
void raw_307(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		ga.whois[raw_parm3].push_back(oINFOn "  Jest zarejestrowanym użytkownikiem.");
	}
}


/*
	311 (WHOIS - początek, gdy nick jest na czacie)
	:cf1f2.onet 311 ucc_test AT89S8253 70914256 aaa2a7.a7f7a6.88308b.464974 * :tururu!
	:cf1f1.onet 311 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
	:cf1f2.onet 311 ucc_test ucc_test 76995189 87edcc.30c29e.611774.3140a3 * :ucc_test
*/
void raw_311(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	ga.whois[raw_parm3].push_back(oINFOb xGREEN + raw_parm3 + xTERMC " [" + raw_parm4 + "@" + raw_parm5 + "]");

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		ga.whois[raw_parm3].push_back(oINFOn "  Nazwa: " + get_rest_from_buf(raw_buf, " :"));
	}
}


/*
	312 (WHOIS)
	:cf1f2.onet 312 ucc_test AT89S8253 cf1f3.onet :cf1f3
	:cf1f4.onet 312 ucc_test RankServ rankserv.onet :Ranking service
	312 (WHOWAS)
	:cf1f2.onet 312 ucc_test ucieszony86 cf1f2.onet :Mon May 26 21:39:35 2014
*/
void raw_312(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	// jeśli WHOIS
	if(ga.cf.whois)
	{
		// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
		if(ga.whois_short != buf_lower2upper(raw_parm3))
		{
			ga.whois[raw_parm3].push_back(oINFOn "  Używa serwera: " + raw_parm4 + " [" + get_rest_from_buf(raw_buf, " :") + "]");
		}
	}

	// jeśli WHOWAS
	else
	{
		std::string raw_parm5 = get_raw_parm(raw_buf, 5);
		std::string raw_parm6 = get_raw_parm(raw_buf, 6);
		std::string raw_parm7 = get_raw_parm(raw_buf, 7);
		std::string raw_parm8 = get_raw_parm(raw_buf, 8);
		std::string raw_parm9 = get_raw_parm(raw_buf, 9);

		ga.whowas[raw_parm3].push_back(oINFOn "  Używał(a) serwera: " + raw_parm4);
		ga.whowas[raw_parm3].push_back(oINFOn "  Był(a) zalogowany(-na) od: "
				+ day_en_to_pl(raw_parm5) + ", " + raw_parm7 + " " + month_en_to_pl(raw_parm6) + " " + raw_parm9 + ", " + raw_parm8);
	}
}


/*
	313 (WHOIS)
	:cf1f2.onet 313 ucc_test Onet-KaOwiec :is a Service on OnetCzat
*/
void raw_313(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		ga.whois[raw_parm3].push_back((srv_msg == "is a Service on OnetCzat"
					? oINFOn "  Jest serwisem na Onet Czacie."
					: oINFOn "  " + srv_msg));
	}
}


/*
	314 (WHOWAS - początek, gdy nick był na czacie)
	:cf1f3.onet 314 ucc_test ucieszony86 50256503 e0c697.bbe735.1b1f7f.56f6ee * :ucieszony86
	:cf1f3.onet 314 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
*/
void raw_314(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	ga.whowas[raw_parm3].push_back(oINFOb xCYAN + raw_parm3 + xTERMC " [" + raw_parm4 + "@" + raw_parm5 + "]");
	ga.whowas[raw_parm3].push_back(oINFOn "  Nazwa: " + get_rest_from_buf(raw_buf, " :"));
}


/*
	317 (WHOIS)
	:cf1f1.onet 317 ucc_test AT89S8253 532 1400636388 :seconds idle, signon time
*/
void raw_317(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		std::string raw_parm4 = get_raw_parm(raw_buf, 4);
		std::string raw_parm5 = get_raw_parm(raw_buf, 5);

//		time_t time_g;
//		time(&time_g);
//		int64_t idle_diff = time_g - std::stol(raw_parm4);
//		std::string idle_diff_str = std::to_string(idle_diff);

		ga.whois[raw_parm3].push_back(oINFOn "  Jest nieaktywny(-na) przez: " + time_sec2time(raw_parm4));
//		ga.whois[raw_parm3].push_back(oINFOn "  Jest nieaktywny(-na) od: " + unixtimestamp2local_full(idle_diff_str));
		ga.whois[raw_parm3].push_back(oINFOn "  Jest zalogowany(-na) od: " + unixtimestamp2local_full(raw_parm5));
	}
}


/*
	318 (WHOIS - koniec)
	:cf1f1.onet 318 ucc_test AT89S8253 :End of /WHOIS list.
*/
void raw_318(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	auto it = ga.whois.find(raw_parm3);

	// nie reaguj na /whois dla nicka, którego nie ma na czacie
	if(it != ga.whois.end())
	{
		int whois_len = it->second.size();

		// wyświetl informacje o użytkowniku
		for(int i = 0; i < whois_len; ++i)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, it->second[i]);
		}

		if(ga.whois_short != buf_lower2upper(raw_parm3))
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "Koniec informacji o użytkowniku " xBOLD_ON + it->first);
		}

		// wyczyść informację o nicku po przetworzeniu /whois
		ga.whois.erase(raw_parm3);
	}

	// wyczyść ewentualne użycie opcji 's' we /whois
	ga.whois_short.clear();
}


/*
	319 (WHOIS)
	:cf1f2.onet 319 ucc_test AT89S8253 :@#Learning_English %#Linux %#zua_zuy_zuo @#Computers @#Augustów %#ateizm #scc @#PHP @#ucc @#Suwałki
*/
void raw_319(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		std::map<std::string, std::string> chanlist;

		std::stringstream chanlist_stream(get_rest_from_buf(raw_buf, " :"));
		std::string chan, chan_key, chanlist_show;

		while(std::getline(chanlist_stream, chan, ' '))
		{
			if(chan.size() > 0)
			{
				chan_key = buf_lower2upper(chan);

				// dodanie cyfry na początku klucza posortuje pokoje wg uprawnień osoby
				if(chan[0] == '`')
				{
					// za symbolem uprawnienia wstaw kolor zielony
					if(chan.size() > 1)
					{
						chan.insert(1, xGREEN);
					}

					chanlist["1" + chan_key] = chan;
				}

				else if(chan[0] == '@')
				{
					if(chan.size() > 1)
					{
						chan.insert(1, xGREEN);
					}

					chanlist["2" + chan_key] = chan;
				}

				else if(chan[0] == '%')
				{
					if(chan.size() > 1)
					{
						chan.insert(1, xGREEN);
					}

					chanlist["3" + chan_key] = chan;
				}

				else if(chan[0] == '!')
				{
					if(chan.size() > 1)
					{
						chan.insert(1, xGREEN);
					}

					chanlist["4" + chan_key] = chan;
				}

				else if(chan[0] == '+')
				{
					if(chan.size() > 1)
					{
						chan.insert(1, xGREEN);
					}

					chanlist["5" + chan_key] = chan;
				}

				else
				{
					chanlist["6" + chan_key] = xGREEN + chan;
				}
			}
		}

		for(auto it = chanlist.begin(); it != chanlist.end(); ++it)
		{
			chanlist_show += (chanlist_show.size() == 0 ? "" : xTERMC ", ") + it->second;
		}

		ga.whois[raw_parm3].push_back(oINFOn "  Przebywa w pokojach (" + std::to_string(chanlist.size()) + "): " + chanlist_show);
	}
}


/*
	332 (temat pokoju)
	:cf1f3.onet 332 ucc_test #ucc :Ucieszony Chat Client
*/
void raw_332(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string raw_parm3)
{
	// najpierw wyświetl temat (tylko, gdy jest ustawiony lub informację o braku tematu)
	std::string topic = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	win_buf_add_str(ga, ci, raw_parm3,
			// początek komunikatu
			oINFOn xWHITE "Temat pokoju " xGREEN + raw_parm3 + (buf_chars(topic) > 0
			// gdy temat był ustawiony
			? xWHITE ": " xNORMAL + topic
			// gdy temat nie był ustawiony
			: xWHITE " nie został ustawiony (jest pusty)."));

	// teraz znajdź pokój, do którego należy temat, wpisz go do jego bufora "topic" i wyświetl na górnym pasku
	for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "DebugIRC" i "RawUnknown"
	{
		if(ci[i] && ci[i]->channel == raw_parm3)	// znajdź pokój, do którego należy temat
		{
			// usuń z tematu formatowanie fontu i kolorów (na pasku nie jest obsługiwane)
			ci[i]->topic = remove_form(topic);

			break;
		}
	}
}


/*
	333 (kto i kiedy ustawił temat)
	:cf1f4.onet 333 ucc_test #ucc ucc_test!76995189 1404519605
	:cf1f4.onet 333 ucc_test #ucc ucc_test!76995189@87edcc.30c29e.b9c507.d5c6b7 1400889719
*/
void raw_333(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, ci, raw_parm3,
			oINFOn xWHITE "Temat pokoju " xGREEN + raw_parm3 + xWHITE " ustawiony przez " xCYAN
			+ get_value_from_buf(raw_buf, raw_parm3 + " ", "!") + " [" + get_value_from_buf(raw_buf, "!", " ") + "]" xWHITE
			" (" + unixtimestamp2local_full(raw_parm5) + ").");
}


/*
	335 (WHOIS)
	:cf1f2.onet 335 ucc_test Onet-KaOwiec :is a bot on OnetCzat
*/
void raw_335(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		std::string srv_msg = get_rest_from_buf(raw_buf, " :");

		ga.whois[raw_parm3].push_back((srv_msg == "is a bot on OnetCzat"
					? oINFOn "  Jest botem na Onet Czacie."
					: oINFOn "  " + srv_msg));
	}
}


/*
	341 (INVITE - pojawia się, gdy ja zapraszam; bez informowania, bo podobną informację zwraca RAW NOTICE)
	:cf1f3.onet 341 ucc_test Kernel_Panic #ucc
*/
void raw_341()
{
}


/*
	353 (NAMES)
	:cf1f1.onet 353 ucc_test = #scc :%ucc_test|rx,0 AT89S8253|brx,0 %Husar|rx,1 ~Ayindida|x,0 YouTube_Dekoder|rx,0 StyX1|rx,0 %Radowsky|rx,1 fml|rx,0
	:cf1f3.onet 353 ucieszony86 = ^cf1f2xxxxxx :ucieszony86|rx,1 Kernel_Panic|rx,0
*/
void raw_353(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	if(raw_parm4.size() > 0)
	{
		ga.names[raw_parm4] += get_rest_from_buf(raw_buf, " :");

		// jeśli to NAMES dla rozmowy prywatnej i to ja wchodzę (bo jest już tam nick osoby, z którą będę rozmawiać), to dodaj go do bufora
		if(raw_parm4[0] == '^' && raw_parm6.size() > 0)
		{
			// znajdź, który nick nie jest mój (nie ma żadnej reguły, w jakiej kolejności podane są nicki) i ten wybierz
			std::string nick_priv = (raw_parm5.find(ga.zuousername) == std::string::npos) ? raw_parm5 : raw_parm6;

			// usuń z nicka jego statusy
			size_t nick_stat_pos = nick_priv.find("|");

			if(nick_stat_pos != std::string::npos)
			{
				nick_priv.erase(nick_stat_pos, nick_priv.size() - nick_stat_pos);
			}

			// dodanie nicka, który wyświetli się na dolnym pasku w nazwie pokoju oraz w tytule
			for(int i = 0; i < CHAN_CHAT; ++i)
			{
				if(ci[i] && ci[i]->channel == raw_parm4)
				{
					ci[i]->chan_priv = "^" + nick_priv;

					ci[i]->topic = "Rozmowa prywatna z " + nick_priv;

					// po odnalezieniu pokoju przerwij pętlę
					break;
				}
			}
		}
	}
}


/*
	366 (koniec NAMES)
	:cf1f4.onet 366 ucc_test #scc :End of /NAMES list.
*/
void raw_366(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	int nick_len;
	struct nick_flags flags;
	bool was_open_chan = false;

	std::stringstream names_stream(ga.names[raw_parm3]);
	std::string nick;

	// zmienne używane, gdy wpiszemy /names wraz z podaniem pokoju (w celu wyświetlenia osób w oknie, a nie na liście)
	std::string nick_onet, nick_key;
	std::map<std::string, struct nick_irc> nick_chan;

	// sprawdź, czy szukany pokój jest na liście otwartych pokoi (aby dopisać osoby na listę, gdy wpiszemy /names z podaniem pokoju)
	if(ga.cf.names && ! ga.cf.names_empty)
	{
		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm3)
			{
				was_open_chan = true;
				break;
			}
		}
	}

	while(std::getline(names_stream, nick, ' '))
	{
		nick_len = nick.size();

		// nie reaguj na nadmiarowe spacje, wtedy nick będzie pusty (mało prawdopodobne, ale trzeba się przygotować na wszystko)
		if(nick_len > 0)
		{
			nick_onet = nick;

			// zacznij od wyzerowanych flag
			flags = {};

			// pobierz statusy użytkownika
			for(int i = 0; i < nick_len; ++i)
			{
				if(nick[i] == '`')
				{
					flags.owner = true;
				}

				else if(nick[i] == '@')
				{
					flags.op = true;
				}

				else if(nick[i] == '%')
				{
					flags.halfop = true;
				}

				else if(nick[i] == '!')
				{
					flags.moderator = true;
				}

				else if(nick[i] == '+')
				{
					flags.voice = true;
				}

				// gdy już są znaki inne, niż statusy powyżej, usuń statusy i przerwij pętlę szukającą statusów użytkownika
				else
				{
					nick.erase(0, i);
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

			// jeśli to NAMES po wejściu do pokoju po użyciu /join, wczytaj nicki na listę, tak samo jeśli to NAMES po użyciu /names bez podania
			// pokoju (ma znaczenie przy kamerkach prywatnych, bo serwer nie zwraca obecnie flagi V dla MODE, za wyjątkiem własnego nicka,
			// a dzięki temu można zaktualizować listę osób z kamerkami prywatnymi, bo NAMES zwraca flagę V), wczytaj również nicki, jeśli
			// szukany pokój jest na liście otwartych pokoi, a wpisano /names z podaniem pokoju
			if(! ga.cf.names || ga.cf.names_empty || was_open_chan)
			{
				// ! ga.cf.names	- występuje po /join pokój (wtedy flaga ta jest na false)
				// ga.cf.names_empty	- występuje po /names bez podania pokoju
				// was_open_chan	- występuje po /names z podaniem pokoju, o ile sprawdzany pokój jest na liście otwartych pokoi

				// wpisz nick na listę
				new_or_update_nick_chan(ga, ci, raw_parm3, nick, "", flags);
			}

			// jeśli to NAMES po użyciu /names z podaniem pokoju, pobierz listę nicków, która zostanie później wyświetlona w oknie rozmowy
			// (można podejrzeć listę osób, nie będąc w tym pokoju)
			if(ga.cf.names && ! ga.cf.names_empty)
			{
				// klucz nicka trzymaj wielkimi literami w celu poprawienia sortowania zapewnianego przez std::map
				nick_key = buf_lower2upper(nick);

				// na listę wpisz już nick w formie odebranej przez serwer
				nick_chan[nick_key] = {nick_onet, "", flags};
			}
		}
	}

	// odśwież listę (o ile włączone jest okno informacyjne oraz zmiana dotyczyła aktualnie otwartego pokoju)
	if(ga.win_info_state && ci[ga.current]->channel == raw_parm3)
	{
		ga.win_info_refresh = true;
	}

	// jeśli to było /names bez podania pokoju, pokaż komunikat o aktualizacji listy pokoi
	if(ga.cf.names && ga.cf.names_empty)
	{
		win_buf_add_str(ga, ci, raw_parm3, uINFOn xWHITE "Zaktualizowano listę użytkowników w pokoju " + raw_parm3, false);
	}

	// jeśli wpisano /names wraz z podaniem pokoju, wyświetl w oknie rozmowy nicki (w formie zwracanej przez serwer) w kolejności statusów, alfabetycznie
	else if(ga.cf.names && ! ga.cf.names_empty)
	{
		if(nick_chan.size() > 0)
		{
			std::string nicklist, nick_owner, nick_op, nick_halfop, nick_moderator, nick_voice, nick_pub_webcam, nick_priv_webcam, nick_normal;
			std::string nicklist_part;
			int width = getmaxx(ga.win_chat) - ga.time_len;		// odejmij rozmiar zajmowany przez wyświetlenie czasu
			int nick_len;
			int count_char = 0;
			int spaces;

			for(auto it = nick_chan.begin(); it != nick_chan.end(); ++it)
			{
				if(it->second.nf.owner)
				{
					nick_owner += it->second.nick + "\n";
				}

				else if(it->second.nf.op)
				{
					nick_op += it->second.nick + "\n";
				}

				else if(it->second.nf.halfop)
				{
					nick_halfop += it->second.nick + "\n";
				}

				else if(it->second.nf.moderator)
				{
					nick_moderator += it->second.nick + "\n";
				}

				else if(it->second.nf.voice)
				{
					nick_voice += it->second.nick + "\n";
				}

				else if(it->second.nf.public_webcam)
				{
					nick_pub_webcam += it->second.nick + "\n";
				}

				else if(it->second.nf.private_webcam)
				{
					nick_priv_webcam += it->second.nick + "\n";
				}

				else
				{
					nick_normal += it->second.nick + "\n";
				}
			}

			nicklist = nick_owner + nick_op + nick_halfop + nick_moderator + nick_voice + nick_pub_webcam + nick_priv_webcam + nick_normal;

			std::stringstream nicklist_stream(nicklist);

			std::getline(nicklist_stream, nick);
			nick_len = buf_chars(nick) + 2;		// + 2 na nawias
			bool parity = false;		// co drugi wiersz pokaż jaśniejszym kolorem, aby łatwiej było czytać listę nicków

			while(true)
			{
				if(count_char == 0)
				{
					if(! parity)
					{
						nicklist_part += xTERMC;
						parity = true;
					}

					else
					{
						nicklist_part += xWHITE;
						parity = false;
					}
				}

				nicklist_part += "[" + nick + "]";

				count_char += nick_len;

				spaces = (nick_len <= 20 ? 21 : 42) - nick_len;

				count_char += spaces;

				if(! std::getline(nicklist_stream, nick))
				{
					break;
				}

				nick_len = buf_chars(nick) + 2;

				if(width - count_char > nick_len - 1)
				{
					for(int i = 0; i < spaces; ++i)
					{
						nicklist_part += " ";
					}
				}

				else
				{
					count_char = 0;
					nicklist_part += "\n";
				}
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn xGREEN "Użytkownicy przebywający w pokoju " + raw_parm3 + " (" + std::to_string(nick_chan.size()) + "):");

			win_buf_add_str(ga, ci, ci[ga.current]->channel, nicklist_part);
		}

		else
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xGREEN "Nikt nie przebywa w pokoju " + raw_parm3);
		}
	}

	// po wyświetleniu nicków wyczyść bufor przetwarzanego pokoju
	ga.names.erase(raw_parm3);
}


/*
	369 (WHOWAS - koniec)
	:cf1f3.onet 369 ucc_test ucieszony86 :End of WHOWAS
*/
void raw_369(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	auto it = ga.whowas.find(raw_parm3);

	// nie reaguj na /whowas dla nicka, o którym nie ma informacji
	if(it != ga.whowas.end())
	{
		int whowas_len = it->second.size();

		// wyświetl informacje o użytkowniku
		for(int i = 0; i < whowas_len; ++i)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, it->second[i]);
		}

		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "Koniec informacji o użytkowniku " xBOLD_ON + it->first);

		// wyczyść informacje po przetworzeniu /whowas dla danego użytkownika
		ga.whowas.erase(raw_parm3);
	}
}


/*
	371 (INFO)
	:cf1f1.onet 371 ucieszony86 :                   -/\- InspIRCd -\/-
*/
void raw_371(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			xBOLD_ON "-" xBLUE + raw_parm0
			// jeśli to początek INFO (RAW jak powyżej), pogrub tekst
			+ (srv_msg == "                   -/\\- InspIRCd -\\/-" ? xTERMC "- " : xTERMC "-" xNORMAL " ") + srv_msg);
}


/*
	372 (MOTD - wiadomość dnia, właściwa wiadomość)
	:cf1f2.onet 372 ucc_test :- Onet Czat. Inny Wymiar Czatowania. Witamy!
	:cf1f2.onet 372 ucc_test :- UWAGA - Nie daj się oszukać! Zanim wyślesz jakikolwiek SMS, zapoznaj się z filmem: http://www.youtube.com/watch?v=4skUNAyIN_c
	:cf1f2.onet 372 ucc_test :-
*/
void raw_372(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	win_buf_add_str(ga, ci, "Status", oINFOn xYELLOW + form_from_chat(get_rest_from_buf(raw_buf, " :")));
}


/*
	374 (INFO - koniec)
	:cf1f1.onet 374 ucieszony86 :End of /INFO list
*/
void raw_374()
{
}


/*
	375 (MOTD - wiadomość dnia, początek)
	:cf1f2.onet 375 ucc_test :cf1f2.onet message of the day
*/
void raw_375(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, "Status", oINFOn xYELLOW "Wiadomość dnia:");
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
void raw_378(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		ga.whois[raw_parm3].push_back(oINFOn "  Jest połączony(-na) z: " + get_rest_from_buf(raw_buf, "from "));
	}
}


/*
	391 (TIME)
	:cf1f4.onet 391 ucc_test cf1f4.onet :Tue May 27 08:59:41 2014
*/
void raw_391(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);
	std::string raw_parm7 = get_raw_parm(raw_buf, 7);
	std::string raw_parm8 = get_raw_parm(raw_buf, 8);

	// przekształć zwracaną datę i godzinę na formę poprawną dla polskiego zapisu
	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xWHITE "Data i czas na serwerze " xMAGENTA + raw_parm3 + xWHITE ": "
			+ day_en_to_pl(raw_parm4) + ", " + raw_parm6 + " " + month_en_to_pl(raw_parm5) + " " + raw_parm8 + ", " + raw_parm7);
}


/*
	396
	:cf1f2.onet 396 ucc_test 87edcc.6bc2d5.1917ec.38c71e :is now your displayed host
*/
void raw_396(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// wyświetl w pokoju "Status" jeśli to odpowiedź po zalogowaniu lub w aktualnym pokoju, jeśli to odpowiedź po użyciu /vhost
	// (komunikat zależny od tego, czy udało się pobrać wcześniej ZUO)
	win_buf_add_str(ga, ci, (ga.cf.vhost ? ci[ga.current]->channel : "Status"), (ga.my_zuo.size() > 0
			// gdy udało się wcześniej pobrać ZUO (w zasadzie nie powinno zdarzyć się inaczej)
			? oINFOn xGREEN "Twoim wyświetlanym ZUO oraz hostem jest teraz: " + ga.my_zuo + "@"
			// gdyby jakimś cudem nie udało się wcześniej pobrać numeru ZUO
			: oINFOn xGREEN "Twoim wyświetlanym hostem jest teraz: ")
			// dodaj host
			+ raw_parm3);
}


/*
	401 (WHOIS - początek, gdy nick nie został znaleziony lub gdy podano nieprawidłowy format nicka)
	:cf1f4.onet 401 ucc_test xyz :No such nick/channel
	:cf1f4.onet 401 ucc_test #xyz :No such nick/channel

	401 (INVITE, KICK - podanie nicka, którego nie ma na czacie)
	:cf1f2.onet 401 ucc abc :No such nick/channel

	401 (TOPIC, NAMES, INVITE - przy podaniu nieistniejącego pokoju lub nieprawidłowego pokoju, gdy brakuje #)
	:cf1f4.onet 401 ucc_test #ucc: :No such nick/channel
	:cf1f4.onet 401 ucc_test abc :No such nick/channel

	401 (INVREJECT - błędny nick, podanie nicka, którego nie ma na czacie)
	:cf1f1.onet 401 Kernel_Panic abc :No such nick

	401 (INVREJECT - błędny pokój)
	- odrzucenie nieistniejącej rozmowy prywatnej lub odrzucenie rozmowy prywatnej, gdy nicka nie ma na czacie
	:cf1f1.onet 401 ucc ^cf1f3123456 :No such channel
	- wpisanie nieprawidłowego formatu rozmowy prywatnej
	:cf1f1.onet 401 ucc ^qwerty :No such channel
	- odrzucenie nieistniejącego pokoju
	:cf1f2.onet 401 ucc #abc :No such channel
	- wpisanie nieprawidłowej nazwy pokoju
	:cf1f2.onet 401 ucc abc :No such channel
*/
void raw_401(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	// WHOIS, INVITE lub INVREJECT przy błędnym nicku
	if(ga.cf.whois || (ga.cf.invite && raw_parm3.size() > 0 && raw_parm3[0] != '#') || ga.cf.kick || srv_msg == "No such nick")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb xGREEN + raw_parm3 + xNORMAL " - nie ma takiego użytkownika na czacie.");
	}

	// TOPIC, NAMES itd. przy nieistniejącym lub błędnie wpisanym pokoju
	else if(srv_msg == "No such nick/channel")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_parm3.size() > 0 && raw_parm3[0] == '#'
				 ? oINFOn xRED + raw_parm3 + " - nie ma takiego pokoju."
				 : oINFOn xRED + raw_parm3 + " - nieprawidłowa nazwa pokoju."));
	}

	// INVREJECT przy błędnym pokoju lub PART do rozmowy, która już nie istnieje (po przelogowaniu się bez zamykania tego priva)
	else if(srv_msg == "No such channel")
	{
		// jeśli to PART do rozmowy, która nie istnieje (jeśli ten pokój istnieje), usuń pokój z pamięci programu (zamknij go)
		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm3)
			{
				del_chan_chat(ga, ci, raw_parm3);

				// po odnalezieniu pokoju zakończ
				return;
			}
		}

		// gdy to nie był PART do nieaktywnego priva, wyświetl komunikat
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm3 + " - wybrana rozmowa prywatna nie istnieje.");
	}

	// nieznany powód wyświetl bez zmian
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - " + srv_msg);
	}
}


/*
	402 (MOTD, TIME - podanie nieistniejącego serwera)
	:cf1f4.onet 402 ucc_test abc :No such server
*/
void raw_402(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - nie ma takiego serwera.");
}


/*
	403 (JOIN do nieistniejącego priva lub do pokoju bez podania # na początku)
	:cf1f4.onet 403 ucc_test sc :Invalid channel name
	:cf1f2.onet 403 ucc_test ^cf1f2123456 :Invalid channel name
*/
void raw_403(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	if(raw_parm3.size() > 0 && raw_parm3[0] != '^')
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - nieprawidłowa nazwa pokoju.");
	}

	else
	{
		// jeśli to był otwarty priv i nie da się do niego wejść (po przelogowaniu), pokaż nazwę z nickiem
		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm3)
			{
				win_buf_add_str(ga, ci, raw_parm3, oINFOn xWHITE + ci[i]->chan_priv + " - wybrana rozmowa prywatna nie istnieje.");

				// po odnalezieniu pokoju zakończ
				return;
			}
		}

		// w przeciwnym razie pokaż zwykłe ostrzeżenie w aktualnie otwartym pokoju
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm3 + " - wybrana rozmowa prywatna nie istnieje.");
	}
}


/*
	404 (próba wysłania wiadomości do kanału, w którym nie przebywamy)
	:cf1f1.onet 404 ucc_test #ucc :Cannot send to channel (no external messages)
	404 (próba wysłania wiadomości w pokoju moderowanym, gdzie nie mamy uprawnień)
	:cf1f2.onet 404 ucc_test #Suwałki :Cannot send to channel (+m)
*/
void raw_404(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	if(srv_msg == "Cannot send to channel (no external messages)")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xRED "Nie można wysłać wiadomości do pokoju " + raw_parm3 + " (nie przebywasz w nim).");
	}

	else if(srv_msg == "Cannot send to channel (+m)")
	{
		win_buf_add_str(ga, ci, raw_parm3,
				oINFOn xRED "Nie możesz pisać w pokoju " + raw_parm3 + " (pokój jest moderowany i nie posiadasz uprawnień).");
	}

	// nieznany powód wyświetl bez zmian
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - " + srv_msg);
	}
}


/*
	405 (próba wejścia do zbyt dużej liczby pokoi)
	:cf1f2.onet 405 ucieszony86 #abc :You are on too many channels
*/
void raw_405(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xRED "Nie możesz wejść do pokoju " + raw_parm3 + " (przekroczono dopuszczalny limit jednocześnie otwartych pokoi).");
}


/*
	406 (WHOWAS - początek, gdy nie było takiego nicka)
	:cf1f3.onet 406 ucc_test abc :There was no such nickname
*/
void raw_406(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb xCYAN + raw_parm3 + xNORMAL " - brak informacji o tym użytkowniku.");
}


/*
	412 (PRIVMSG #pokój :)
	:cf1f2.onet 412 ucc_test :No text to send
*/
void raw_412(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie wysłano tekstu.");
}


/*
	421
	:cf1f2.onet 421 ucc_test WHO :This command has been disabled.
	:cf1f2.onet 421 ucc_test ABC :Unknown command
*/
void raw_421(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	// jeśli polecenie jest wyłączone
	if(srv_msg == "This command has been disabled.")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - to polecenie czata zostało wyłączone.");
	}

	// jeśli polecenie jest nieznane
	else if(srv_msg == "Unknown command")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - nieznane polecenie czata.");
	}

	// gdy inna odpowiedź serwera, wyświetl oryginalny tekst
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - " + srv_msg);
	}
}


/*
	433
	:cf1f4.onet 433 * ucc_test :Nickname is already in use.
*/
void raw_433(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xRED "Nick " + raw_parm3 + " jest już w użyciu. Spróbuj wpisać " xCYAN "/connect o " xRED "lub " xCYAN "/c o");

	// w przypadku użycia już nicka, wyślij polecenie rozłączenia się
	irc_send(ga, ci, "QUIT");

	// ustawienie tej flagi spowoduje przerwanie dalszej autoryzacji do IRC
	ga.nick_in_use = true;
}


/*
	441 (KICK #pokój nick :<...> - gdy nie ma nicka w pokoju)
	:cf1f1.onet 441 ucieszony86 Kernel_Panic #ucc :They are not on that channel
*/
void raw_441(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, raw_parm4, oINFOn xRED "Nie możesz wyrzucić " + raw_parm3 + ", ponieważ nie przebywa w pokoju " + raw_parm4);
}


/*
	442 (KICK, INVITE, jeśli nie przebywamy w danym pokoju)
	:cf1f1.onet 442 ucc_test #ucc :You're not on that channel!
*/
void raw_442(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie przebywasz w pokoju " + raw_parm3);
}


/*
	443 (INVITE, gdy osoba już przebywa w tym pokoju)
	:cf1f1.onet 443 ucc_test AT89S8253 #ucc :is already on channel
*/
void raw_443(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, raw_parm4, oINFOn xWHITE + raw_parm3 + " przebywa już w pokoju " + raw_parm4);
}


/*
	445 (SUMMON)
	:cf1f2.onet 445 ucieszony86 :SUMMON has been disabled (depreciated command)
*/
void raw_445(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - to polecenie czata zostało wyłączone (jest przestarzałe).");
}


/*
	446 (USERS)
	:cf1f2.onet 446 ucieszony86 :USERS has been disabled (depreciated command)
*/
void raw_446(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - to polecenie czata zostało wyłączone (jest przestarzałe).");
}


/*
	451
	:cf1f4.onet 451 PING :You have not registered
	:cf1f3.onet 451 VHOST :You have not registered
*/
void raw_451(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm2 + " - nie posiadasz zarejestrowanego nicka.");
}


/*
	461
	:cf1f3.onet 461 ucc_test PRIVMSG :Not enough parameters.
	:cf1f4.onet 461 ucc_test NICK :Not enough parameters.
*/
void raw_461(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - brak wystarczającej liczby parametrów.");
}


/*
	462 (USER)
	:cf1f3.onet 462 ucc :You may not reregister
*/
void raw_462(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie możesz zarejestrować się ponownie.");
}


/*
	471 (JOIN gdy pokój jest pełny)
	:cf1f2.onet 471 Kernel_Panic #ucc :Cannot join channel (Channel is full)
*/
void raw_471(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xRED "Nie możesz wejść do pokoju " + raw_parm3 + " (przekroczono limit osób jednocześnie przebywających w pokoju).");
}


/*
	473 (JOIN do pokoju, który jest prywatny i nie mam zaproszenia)
	:cf1f2.onet 473 ucc_test #a :Cannot join channel (Invite only)
*/
void raw_473(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie możesz wejść do pokoju " + raw_parm3 + " (nie posiadasz zaproszenia).");
}


/*
	474 (JOIN do pokoju, w którym mam bana)
	:cf1f3.onet 474 ucc #ucc :Cannot join channel (You're banned)
*/
void raw_474(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie możesz wejść do pokoju " + raw_parm3 + " (jesteś zbanowany(-na)).");
}


/*
	475 (JOIN do pokoju z hasłem gdy podamy złe hasło lub jego brak)
	:cf1f2.onet 475 ucieszony86 #ucc :Cannot join channel (Incorrect channel key)
*/
void raw_475(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie możesz wejść do pokoju " + raw_parm3 + " (nieprawidłowe hasło dostępu).");
}


/*
	480 (KNOCK #pokój :text) - obecnie wyłączony, dlatego komunikat nie będzie tłumaczony, dodano na wszelki wypadek, gdyby KNOCK kiedyś wrócił
	:cf1f2.onet 480 Kernel_Panic :Can't KNOCK on #ucc, channel is not invite only so knocking is pointless!
*/
void raw_480(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + srv_msg);
}


/*
	481 (SQUIT, CONNECT, TRACE, KILL, REHASH, DIE, RESTART, WALLOPS, KLINE - przy braku uprawnień, to otrzyma każdy, kto nie jest adminem)
	:cf1f1.onet 481 Kernel_Panic :Permission Denied - You do not have the required operator privileges
*/
void raw_481(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (srv_msg == "Permission Denied - You do not have the required operator privileges"
			? oINFOn xRED "Dostęp zabroniony, nie posiadasz wymaganych uprawnień operatora."
			: oINFOn xRED + srv_msg));
}


/*
	482
*/
void raw_482(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	// TOPIC
	// :cf1f1.onet 482 Kernel_Panic #Suwałki :You must be at least a half-operator to change the topic on this channel
	if(srv_msg == "You must be at least a half-operator to change the topic on this channel")
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED "Nie posiadasz uprawnień do zmiany tematu w pokoju " + raw_parm3);
	}

	// KICK sopa, będąc opem
	// :cf1f2.onet 482 ucc_test #ucc :You must be a channel operator
	else if(srv_msg == "You must be a channel operator")
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED "Musisz być przynajmniej superoperatorem pokoju " + raw_parm3);
	}

	// KICK sopa lub opa, nie mając żadnych uprawnień
	// :cf1f3.onet 482 ucc_test #irc :You must be a channel half-operator
	else if(srv_msg == "You must be a channel half-operator")
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED "Musisz być przynajmniej operatorem pokoju " + raw_parm3);
	}

	// KICK np. ChanServ
	// :cf1f4.onet 482 ucc_test #ucc :Only a u-line may kick a u-line from a channel.
	// a tego nie będę tłumaczyć, niech wyświetli się w else

	// nieznany lub niezaimplementowany powód wyświetl bez zmian
	else
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED + srv_msg);
	}
}


/*
	484
*/
void raw_484(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	std::string nick_kick = get_value_from_buf(srv_msg, "kick ", " as");

	// KICK #pokój nick :<...> - próba wyrzucenia właściciela
	// :cf1f1.onet 484 ucieszony86 #ucc :Can't kick ucieszony86 as they're a channel founder
	if(srv_msg == "Can't kick " + nick_kick + " as they're a channel founder")
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED "Nie możesz wyrzucić " + nick_kick + ", ponieważ jest właścicielem pokoju " + raw_parm3);
	}

	// KICK #pokój nick :<...> - próba wyrzucenia sopa przez innego sopa, opa przez innego opa lub nicka bez uprawnień, gdy sami ich nie posiadamy
	// :cf1f1.onet 484 ucieszony86 #Computers :Can't kick AT89S8253 as your spells are not good enough
	else if(srv_msg == "Can't kick " + nick_kick + " as your spells are not good enough")
	{
		win_buf_add_str(ga, ci, raw_parm3,
				oINFOn xRED "Nie posiadasz wystarczających uprawnień, aby wyrzucić " + nick_kick + " z pokoju " + raw_parm3);
	}

	// nieznany powód wyświetl bez zmian
	else
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED + srv_msg);
	}
}


/*
	492 (INVITE nick #pokój - przy blokadzie zaproszeń)
	:cf1f4.onet 492 ucc_test #ucc :Can't invite Kernel_Panic to channel (+V set)
*/
void raw_492(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	if(srv_msg == "Can't invite " + raw_parm6 + " to channel (+V set)")
	{
		win_buf_add_str(ga, ci, raw_parm3,
				oINFOn xRED "Nie możesz zaprosić " + raw_parm6 + " do pokoju " + raw_parm3 + " (ustawiona blokada zaproszeń).");
	}

	else
	{
		win_buf_add_str(ga, ci, raw_parm3, oINFOn xRED + srv_msg);
	}
}


/*
	495 (JOIN #pokój - gdy wyrzucono mnie i jest aktywny kickrejoin)
	:cf1f1.onet 495 Kernel_Panic #ucc :You cannot rejoin this channel yet after being kicked (+J)
*/
void raw_495(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	if(srv_msg == "You cannot rejoin this channel yet after being kicked (+J)")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xRED "Nie możesz wejść do pokoju " + raw_parm3
				+ " (posiada blokadę uniemożliwiającą użytkownikom automatyczny powrót przez określony czas po wyrzuceniu ich z pokoju).");
	}

	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - " + srv_msg);
	}
}


/*
	530 (JOIN #sc - nieistniejący pokój)
	:cf1f3.onet 530 ucc_test #sc :Only IRC operators may create new channels
*/
void raw_530(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - nie ma takiego pokoju.");
}


/*
	531 (PRIVMSG user :text, NOTICE user :text)
	:cf1f1.onet 531 Kernel_Panic AT89S8253 :You are not permitted to send private messages to this user
*/
void raw_531(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie posiadasz uprawnień do wysłania wiadomości prywatnej do " + raw_parm3);
}


/*
	600
	:cf1f2.onet 600 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :arrived online
*/
void raw_600(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
	if(ga.current < CHAN_CHAT)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xMAGENTA "Twój przyjaciel " xBOLD_ON + raw_parm3 + xBOLD_OFF " [" + raw_parm4 + "@" + raw_parm5
				+ "] pojawia się na czacie.");
	}

	// informacja w "Status" wraz z datą i godziną
	win_buf_add_str(ga, ci, "Status",
			oINFOn xMAGENTA "Twój przyjaciel " xBOLD_ON + raw_parm3 + xBOLD_OFF " [" + raw_parm4 + "@" + raw_parm5
			+ "] pojawia się na czacie (" + unixtimestamp2local_full(raw_parm6) +  ").");

	// dodaj nick do listy zalogowanych przyjaciół
	ga.my_friends_online.push_back(raw_parm3);
}


/*
	601
	:cf1f2.onet 601 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337863 :went offline
*/
void raw_601(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
	if(ga.current < CHAN_CHAT)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xWHITE "Twój przyjaciel " xBOLD_ON xTERMC + raw_parm3 + xBOLD_OFF xWHITE " [" + raw_parm4 + "@" + raw_parm5
				+ "] wychodzi z czata.");
	}

	// informacja w "Status" wraz z datą i godziną
	win_buf_add_str(ga, ci, "Status",
			oINFOn xWHITE "Twój przyjaciel " xBOLD_ON xTERMC + raw_parm3 + xBOLD_OFF xWHITE " [" + raw_parm4 + "@" + raw_parm5
			+ "] wychodzi z czata (" + unixtimestamp2local_full(raw_parm6) + ").");

	// usuń nick z listy zalogowanych przyjaciół
	auto it = std::find(ga.my_friends_online.begin(), ga.my_friends_online.end(), raw_parm3);

	if(it != ga.my_friends_online.end())
	{
		ga.my_friends_online.erase(it);
	}
}


/*
	602
	:cf1f2.onet 602 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :stopped watching
	:cf1f2.onet 602 ucc_test ucieszony86 * * 0 :stopped watching
*/
void raw_602(struct global_args &ga, std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// usuń nick z listy zalogowanych przyjaciół
	auto it = std::find(ga.my_friends_online.begin(), ga.my_friends_online.end(), raw_parm3);

	if(it != ga.my_friends_online.end())
	{
		ga.my_friends_online.erase(it);
	}
}


/*
	604
	:cf1f2.onet 604 ucc_test ucieszony86 50256503 87edcc.6bc2d5.4c33d7.76ada2 1401337308 :is online
*/
void raw_604(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// wyświetl w pokoju "Status" lub w aktywnym po użyciu /friends bez parametrów
	win_buf_add_str(ga, ci, (ga.cf.friends ? ci[ga.current]->channel : "Status"),
			oINFOn xMAGENTA "Twój przyjaciel " + raw_parm3 + " [" + raw_parm4 + "@" + raw_parm5 + "] jest na czacie od: "
			+ unixtimestamp2local_full(raw_parm6));

	// dodaj nick do listy zalogowanych przyjaciół
	ga.my_friends_online.push_back(raw_parm3);
}


/*
	605
	:cf1f1.onet 605 ucc nick1 * * 0 :is offline
*/
void raw_605(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// wyświetl w pokoju "Status" lub w aktywnym po użyciu /friends bez parametrów
	win_buf_add_str(ga, ci, (ga.cf.friends ? ci[ga.current]->channel : "Status"),
			oINFOn xWHITE "Twojego przyjaciela " + raw_parm3 + " nie ma na czacie.");
}


/*
	607 (WATCH)
	:cf1f4.onet 607 ucieszony86 :End of WATCH list
*/
void raw_607()
{
}


/*
	666 (SERVER)
	:cf1f2.onet 666 ucieszony86 :You cannot identify as a server, you are a USER. IRC Operators informed.
*/
void raw_666(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			(srv_msg == "You cannot identify as a server, you are a USER. IRC Operators informed."
			// dla komunikatu powyżej pokaż przetłumaczoną wersję
			? oINFOn xRED "Nie możesz zidentyfikować się jako serwer, jesteś użytkownikiem. Operatorzy IRC poinformowani."
			// w przeciwnym razie pokaż oryginalny tekst (już nie na czerwono)
			: oINFOn xWHITE + srv_msg));
}


/*
	801 (AUTHKEY)
	:cf1f4.onet 801 ucc_test :authKey
*/
void raw_801(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string authkey = get_raw_parm(raw_buf, 3);

	// konwersja authKey
	auth_code(authkey);

	if(authkey.size() != 16)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "authKey nie zawiera oczekiwanych 16 znaków (możliwa zmiana autoryzacji).");

		// wyślij polecenie rozłączenia się
		irc_send(ga, ci, "QUIT");

		// ustawienie tej flagi spowoduje przerwanie dalszej autoryzacji do IRC
		ga.authkey_failed = true;
	}

	else
	{
		// wyślij przekonwertowany authKey:
		// AUTHKEY authKey
		irc_send(ga, ci, "AUTHKEY " + authkey, "authIrc3b: ");	// to 3b część autoryzacji, dlatego dodano informację o tym przy błędzie
	}
}


/*
	807 (BUSY 1)
	:cf1f3.onet 807 ucc_test :You are marked as busy
*/
void raw_807(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xWHITE "Jesteś teraz oznaczony(-na) jako zajęty(-ta) i nie będziesz otrzymywać zaproszeń do rozmów prywatnych.");

	ga.my_busy = true;
}


/*
	808 (BUSY 0)
	:cf1f3.onet 808 ucc_test :You are no longer marked busy
*/
void raw_808(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xWHITE "Nie jesteś już oznaczony(-na) jako zajęty(-ta) i możesz otrzymywać zaproszenia do rozmów prywatnych.");

	ga.my_busy = false;
}


/*
	809 (WHOIS)
	:cf1f2.onet 809 ucc_test AT89S8253 :is busy
*/
void raw_809(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// jeśli użyto /whois, dodaj wynik do bufora
	if(ga.cf.whois)
	{
		// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
		if(ga.whois_short != buf_lower2upper(raw_parm3))
		{
			ga.whois[raw_parm3].push_back(oINFOn "  Jest zajęty(-ta) i nie przyjmuje zaproszeń do rozmów prywatnych.");
		}
	}

	// w przeciwnym razie wyświetl od razu (ma znaczenie podczas zapraszania na priv)
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xWHITE + raw_parm3 + " jest zajęty(-ta) i nie przyjmuje zaproszeń do rozmów prywatnych.");
	}
}


/*
	811 (INVIGNORE)
	:cf1f2.onet 811 ucc_test Kernel_Panic :Ignore invites
*/
void raw_811(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Zignorowano wszelkie zaproszenia od " + raw_parm3);
}


/*
	812 (INVREJECT)
	:cf1f4.onet 812 ucc_test Kernel_Panic ^cf1f2754610 :Invite rejected
*/
void raw_812(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	// poprawić na odróżnianie pokoi od rozmów prywatnych

	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Odrzucono zaproszenie od " + raw_parm3 + " do " + raw_parm4);
}


/*
	815 (WHOIS)
	:cf1f4.onet 815 ucc_test ucieszony86 :Public webcam
*/
void raw_815(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		ga.whois[raw_parm3].push_back(oINFOn "  Posiada włączoną publiczną kamerkę.");
	}
}


/*
	816 (WHOIS)
	:cf1f4.onet 816 ucc_test ucieszony86 :Private webcam
*/
void raw_816(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	// ukryj informację o użytkowniku, jeśli użyto opcji 's' we /whois
	if(ga.whois_short != buf_lower2upper(raw_parm3))
	{
		ga.whois[raw_parm3].push_back(oINFOn "  Posiada włączoną prywatną kamerkę.");
	}
}


/*
	817 (historia)
	:cf1f4.onet 817 ucc_test #scc 1401032138 Damian - :bardzo korzystne oferty maja i w sumie dosc rozwiniety jak na Polskie standardy panel chmury
	:cf1f4.onet 817 ucc_test #ucc 1401176793 ucc_test - :\1ACTION test\1
*/
void raw_817(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	std::string user_msg = form_from_chat(get_rest_from_buf(raw_buf, " :"));

	// wykryj, gdy ktoś pisze przez użycie /me
	std::string user_msg_action = get_value_from_buf(user_msg, "\x01" "ACTION", "\x01");

	win_buf_add_str(ga, ci, raw_parm3,
			// początek komunikatu (czas)
			unixtimestamp2local(raw_parm4) + (user_msg_action.size() > 0 && user_msg_action[0] == ' '
			// tekst pisany z użyciem /me
			? xMAGENTA "* " + raw_parm5 + xNORMAL + user_msg_action
			// tekst normalny
			: xBOLD_ON xDARK "<" + raw_parm5 + ">" xNORMAL " " + user_msg), true, 2, false);
}


/*
	942 (np. gdy przyjaciel usunie swój nick)
	:cf1f1.onet 942 ucieszony86 nick_porzucony.123456 :Invalid nickname
*/
void raw_942(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm3 = get_raw_parm(raw_buf, 3);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + raw_parm3 + " - nieprawidłowy nick.");
}


/*
	950 (NS IGNORE DEL nick)
	:cf1f2.onet 950 ucc ucc :Removed ucc_test!*@* <privatemessages,channelmessages,invites> from silence list
*/
void raw_950(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xWHITE "Usunięto " + raw_parm5 + " z listy ignorowanych, możesz teraz otrzymywać od niego zaproszenia oraz powiadomienia.");
}


/*
	951 (NS IGNORE ADD nick, a także po zalogowaniu)
	:cf1f1.onet 951 ucieszony86 ucieszony86 :Added Bot!*@* <privatemessages,channelmessages,invites> to silence list
*/
void raw_951(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// informacja w "Status" lub w aktywnym pokoju po użyciu /ignore bez parametrów
	win_buf_add_str(ga, ci, (ga.cf.ignore ? ci[ga.current]->channel : "Status"),
			oINFOn xYELLOW "Dodano " + raw_parm5 + " do listy ignorowanych, nie będziesz otrzymywać od niego zaproszeń ani powiadomień.");
}


/*
	Poniżej obsługa RAW NOTICE nienumerycznych.
*/


/*
	NOTICE nienumeryczny
*/
void raw_notice(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	// jeśli to typowy nick w stylu AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974, pobierz część przed !
	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	std::string nick_who = (raw_parm0.find("!") != std::string::npos ? get_value_from_buf(raw_buf, ":", "!") : raw_parm0);

	// pobierz zwracany komunikat serwera
	std::string srv_msg = get_rest_from_buf(raw_buf, " :");

	// :cf1f4.onet NOTICE Auth :*** Looking up your hostname...
	if(raw_parm2 == "Auth" && srv_msg == "*** Looking up your hostname...")
	{
		win_buf_add_str(ga, ci, "Status", xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " Wyszukiwanie Twojej nazwy hosta...");
	}

	// :cf1f3.onet NOTICE Auth :*** Found your hostname (eik220.neoplus.adsl.tpnet.pl)
	else if(raw_parm2 == "Auth" && srv_msg == "*** Found your hostname (" + get_value_from_buf(srv_msg, "(", ")") + ")")
	{
		win_buf_add_str(ga, ci, "Status",
				xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " Znaleziono Twoją nazwę hosta "
				+ get_rest_from_buf(srv_msg, "hostname ") + ".");
	}

	// :cf1f1.onet NOTICE Auth :*** Found your hostname (eik220.neoplus.adsl.tpnet.pl) -- cached
	else if(raw_parm2 == "Auth" && srv_msg == "*** Found your hostname (" + get_value_from_buf(srv_msg, "(", ") ") + ") -- cached")
	{
		win_buf_add_str(ga, ci, "Status",
				xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " Znaleziono Twoją nazwę hosta "
				+ get_value_from_buf(srv_msg, "hostname ", " ") + " - była zbuforowana na serwerze.");
	}

	// :cf1f2.onet NOTICE Auth :*** Could not resolve your hostname: Domain name not found; using your IP address (93.159.185.10) instead.
	else if(raw_parm2 == "Auth" && srv_msg == "*** Could not resolve your hostname: Domain name not found; using your IP address ("
		+ get_value_from_buf(srv_msg, "(", ")") + ") instead.")
	{
		win_buf_add_str(ga, ci, "Status",
				xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " Nie można rozwiązać Twojej nazwy hosta (nie znaleziono nazwy "
				"domeny). Zamiast tego użyto Twojego adresu IP " + get_value_from_buf(srv_msg, " address ", " instead.") + ".");
	}

	// :cf1f3.onet NOTICE Auth :Welcome to OnetCzat!
	else if(raw_parm2 == "Auth" && srv_msg == "Welcome to OnetCzat!")
	{
		win_buf_add_str(ga, ci, "Status",
				xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " " + ga.zuousername + ", witaj na Onet Czacie!");
	}

	// :cf1f4.onet NOTICE ucc_test :Setting your VHost: ucc
	// ignoruj tę sekwencję dla zwykłych nicków, czyli takich, które mają ! w raw_parm0
	else if(raw_parm0.find("!") == std::string::npos && srv_msg == "Setting your VHost:" + get_rest_from_buf(srv_msg, "VHost:"))
	{
		// wyświetl w aktualnie otwartym pokoju, jeśli to odpowiedź po użyciu /vhost lub w "Status", jeśli to odpowiedź po zalogowaniu się
		win_buf_add_str(ga, ci, (ga.cf.vhost ? ci[ga.current]->channel : "Status"),
				xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " Ustawiam Twój VHost:" + get_rest_from_buf(srv_msg, "VHost:"));
	}

	// :cf1f1.onet NOTICE ucieszony86 :Invalid username or password.
	// np. dla VHost
	// ignoruj tę sekwencję dla zwykłych nicków, czyli takich, które mają ! w raw_parm0
	else if(raw_parm0.find("!") == std::string::npos && srv_msg == "Invalid username or password.")
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " Nieprawidłowa nazwa użytkownika lub hasło.");
	}

	// :cf1f1.onet NOTICE ^cf1f1756979 :*** ucc_test invited Kernel_Panic into the channel
	// jeśli to zaproszenie do rozmowy prywatnej, komunikat skieruj do pokoju z tą rozmową (pojawia się po wysłaniu zaproszenia dla nicka);
	// ignoruj tę sekwencję dla zwykłych nicków, czyli takich, które mają ! w raw_parm0
	else if(raw_parm0.find("!") == std::string::npos && raw_parm2.size() > 0 && raw_parm2[0] == '^'
		&& srv_msg == "*** " +  raw_parm4 + " invited " + raw_parm6 + " into the channel")
	{
		win_buf_add_str(ga, ci, raw_parm2, oINFOn xWHITE "Wysłano zaproszenie do rozmowy prywatnej dla " + raw_parm6);

		// dodanie nicka, który wyświetli się na dolnym pasku w nazwie pokoju oraz w tytule
		for(int i = 0; i < CHAN_CHAT; ++i)
		{
			if(ci[i] && ci[i]->channel == raw_parm2)
			{
				ci[i]->chan_priv = "^" + raw_parm6;

				ci[i]->topic = "Rozmowa prywatna z " + raw_parm6;

				// po odnalezieniu pokoju przerwij pętlę
				break;
			}
		}
	}

	// :cf1f2.onet NOTICE #Computers :*** drew_barrymore invited aga271980 into the channel
	// jeśli to zaproszenie do pokoju, komunikat skieruj do właściwego pokoju;
	// ignoruj tę sekwencję dla zwykłych nicków, czyli takich, które mają ! w raw_parm0
	else if(raw_parm0.find("!") == std::string::npos && raw_parm2.size() > 0 && raw_parm2[0] == '#'
		&& srv_msg == "*** " +  raw_parm4 + " invited " + raw_parm6 + " into the channel")
	{
		win_buf_add_str(ga, ci, raw_parm2, oINFOn xWHITE + raw_parm6 + " został(a) zaproszony(-na) do pokoju " + raw_parm2 + " przez " + raw_parm4);
	}

	// :AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 NOTICE #ucc :test
	// jeśli to wiadomość dla pokoju, a nie nicka, komunikat skieruj do właściwego pokoju
	else if(raw_parm2.size() > 0 && raw_parm2[0] == '#')
	{
		win_buf_add_str(ga, ci, raw_parm2, xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " " + form_from_chat(srv_msg));
	}

	// jeśli to wiadomość dla nicka (mojego), komunikat skieruj do aktualnie otwartego pokoju
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " " + form_from_chat(srv_msg));
	}
}


/*
	Poniżej obsługa RAW NOTICE numerycznych.
*/


/*
	NOTICE 100
	:Onet-Informuje!bot@service.onet NOTICE $* :100 #jakis_pokoj 1401386400 :jakis tekst
*/
void raw_notice_100(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	// ogłoszenia serwera wrzucaj do "Status"
	win_buf_add_str(ga, ci, "Status",
			xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL + " W pokoju " + raw_parm4
			+ " (" + unixtimestamp2local_full(raw_parm5) + "): " + form_from_chat(get_rest_from_buf(raw_buf, raw_parm5 + " :")));
}


/*
	NOTICE 109
	:GuardServ!service@service.onet NOTICE ucc_test :109 #Suwałki :oszczędzaj enter - pisz w jednej linijce
	:GuardServ!service@service.onet NOTICE ucc_test :109 #Suwałki :rzucanie mięsem nie będzie tolerowane
*/
void raw_notice_109(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, raw_parm4,
			xBOLD_ON "-" xMAGENTA + nick_who + xTERMC "-" xNORMAL " " + form_from_chat(get_rest_from_buf(raw_buf, raw_parm4 + " :")));
}


/*
	NOTICE 111 (NS INFO nick - początek)
*/
void raw_notice_111(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 avatar :http://foto0.m.ocdn.eu/_m/3e7c4b7dec69eb13ed9f013f1fa2abd4,1,19,0.jpg
	if(raw_parm5 == "avatar")
	{
		ga.co[raw_parm4].avatar = get_rest_from_buf(raw_buf, "avatar :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 birthdate :1986-02-12
	else if(raw_parm5 == "birthdate")
	{
		ga.co[raw_parm4].birthdate = get_rest_from_buf(raw_buf, "birthdate :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 city :
	else if(raw_parm5 == "city")
	{
		ga.co[raw_parm4].city = get_rest_from_buf(raw_buf, "city :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 country :
	else if(raw_parm5 == "country")
	{
		ga.co[raw_parm4].country = get_rest_from_buf(raw_buf, "country :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 email :
	else if(raw_parm5 == "email")
	{
		ga.co[raw_parm4].email = get_rest_from_buf(raw_buf, "email :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 longDesc :
	else if(raw_parm5 == "longDesc")
	{
		ga.co[raw_parm4].long_desc = form_from_chat(get_rest_from_buf(raw_buf, "longDesc :"));
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 offmsg :friend
	else if(raw_parm5 == "offmsg")
	{
		ga.co[raw_parm4].offmsg = get_rest_from_buf(raw_buf, "offmsg :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 prefs :111000001001110100;1|100|100|0;verdana;006699;14
	else if(raw_parm5 == "prefs")
	{
		ga.co[raw_parm4].prefs = get_rest_from_buf(raw_buf, "prefs :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 rank :1.6087
	else if(raw_parm5 == "rank")
	{
		ga.co[raw_parm4].rank = get_rest_from_buf(raw_buf, "rank :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 sex :M
	else if(raw_parm5 == "sex")
	{
		ga.co[raw_parm4].sex = get_rest_from_buf(raw_buf, "sex :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 shortDesc :Timeout.
	else if(raw_parm5 == "shortDesc")
	{
		ga.co[raw_parm4].short_desc = form_from_chat(get_rest_from_buf(raw_buf, "shortDesc :"));
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 type :1
	else if(raw_parm5 == "type")
	{
		ga.co[raw_parm4].type = get_rest_from_buf(raw_buf, "type :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 vEmail :0
	else if(raw_parm5 == "vEmail")
	{
		ga.co[raw_parm4].v_email = get_rest_from_buf(raw_buf, "vEmail :");
	}

	// :NickServ!service@service.onet NOTICE ucieszony86 :111 ucieszony86 www :
	else if(raw_parm5 == "www")
	{
		ga.co[raw_parm4].www = get_rest_from_buf(raw_buf, "www :");
	}

	// nieznany typ danych w wizytówce pokaż w "RawUnknown"
	else
	{
		new_chan_raw_unknown(ga, ci);	// jeśli istnieje, funkcja nie utworzy ponownie pokoju
		win_buf_add_str(ga, ci, "RawUnknown", xWHITE + raw_buf, true, 2, true, false);		// aby zwrócić uwagę, pokaż aktywność typu 2
	}
}


/*
	NOTICE 112 (NS INFO nick - koniec)
	:NickServ!service@service.onet NOTICE ucieszony86 :112 ucieszony86 :end of user info
*/
void raw_notice_112(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	// wyświetl wizytówkę tylko po użyciu polecenia /card, natomiast ukryj ją po zalogowaniu na czat
	if(ga.cf.card)
	{
		auto it = ga.co.find(raw_parm4);

		if(it != ga.co.end())
		{
			std::string card_color;

			// kolor nicka zależny od płci
			if(it->second.sex.size() > 0 && it->second.sex == "M")
			{
				card_color = xBLUE;
			}

			else if(it->second.sex.size() > 0 && it->second.sex == "F")
			{
				card_color = xMAGENTA;
			}

			// gdy płeć nie jest ustawiona, nick będzie ciemnoszary
			else
			{
				card_color = xDARK;
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb + card_color + raw_parm4 + xTERMC " [Wizytówka]");

			if(it->second.avatar.size() > 0)
			{
				size_t card_avatar_full = it->second.avatar.find(",1");

				if(card_avatar_full != std::string::npos)
				{
					it->second.avatar.replace(card_avatar_full + 1, 1, "0");
				}

				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Awatar: " + it->second.avatar);
			}

			if(it->second.sex.size() > 0)
			{
				if(it->second.sex == "M")
				{
					it->second.sex = "mężczyzna";
				}

				else if(it->second.sex == "F")
				{
					it->second.sex = "kobieta";
				}

				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Płeć: " + it->second.sex);
			}

			if(it->second.birthdate.size() > 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Data urodzenia: " + it->second.birthdate);

				// oblicz wiek (wersja uproszczona zakłada, że data zapisana jest za pomocą 10 znaków łącznie z separatorami)
				if(it->second.birthdate.size() == 10)
				{
					std::string y_bd_str, m_bd_str, d_bd_str;

					y_bd_str.insert(0, it->second.birthdate, 0, 4);
					int y_bd = std::stoi("0" + y_bd_str);

					m_bd_str.insert(0, it->second.birthdate, 5, 2);
					int m_bd = std::stoi("0" + m_bd_str);

					d_bd_str.insert(0, it->second.birthdate, 8, 2);
					int d_bd = std::stoi("0" + d_bd_str);

					// żadna z liczb nie może być zerem
					if(y_bd != 0 && m_bd != 0 && d_bd != 0)
					{
						// pobierz aktualną datę
						time_t time_g;
						struct tm *time_l;

						time(&time_g);
						time_l = localtime(&time_g);

						int y = time_l->tm_year + 1900;		// + 1900, bo rok jest liczony od 1900
						int m = time_l->tm_mon + 1;		// + 1, bo miesiąc jest od zera
						int d = time_l->tm_mday;

						int age = y - y_bd;

						// wykryj urodziny, jeśli ich jeszcze nie było w danym roku, trzeba odjąć rok
						if(m < m_bd || (m <= m_bd && d < d_bd))
						{
							--age;
						}

						win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Wiek: " + std::to_string(age));
					}
				}
			}

			if(it->second.city.size() > 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Miasto: " + it->second.city);
			}

			if(it->second.country.size() > 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Kraj: " + it->second.country);
			}

			if(it->second.short_desc.size() > 0)
			{
				std::string short_desc = it->second.short_desc;

				// jeśli w opisie były tylko ciągi formatujące, nie pokazuj tekstu
				if(buf_chars(short_desc) > 0)
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Krótki opis: " + short_desc);
				}
			}

			if(it->second.long_desc.size() > 0)
			{
				std::string long_desc = it->second.long_desc;

				// jeśli w opisie były tylko ciągi formatujące, nie pokazuj tekstu
				if(buf_chars(long_desc) > 0)
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Długi opis: " + long_desc);
				}
			}

			if(it->second.email.size() > 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Adres email: " + it->second.email);

/*
				if(it->second.v_email.size() > 0)
				{
					win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  vEmail: " + it->second.v_email);
				}
*/
			}

			if(it->second.www.size() > 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Strona internetowa: " + it->second.www);
			}

			if(it->second.offmsg.size() > 0)
			{
				if(it->second.offmsg == "all")
				{
					it->second.offmsg = "przyjmuje od wszystkich";
				}

				else if(it->second.offmsg == "friend")
				{
					it->second.offmsg = "przyjmuje od przyjaciół";
				}

				else if(it->second.offmsg == "none")
				{
					it->second.offmsg = "nie przyjmuje";
				}

				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Wiadomości offline: " + it->second.offmsg);
			}

			if(it->second.type.size() > 0)
			{
				if(it->second.type == "0")
				{
					it->second.type = "nowicjusz";
				}

				else if(it->second.type == "1")
				{
					it->second.type = "bywalec";
				}

				else if(it->second.type == "2")
				{
					it->second.type = "wyjadacz";
				}

				else if(it->second.type == "3")
				{
					it->second.type = "guru";
				}

				// dla mojego nicka zwracana jest dokładniejsza informacja o randze, dodaj ją w nawiasie
				if(it->second.rank.size() > 0)
				{
					it->second.type += " (" + it->second.rank + ")";
				}

				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Ranga: " + it->second.type);
			}

/*
			if(it->second.prefs.size() > 0)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Preferencje: " + it->second.prefs);
			}
*/

			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "Koniec informacji o użytkowniku " xBOLD_ON + it->first);
		}

		else
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					uINFOn xRED "Wystąpił błąd podczas przetwarzania wizytówki użytkownika " + raw_parm4);
		}
	}

	// po przetworzeniu wizytówki wyczyść informacje o użytkowniku
	ga.co.erase(raw_parm4);
}


/*
	NOTICE 121 (ulubione nicki - lista)
	:NickServ!service@service.onet NOTICE ucieszony86 :121 :nick1 nick2 nick3
*/
void raw_notice_121(struct global_args &ga, std::string &raw_buf)
{
	if(ga.cf.friends_empty)
	{
		if(ga.my_friends.size() > 0)
		{
			ga.my_friends += " ";
		}

		ga.my_friends += get_rest_from_buf(raw_buf, ":121 :");
	}
}


/*
	NOTICE 122 (ulubione nicki - koniec listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :122 :end of friend list
*/
void raw_notice_122(struct global_args &ga, struct channel_irc *ci[])
{
	if(ga.cf.friends_empty)
	{
		if(ga.my_friends.size() > 0)
		{
			std::map<std::string, std::string> nicklist;

			std::stringstream my_friends_stream(ga.my_friends);
			std::string nick, nick_key, nicklist_show;

			while(std::getline(my_friends_stream, nick, ' '))
			{
				if(nick.size() > 0)
				{
					nick_key = buf_lower2upper(nick);

					// jeśli osoba z listy przyjaciół jest online, jej kolor będzie inny, niż gdy jest offline,
					// nick ten będzie też przed osobami offline na liście
					if(std::find(ga.my_friends_online.begin(), ga.my_friends_online.end(), nick) != ga.my_friends_online.end())
					{
						// przy braku obsługi kolorów podkreśl nick dla odróżnienia, że jest online
						nicklist["1" + nick_key] = (ga.use_colors ? xBOLD_ON xGREEN : xBOLD_ON xUNDERLINE_ON) + nick;
					}

					else
					{
						nicklist["2" + nick_key] = xBOLD_ON xDARK + nick;
					}
				}
			}

			for(auto it = nicklist.begin(); it != nicklist.end(); ++it)
			{
				nicklist_show += (nicklist_show.size() == 0 ? "" : xNORMAL ", ") + it->second;
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn "Osoby dodane do listy przyjaciół (" + std::to_string(nicklist.size()) + "): " + nicklist_show);
		}

		else
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Nie posiadasz osób dodanych do listy przyjaciół.");
		}
	}

	ga.my_friends.clear();
}


/*
	NOTICE 131 (ignorowane nicki - lista)
	:NickServ!service@service.onet NOTICE ucieszony86 :131 :Bot
*/
void raw_notice_131(struct global_args &ga, std::string &raw_buf)
{
	if(ga.cf.ignore_empty)
	{
		if(ga.my_ignore.size() > 0)
		{
			ga.my_ignore += " ";
		}

		ga.my_ignore += get_rest_from_buf(raw_buf, ":131 :");
	}
}


/*
	NOTICE 132 (ignorowane nicki - koniec listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :132 :end of ignore list
*/
void raw_notice_132(struct global_args &ga, struct channel_irc *ci[])
{
	if(ga.cf.ignore_empty)
	{
		if(ga.my_ignore.size() > 0)
		{
			std::map<std::string, std::string> nicklist;

			std::stringstream my_friends_stream(ga.my_ignore);
			std::string nick, nick_key, nicklist_show;

			while(std::getline(my_friends_stream, nick, ' '))
			{
				if(nick.size() > 0)
				{
					nick_key = buf_lower2upper(nick);
					nicklist[nick_key] = nick;
				}
			}

			for(auto it = nicklist.begin(); it != nicklist.end(); ++it)
			{
				nicklist_show += (nicklist_show.size() == 0 ? xYELLOW "" : xTERMC ", " xYELLOW) + it->second;
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn "Osoby dodane do listy ignorowanych (" + std::to_string(nicklist.size()) + "): " + nicklist_show);
		}

		else
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Nie posiadasz osób dodanych do listy ignorowanych.");
		}
	}

	ga.my_ignore.clear();
}


/*
	NOTICE 141 (ulubione pokoje - lista)
	:NickServ!service@service.onet NOTICE ucieszony86 :141 :#pokój1 #pokój2 #pokój3
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
void raw_notice_142(struct global_args &ga, struct channel_irc *ci[])
{
	// pokaż listę ulubionych pokoi lub wejdź do ulubionych pokoi w kolejności alfabetycznej (później dodać opcję wyboru)

	std::map<std::string, std::string> chanlist;

	std::stringstream my_favourites_stream(ga.my_favourites);
	std::string chan, chan_key;

	// pobierz pokoje z bufora i wpisz do std::map
	while(std::getline(my_favourites_stream, chan, ' '))
	{
		// nie reaguj na nadmiarowe spacje, wtedy chan będzie pusty (mało prawdopodobne, ale trzeba się przygotować na wszystko)
		if(chan.size() > 0)
		{
			// w kluczu trzymaj pokój zapisany wielkimi literami (w celu poprawienia sortowania zapewnianego przez std::map)
			chan_key = buf_lower2upper(chan);

			// dodaj pokój do listy w std::map
			chanlist[chan_key] = chan;
		}
	}

	if(ga.cf.favourites_empty)
	{
		std::string chanlist_show;

		for(auto it = chanlist.begin(); it != chanlist.end(); ++it)
		{
			chanlist_show += (chanlist_show.size() == 0 ? xGREEN "" : xTERMC ", " xGREEN) + it->second;
		}

		win_buf_add_str(ga, ci, ci[ga.current]->channel, (chanlist_show.size() > 0
				? oINFOn "Pokoje dodane do listy ulubionych (" + std::to_string(chanlist.size()) + "): " + chanlist_show
				: oINFOn xWHITE "Nie posiadasz pokoi dodanych do listy ulubionych."));
	}

	else
	{
		std::string chanlist_join;
		bool was_chan;

		// pomiń te pokoje, które już były (po wylogowaniu i ponownym zalogowaniu się, nie dotyczy pierwszego logowania po uruchomieniu programu)
		for(auto it = chanlist.begin(); it != chanlist.end(); ++it)
		{
			was_chan = false;

			for(int i = 0; i < CHAN_CHAT; ++i)	// szukaj jedynie pokoi czata, bez "Status", "DebugIRC" i "RawUnknown"
			{
				if(ci[i] && ci[i]->channel == it->second)
				{
					was_chan = true;
					break;
				}
			}

			if(! was_chan)
			{
				// pierwszy pokój przepisz bez zmian, kolejne pokoje muszą być rozdzielone przecinkiem
				chanlist_join += (chanlist_join.size() == 0 ? "" : ",") + it->second;
			}
		}

		// wejdź do ulubionych po pominięciu ewentualnych pokoi, w których program już był (jeśli jakieś zostały do wejścia)
		if(chanlist_join.size() > 0)
		{
			irc_send(ga, ci, "JOIN " + chanlist_join);
		}
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
void raw_notice_152(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string srv_msg = get_rest_from_buf(raw_buf, ":152 :");

	if(srv_msg == "end of homes list")
	{
		if(ga.cs_homes.size() > 0)
		{
			std::map<std::string, std::string> chanlist;

			std::stringstream chanlist_stream(ga.cs_homes);
			std::string chan, chan_key, chanlist_show;

			while(std::getline(chanlist_stream, chan, ' '))
			{
				if(chan.size() > 0)
				{
					chan_key = buf_lower2upper(chan);

					if(chan[0] == 'q')
					{
						if(chan.size() > 1)
						{
							chan.insert(1, xGREEN);
						}

						chanlist["1" + chan_key] = chan;
					}

					else if(chan[0] == 'o')
					{
						if(chan.size() > 1)
						{
							chan.insert(1, xGREEN);
						}

						chanlist["2" + chan_key] = chan;
					}

					else if(chan[0] == 'h')
					{
						if(chan.size() > 1)
						{
							chan.insert(1, xGREEN);
						}

						chanlist["3" + chan_key] = chan;
					}

					else
					{
						// jako wyjątek wrzuć na początek listy
						chanlist["0" + chan_key] = xGREEN + chan;
					}
				}
			}

			for(auto it = chanlist.begin(); it != chanlist.end(); ++it)
			{
				chanlist_show += (chanlist_show.size() == 0 ? "" : xTERMC ", ") + it->second;
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn "Pokoje, w których posiadasz uprawnienia (" + std::to_string(chanlist.size()) + "): " + chanlist_show);
		}

		else
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Nie posiadasz uprawnień w żadnym pokoju.");
		}

		// po użyciu wyczyść bufor, aby kolejne użycie CS HOMES wpisało wartość od nowa, a nie nadpisało
		ga.cs_homes.clear();
	}

	else if(srv_msg == "end of offline senders list")
	{
		// feature
	}

	// gdyby pojawiła się nieoczekiwana wartość, wyświetl ją bez zmian
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOn xWHITE + get_value_from_buf(raw_buf, ":", "!") + ": " + srv_msg);
	}
}


/*
	NOTICE 160 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :160 #ucc :Jakiś temat pokoju
*/
void raw_notice_160(struct global_args &ga, std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	ga.cs_i[raw_parm4].topic = form_from_chat(get_rest_from_buf(raw_buf, raw_parm4 + " :"));
}


/*
	NOTICE 161 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :161 #ucc :topicAuthor=ucieszony86 rank=0.9100 topicDate=1424534155 private=0 type=0 createdDate=1381197987 password= limit=0 vEmail=0 www= catMajor=4 moderated=0 avatar= guardian=0 kickRejoin=120 email= auditorium=0
*/
void raw_notice_161(struct global_args &ga, std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	// dodaj spację na końcu bufora, aby parser wyciągający dane działał prawidłowo dla ostatniej wartości w buforze
	raw_buf += " ";

	// pobierz informacje o pokoju
	ga.cs_i[raw_parm4].topic_author = get_value_from_buf(raw_buf, "topicAuthor=", " "); //

	ga.cs_i[raw_parm4].rank = get_value_from_buf(raw_buf, "rank=", " "); //

	ga.cs_i[raw_parm4].topic_date = get_value_from_buf(raw_buf, "topicDate=", " "); //

	ga.cs_i[raw_parm4].priv = get_value_from_buf(raw_buf, "private=", " "); //

	ga.cs_i[raw_parm4].type = get_value_from_buf(raw_buf, "type=", " "); //

	ga.cs_i[raw_parm4].created_date = get_value_from_buf(raw_buf, "createdDate=", " "); //

	ga.cs_i[raw_parm4].password = get_value_from_buf(raw_buf, "password=", " ");

	ga.cs_i[raw_parm4].limit = get_value_from_buf(raw_buf, "limit=", " ");

	ga.cs_i[raw_parm4].v_email = get_value_from_buf(raw_buf, "vEmail=", " ");

	ga.cs_i[raw_parm4].www = get_value_from_buf(raw_buf, "www=", " "); //

	ga.cs_i[raw_parm4].cat_major = get_value_from_buf(raw_buf, "catMajor=", " ");

	ga.cs_i[raw_parm4].moderated = get_value_from_buf(raw_buf, "moderated=", " ");

	ga.cs_i[raw_parm4].avatar = get_value_from_buf(raw_buf, "avatar=", " "); //

	ga.cs_i[raw_parm4].guardian = get_value_from_buf(raw_buf, "guardian=", " ");

	ga.cs_i[raw_parm4].kick_rejoin = get_value_from_buf(raw_buf, "kickRejoin=", " ");

	ga.cs_i[raw_parm4].email = get_value_from_buf(raw_buf, "email=", " "); //

	ga.cs_i[raw_parm4].auditorium = get_value_from_buf(raw_buf, "auditorium=", " ");
}


/*
	NOTICE 162 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :162 #ucc :q,ucieszony86 (i inne opy/sopy)
*/
void raw_notice_162(struct global_args &ga, std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	ga.cs_i[raw_parm4].stats = get_rest_from_buf(raw_buf, raw_parm4 + " :");
}


/*
	NOTICE 163 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :163 #ucc b nick ucieszony86 1402408270 :
	:ChanServ!service@service.onet NOTICE ucieszony86 :163 #ucc b *!*@host ucieszony86 1423269397 :nick
	:ChanServ!service@service.onet NOTICE ucieszony86 :163 #ucc I test!*@* ucieszony86 1424732412 :
*/
void raw_notice_163(struct global_args &ga, std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);
	std::string raw_parm7 = get_raw_parm(raw_buf, 7);
	std::string raw_parm8 = get_raw_parm(raw_buf, 8);
	std::string raw_parm_rest = get_rest_from_buf(raw_buf, raw_parm8 + " :");

/*
	Dodawanie użytkowników po kluczu z daty dodania posortuje ich wg daty dodania.
*/

	if(raw_parm5 == "b")
	{
		if(raw_parm_rest.size() == 0)
		{
			// zbanowany nick, który istnieje
			if(raw_parm6.find("!") == std::string::npos)
			{
				ga.cs_i[raw_parm4].banned[raw_parm8] = xRED + raw_parm6 + xTERMC " " xWHITE "przez" xTERMC " " + raw_parm7
						+ " " xWHITE "(" + unixtimestamp2local_full(raw_parm8) + ")";
			}

			// zbanowanie po masce
			else
			{
				ga.cs_i[raw_parm4].banned[raw_parm8] = raw_parm6 + " " xWHITE "przez" xTERMC " " + raw_parm7
						+ " " xWHITE "(" + unixtimestamp2local_full(raw_parm8) + ")";
			}
		}

		else
		{
			// zbanowany nick po IP, który istnieje
			ga.cs_i[raw_parm4].banned[raw_parm8] = raw_parm6 + " (" xRED + raw_parm_rest + xTERMC ") " xWHITE "przez" xTERMC " " + raw_parm7
					+ " " xWHITE "(" + unixtimestamp2local_full(raw_parm8) + ")";
		}
	}

	else
	{
		if(raw_parm6.find("!") == std::string::npos)
		{
			// zaproszony nick, który istnieje
			ga.cs_i[raw_parm4].invited[raw_parm8] = xGREEN + raw_parm6 + xTERMC " " xWHITE "przez" xTERMC " " + raw_parm7
					+ " " xWHITE "(" + unixtimestamp2local_full(raw_parm8) + ")";
		}

		else
		{
			// zaproszenie po masce
			ga.cs_i[raw_parm4].invited[raw_parm8] = raw_parm6 + " " xWHITE "przez" xTERMC " " + raw_parm7
					+ " " xWHITE "(" + unixtimestamp2local_full(raw_parm8) + ")";
		}
	}
}


/*
	NOTICE 164 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :164 #ucc :end of channel info
*/
void raw_notice_164(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	auto it = ga.cs_i.find(raw_parm4);

	if(it != ga.cs_i.end())
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb xYELLOW_BLACK + raw_parm4 + xTERMC + " [Informacje]");

		if(it->second.created_date.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn "  Data utworzenia pokoju: " + unixtimestamp2local_full(it->second.created_date));
		}

		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				(it->second.topic.size() > 0
				? oINFOn "  Temat: " + it->second.topic
				: oINFOn "  Temat pokoju nie został ustawiony (jest pusty)."));

		if(it->second.topic_author.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Autor tematu: " + it->second.topic_author);
		}

		if(it->second.topic_date.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel,
					oINFOn "  Data ustawienia tematu: " + unixtimestamp2local_full(it->second.topic_date));
		}

		if(it->second.avatar.size() > 0)
		{
			size_t avatar_full = it->second.avatar.find(",1");

			if(avatar_full != std::string::npos)
			{
				it->second.avatar.replace(avatar_full + 1, 1, "0");
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Awatar: " + it->second.avatar);
		}

		if(it->second.desc.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Opis: " + it->second.desc);
		}

		if(it->second.email.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Adres email: " + it->second.email);
		}

		if(it->second.www.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Strona internetowa: " + it->second.www);
		}

		if(it->second.type.size() > 0)
		{
			if(it->second.type == "0")
			{
				it->second.type = "Dziki";
			}

			else if(it->second.type == "1")
			{
				it->second.type = "Oswojony";
			}

			else if(it->second.type == "2")
			{
				it->second.type = "Z klasą";
			}

			else if(it->second.type == "3")
			{
				it->second.type = "Kultowy";
			}

			if(it->second.rank.size() > 0)
			{
				it->second.type += " (" + it->second.rank + ")";
			}

			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  Ranga: " + it->second.type);
		}

		if(it->second.priv == "1")
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "  Pokój jest prywatny.");
		}

		if(it->second.stats.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb "  Uprawnienia:");

			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  " + it->second.stats);
		}

		if(it->second.banned.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb "  Zbanowani:");

			for(auto it2 = it->second.banned.begin(); it2 != it->second.banned.end(); ++it2)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  " + it2->second);
			}
		}

		if(it->second.invited.size() > 0)
		{
			win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb "  Zaproszeni:");

			for(auto it2 = it->second.invited.begin(); it2 != it->second.invited.end(); ++it2)
			{
				win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "  " + it2->second);
			}
		}

		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn "Koniec informacji o pokoju " xBOLD_ON + raw_parm4);
	}

	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, uINFOn xRED "Wystąpił błąd podczas przetwarzania informacji o pokoju " + raw_parm4);
	}

	// po przetworzeniu informacji o pokoju wyczyść jego dane
	ga.cs_i.erase(raw_parm4);
}


/*
	NOTICE 165 (CS INFO #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :165 #ucc :Opis pokoju
*/
void raw_notice_165(struct global_args &ga, std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	ga.cs_i[raw_parm4].desc = form_from_chat(get_rest_from_buf(raw_buf, raw_parm4 + " :"));
}


/*
	NOTICE 210 (NS SET LONGDESC - gdy nie podano opcji do zmiany)
	:NickServ!service@service.onet NOTICE ucieszony86 :210 :nothing changed
*/
void raw_notice_210(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + nick_who + ": niczego nie zmieniono.");
}


/*
	NOTICE 211 (NS SET SHORTDESC - gdy nie podano opcji do zmiany, a wcześniej była ustawiona jakaś wartość)
	:NickServ!service@service.onet NOTICE ucieszony86 :211 shortDesc :value unset
*/
void raw_notice_211(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + nick_who + ": " + raw_parm4 + " - wartość wyłączona.");
}


/*
	NOTICE 220 (NS FRIENDS ADD nick)
	:NickServ!service@service.onet NOTICE ucc_test :220 ucieszony86 :friend added to list
*/
void raw_notice_220(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm4 + " został(a) dodany(-na) do listy przyjaciół.");
}


/*
	NOTICE 221 (NS FRIENDS DEL nick)
	:NickServ!service@service.onet NOTICE ucc_test :221 ucieszony86 :friend removed from list
*/
void raw_notice_221(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm4 + " został(a) usunięty(-ta) z listy przyjaciół.");
}


/*
	NOTICE 230 (NS IGNORE ADD nick)
	:NickServ!service@service.onet NOTICE ucc :230 ucc_test :ignore added to list
*/
void raw_notice_230(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm4 + " został(a) dodany(-na) do listy ignorowanych.");
}


/*
	NOTICE 231 (NS IGNORE DEL nick)
	:NickServ!service@service.onet NOTICE ucc :231 ucc_test :ignore removed from list
*/
void raw_notice_231(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm4 + " został(a) usunięty(-ta) z listy ignorowanych.");
}



/*
	NOTICE 240 (NS FAVOURITES ADD #pokój)
	:NickServ!service@service.onet NOTICE ucc_test :240 #Linux :favourite added to list
*/
void raw_notice_240(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Pokój " + raw_parm4 + " został dodany do listy ulubionych.");
}


/*
	NOTICE 241 (NS FAVOURITES DEL #pokój)
	:NickServ!service@service.onet NOTICE ucc_test :241 #ucc :favourite removed from list
*/
void raw_notice_241(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Pokój " + raw_parm4 + " został usunięty z listy ulubionych.");
}


/*
	NOTICE 250 (CS REGISTER #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :250 #nowy_test :channel registered
*/
void raw_notice_250(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xGREEN "Pomyślnie utworzono nowy pokój " + raw_parm4);
}


/*
	NOTICE 251 (CS DROP #pokój)
	:ChanServ!service@service.onet NOTICE ucieszony86 :251 #nowy_test :has been dropped
*/
void raw_notice_251(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	// informacja w aktywnym pokoju (o ile to nie "Status", "DebugIRC" i "RawUnknown")
	if(ga.current < CHAN_CHAT)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Pokój " + raw_parm4 + " został usunięty przez " + raw_parm2);
	}

	// pokaż również w "Status"
	win_buf_add_str(ga, ci, "Status", oINFOn xRED "Pokój " + raw_parm4 + " został usunięty przez " + raw_parm2);
}


/*
	NOTICE 252 (CS DROP #pokój - pojawia się, gdy to ja usuwam pokój, ale podobna informacja pojawia się w RAW NOTICE 252 i 261, więc tę ukryj)
	:ChanServ!service@service.onet NOTICE #nowy_test :252 ucieszony86 :has dropped this channel
*/
void raw_notice_252()
{
}


/*
	NOTICE 253 (CS TRANSFER #pokój nick - pojawia się, gdy to ja przekazuję komuś pokój, ale podobna informacja jest w RAW NOTICE 254, więc tę ukryj)
	:ChanServ!service@service.onet NOTICE ucieszony86 :253 #ucc ucc :channel owner changed
*/
void raw_notice_253()
{
}


/*
	NOTICE 254 (CS TRANSFER #pokój nick)
	:ChanServ!service@service.onet NOTICE #ucc :254 ucieszony86 ucc :changed channel owner
*/
void raw_notice_254(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// informacja w pokoju
	win_buf_add_str(ga, ci, raw_parm2, oINFOn xMAGENTA + raw_parm4 + " przekazał(a) pokój " + raw_parm2 + " dla " + raw_parm5);

	// pokaż również w "Status"
	win_buf_add_str(ga, ci, "Status", oINFOn xMAGENTA + raw_parm4 + " przekazał(a) pokój " + raw_parm2 + " dla " + raw_parm5);
}


/*
	NOTICE 255 (pojawia się, gdy to ja zmieniam ustawienia, ale podobna informacja pojawia się w RAW NOTICE 256, więc tę ukryj)
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
void raw_notice_256(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	// przebuduj parametry tak, aby pasowały do funkcji raw_mode(), aby nie dublować tego samego; poniżej przykładowy RAW z MODE:
	// :ChanServ!service@service.onet MODE #ucc +h ucc_test

	// w ten sposób należy przebudować parametry, aby pasowały do raw_mode()
	// 0 - tu wrzucić 4
	// 1 - bez znaczenia
	// 2 - pozostaje bez zmian
	// 3 - tu wrzucić 5
	// 4 - tu wrzucić wszystko od 6 do końca bufora (czyli za 5)

	std::string raw_parm0 = get_raw_parm(raw_buf, 4);
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm3 = get_raw_parm(raw_buf, 5);
	std::string raw_rest = get_rest_from_buf(raw_buf, raw_parm3 + " ");

	// w miejscu raw_parm0 i raw_parm1 trzeba coś wpisać (cokolwiek), dlatego wpisano kropkę (należy pamiętać, że raw_parm0 jest wysyłany jako parametr)
	std::string raw_buf_new = ". . " + raw_parm2 + " " + raw_parm3 + " " + raw_rest;

	// przykładowy RAW z NOTICE 256:
	// :ChanServ!service@service.onet NOTICE #ucc :256 ucieszony86 -h ucc_test :channel privilege changed
	// i po przebudowaniu (należy pamiętać, że raw_parm0 jest wysyłany jako parametr w raw_mode() ):
	// . . #ucc -h ucc_test :channel privilege changed
	raw_mode(ga, ci, raw_buf_new, raw_parm0);
}


/*
	NOTICE 257 (pojawia się, gdy to ja zmieniam ustawienia, ale podobna informacja pojawia się w RAW NOTICE 258, więc tę ukryj)
	:ChanServ!service@service.onet NOTICE ucieszony86 :257 #ucc * :settings changed
*/
void raw_notice_257()
{
}


/*
	NOTICE 258 (np. CS SET #ucc GUARDIAN 0 - zwraca 'i')
	:ChanServ!service@service.onet NOTICE #ucc :258 ucieszony86 * :channel settings changed
	:ChanServ!service@service.onet NOTICE #ucc :258 ucieszony86 i :channel settings changed
*/
void raw_notice_258(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm2 = get_raw_parm(raw_buf, 2);
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, ci, raw_parm2, oINFOn xMAGENTA + raw_parm4 + " zmienił(a) ustawienia pokoju " + raw_parm2 + " [" + raw_parm5 + "]");
}


/*
	NOTICE 259 (np. wpisanie 2x tego samego tematu: CS SET #ucc TOPIC abc)
	:ChanServ!service@service.onet NOTICE ucieszony86 :259 #ucc :nothing changed
*/
void raw_notice_259(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, raw_parm4, oINFOn xWHITE "Niczego nie zmieniono w pokoju " + raw_parm4);
}


/*
	NOTICE 260 (pokazywać tylko wtedy, gdy nie przebywamy w pokoju, którego zmiana dotyczy, bo w przeciwnym razie pokaże się dwa razy)
	:ChanServ!service@service.onet NOTICE ucc_test :260 ucc_test #ucc -h :channel privilege changed
	:ChanServ!service@service.onet NOTICE ucc_test :260 AT89S8253 #ucc -h :channel privilege changed
	:ChanServ!service@service.onet NOTICE ucieszony86 :260 ucieszony86 #ucc -q :channel privilege changed
*/
void raw_notice_260(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	// przebuduj parametry tak, aby pasowały do funkcji raw_mode(), aby nie dublować tego samego; poniżej przykładowy RAW z MODE:
	// :ChanServ!service@service.onet MODE #ucc +h ucc_test

	// w ten sposób należy przebudować parametry, aby pasowały do raw_mode()
	// 0 - tu wrzucić 4
	// 1 - bez znaczenia
	// 2 - tu wrzucić 5
	// 3 - tu wrzucić 6
	// 4 - tu wrzucić 2

	std::string raw_parm0 = get_raw_parm(raw_buf, 4);
	std::string raw_parm2 = get_raw_parm(raw_buf, 5);
	std::string raw_parm3 = get_raw_parm(raw_buf, 6);
	std::string raw_parm4 = get_raw_parm(raw_buf, 2);

	// w miejscu raw_parm0 i raw_parm1 trzeba coś wpisać (cokolwiek), dlatego wpisano kropkę (należy pamiętać, że raw_parm0 jest wysyłany jako parametr)
	std::string raw_buf_new = ". . " + raw_parm2 + " " + raw_parm3 + " " + raw_parm4;

	// przykładowy RAW z NOTICE 260:
	// :ChanServ!service@service.onet NOTICE Kernel_Panic :260 ucieszony86 #ucc +h :channel privilege changed
	// i po przebudowaniu (należy pamiętać, że raw_parm0 jest wysyłany jako parametr w raw_mode() ):
	// . . #ucc +h Kernel_Panic

	// nie pokazuj, jeśli pokój, którego zmiana dotyczy jest pokojem, w którym jesteśmy
	for(int i = 0; i < CHAN_CHAT; ++i)
	{
		if(ci[i] && ci[i]->channel == raw_parm2)
		{
			return;
		}
	}

	raw_mode(ga, ci, raw_buf_new, raw_parm0);
}


/*
	NOTICE 261 (CS DROP #pokój - podobną informację pokazuje RAW NOTICE 251 i RAW NOTICE 252, więc tę ukryj)
	:ChanServ!service@service.onet NOTICE ucieszony86 :261 ucieszony86 #nowy_test :has dropped this channel
*/
void raw_notice_261()
{
}


/*
	NOTICE 400 (NS FAVOURITES ADD/DEL #pokój, NS FRIENDS ADD/DEL nick - wykonywane na nicku tymczasowym)
	:NickServ!service@service.onet NOTICE ~zyxewq :400 :you are not registered
*/
void raw_notice_400(struct global_args &ga, struct channel_irc *ci[])
{
	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nie jesteś zarejestrowany(-na).");
}


/*
	NOTICE 401 (NS FRIENDS ADD ~nick - podanie nicka tymczasowego)
	:NickServ!service@service.onet NOTICE ucc :401 ~abc :no such nick

	NOTICE 401 (NS INFO nick)
	:NickServ!service@service.onet NOTICE ucc_test :401 t :no such nick
	:NickServ!service@service.onet NOTICE ucc_test :401  :no such nick
*/
void raw_notice_401(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":401 ", " :");

	if(ga.cf.friends && raw_arg.size() > 0 && raw_arg[0] == '~')
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel,
				oINFOb xMAGENTA + raw_arg + xNORMAL " - nie można dodać nicka tymczasowego do listy przyjaciół.");
	}

	else if(raw_arg.size() > 0)
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb xDARK + raw_arg + xNORMAL " - nie ma takiego użytkownika na czacie.");
	}

	// jeśli po ADD lub INFO wpisano 4 lub więcej spacji
	else
	{
		win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka.");
	}
}


/*
	NOTICE 402 (NS IGNORE ADD - 4 lub więcej spacji po ADD)
	:NickServ!service@service.onet NOTICE ucc :402  :invalid mask

	NOTICE 402 (CS BAN #pokój ADD anonymous@IP - nieprawidłowa maska)
	:ChanServ!service@service.onet NOTICE ucieszony86 :402 anonymous@IP!*@* :invalid mask
*/
void raw_notice_402(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":402 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (! ga.cf.ignore
			? oINFOn xRED + raw_arg + " - nieprawidłowa maska."
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka."));
}


/*
	NOTICE 403 (RS INFO nick, CS VOICE/MODERATOR #pokój ADD nick - gdy nie ma nicka)
	:RankServ!service@service.onet NOTICE ucc_test :403 abc :user is not on-line
	:RankServ!service@service.onet NOTICE ucc_test :403  :user is not on-line
*/
void raw_notice_403(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":403 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_arg.size() > 0
			// opcja dla pierwszego RAW powyżej
			? oINFOb xYELLOW_BLACK + raw_arg + xNORMAL " - nie ma takiego użytkownika na czacie."
			// jeśli po INFO wpisano 4 lub więcej spacji
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka."));
}


/*
	NOTICE 404 (NS INFO nick - dla nicka tymczasowego)
	:NickServ!service@service.onet NOTICE ucc_test :404 ~zyxewq :user is not registered
*/
void raw_notice_404(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOb xMAGENTA + raw_parm4 + xNORMAL " - użytkownik nie jest zarejestrowany.");
}


/*
	NOTICE 406 (NS/CS/RS/GS nieznane_polecenie)
	:NickServ!service@service.onet NOTICE ucc_test :406 ABC :unknown command
*/
void raw_notice_406(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":406 ", " :");

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + nick_who + ": " + raw_arg + " - nieznane polecenie.");
}


/*
	NOTICE 407 (NS/CS/RS SET)
	:ChanServ!service@service.onet NOTICE ucieszony86 :407 SET :not enough parameters
*/
void raw_notice_407(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + nick_who + ": " + raw_parm4 + " - brak wystarczającej liczby parametrów.");
}


/*
	NOTICE 408 (NS FAVOURITES ADD/CS DROP - 4 lub więcej spacji po ADD)
	:NickServ!service@service.onet NOTICE ucc :408 # :no such channel

	NOTICE 408 (CS INFO #pokój/CS DROP #pokój)
	:ChanServ!service@service.onet NOTICE ucc_test :408 abc :no such channel
	:ChanServ!service@service.onet NOTICE ucc_test :408  :no such channel
*/
void raw_notice_408(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":408 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_arg.size() > 0 && ! ga.cf.favourites_empty
			? oINFOn xRED + raw_arg + xNORMAL " - nie ma takiego pokoju."
			// jeśli po ADD lub INFO wpisano 4 lub więcej spacji
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano pokoju."));
}


/*
	NOTICE 409 (NS SET opcja_do_zmiany - brak argumentu)
	:NickServ!service@service.onet NOTICE ucieszony86 :409 OFFMSG :invalid argument
*/
void raw_notice_409(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + nick_who + ": nieprawidłowy argument dla " + raw_parm4);
}


/*
	NOTICE 411 (NS/CS/RS SET - 4 lub więcej spacji po SET)
	:NickServ!service@service.onet NOTICE ucc :411  :no such setting

	NOTICE 411 (NS SET ABC)
	:NickServ!service@service.onet NOTICE ucieszony86 :411 ABC :no such setting
*/
void raw_notice_411(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":411 ", " :");

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xRED + nick_who + (raw_arg.size() > 0
			? ": " + raw_arg + " - nie ma takiego ustawienia."
			: ": brak wystarczającej liczby parametrów."));
}


/*
	NOTICE 415 (RS INFO nick - gdy jest online i nie mamy dostępu do informacji)
	:RankServ!service@service.onet NOTICE ucc_test :415 ucieszony86 :permission denied
*/
void raw_notice_415(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + nick_who + ": " + raw_parm4 + " - dostęp do informacji zabroniony.");
}


/*
	NOTICE 416 (RS INFO #pokój - gdy nie mamy dostępu do informacji, musimy być w danym pokoju i być minimum opem, aby uzyskać informacje o nim)
	:RankServ!service@service.onet NOTICE ucc_test :416 #ucc :permission denied
*/
void raw_notice_416(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + nick_who + ": " + raw_parm4 + " - dostęp do informacji zabroniony.");
}


/*
	NOTICE 420 (NS FRIENDS ADD nick - gdy nick już dodano do listy)
	:NickServ!service@service.onet NOTICE ucieszony86 :420 legionella :is already on your friend list
*/
void raw_notice_420(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm4 + " jest już na liście przyjaciół.");
}


/*
	NOTICE 421 (NS FRIENDS DEL nick - gdy nicka nie było na liście)
	:NickServ!service@service.onet NOTICE ucieszony86 :421 abc :is not on your friend list
*/
void raw_notice_421(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":421 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_arg.size() > 0
			? oINFOn xWHITE + raw_arg + " nie był(a) dodany(-na) do listy przyjaciół."
			// jeśli po DEL wpisano 4 lub więcej spacji
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka."));
}


/*
	NOTICE 430 (NS IGNORE ADD nick - gdy nick już dodano do listy)
	:NickServ!service@service.onet NOTICE ucc :430 ucc_test :is already on your ignore list
*/
void raw_notice_430(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE + raw_parm4 + " jest już na liście ignorowanych.");
}


/*
	NOTICE 431 (NS IGNORE DEL nick - gdy nicka nie było na liście)
	:NickServ!service@service.onet NOTICE ucc :431 ucc_test :is not on your ignore list
*/
void raw_notice_431(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":431 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_arg.size() > 0
			? oINFOn xWHITE + raw_arg + " nie był(a) dodany(-na) do listy ignorowanych."
			// jeśli po DEL wpisano 4 lub więcej spacji
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano nicka."));
}


/*
	NOTICE 440 (NS FAVOURITES ADD #pokój - gdy pokój już jest na liście ulubionych)
	:NickServ!service@service.onet NOTICE ucieszony86 :440 #Towarzyski :is already on your favourite list
*/
void raw_notice_440(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xWHITE "Pokój " + raw_parm4 + " jest już na liście ulubionych.");
}


/*
	NOTICE 441 (NS FAVOURITES DEL #pokój - gdy pokój nie został dodany do listy ulubionych)
	:NickServ!service@service.onet NOTICE ucieszony86 :441 #Towarzyski :is not on your favourite list
*/
void raw_notice_441(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":441 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_arg.size() > 0
			? oINFOn xWHITE "Pokój " + raw_arg + " nie był dodany do listy ulubionych."
			// jeśli po DEL wpisano 4 lub więcej spacji
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano pokoju."));
}


/*
	NOTICE 452 (CS REGISTER #pokój - gdy pokój już istnieje)
	:ChanServ!service@service.onet NOTICE ucieszony86 :452 #ertt :channel name already in use
*/
void raw_notice_452(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Pokój " + raw_parm4 + " już istnieje.");
}


/*
	NOTICE 453 (CS REGISTER #nieprawidłowa_nazwa_pokoju - np. $ lub podanie przynajmniej czterech spacji)
	:ChanServ!service@service.onet NOTICE ucieszony86 :453 #$ :is not valid channel name
*/
void raw_notice_453(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":453 ", " :");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, (raw_arg.size() > 1
			? oINFOn xRED "Nazwa pokoju " + raw_arg + " jest nieprawidłowa, wybierz inną."
			// jeśli po REGISTER wpisano 4 lub więcej spacji
			: oINFOn xRED + get_value_from_buf(raw_buf, ":", "!") + ": nie podano pokoju."));
}


/*
	NOTICE 454 (CS REGISTER #xyzz - nazwa pokoju jest za mało unikalna, cokolwiek to znaczy)
	:ChanServ!service@service.onet NOTICE ucc_test :454 #xyzz :not enough unique channel name
*/
void raw_notice_454(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nazwa pokoju " + raw_parm4 + " jest za mało unikalna, wybierz inną.");
}


/*
	NOTICE 458 (CS opcja #pokój DEL nick - próba zdjęcia nienadanego statusu/uprawnienia)
	:ChanServ!service@service.onet NOTICE ucieszony86 :458 #pokój b anonymous@IP!*@* :unable to remove non-existent privilege

*/
void raw_notice_458(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	// poprawić na rozróżnianie uprawnień

	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	win_buf_add_str(ga, ci, raw_parm4, oINFOn xWHITE "Nie można usunąć nienadanego uprawnienia dla " + raw_parm6 + " (" + raw_parm5 + ").");
}


/*
	NOTICE 459 (CS opcja #pokój ADD nick - nadanie istniejącego już statusu/uprawnienia)
	:ChanServ!service@service.onet NOTICE ucieszony86 :459 #ucc q ucieszony86 :channel privilege already given
	:ChanServ!service@service.onet NOTICE ucieszony86 :459 #ucc v ucieszony86 :channel privilege already given
*/
void raw_notice_459(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	// poprawić na rozróżnianie uprawnień

	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	win_buf_add_str(ga, ci, raw_parm4, oINFOn xWHITE + raw_parm6 + " posiada już nadane uprawnienie (" + raw_parm5 + ").");
}


/*
	NOTICE 461 (CS BAN #pokój ADD nick - ban na superoperatora/operatora)
	:ChanServ!service@service.onet NOTICE ucieszony86 :461 #ucc ucc :channel operators cannot be banned
*/
void raw_notice_461(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	win_buf_add_str(ga, ci, raw_parm4,
			oINFOn xRED + raw_parm5 + " jest superoperatorem lub operatorem pokoju " + raw_parm4 + ", nie może zostać zbanowany(-na).");
}


/*
	NOTICE 463
*/
void raw_notice_463(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm5 = get_raw_parm(raw_buf, 5);

	// CS SET #pokój MODERATED ON/OFF - przy braku uprawnień
	// :ChanServ!service@service.onet NOTICE ucieszony86 :463 #Suwałki MODERATED :permission denied, insufficient privileges
	if(raw_parm5 == "MODERATED")
	{
		win_buf_add_str(ga, ci, raw_parm4, oINFOn xRED "Nie posiadasz uprawnień do zmiany stanu moderacji w pokoju " + raw_parm4);
	}

	// CS SET #pokój PASSWORD xyz - przy braku uprawnień
	// :ChanServ!service@service.onet NOTICE ucc_test :463 #ucc PASSWORD :permission denied, insufficient privileges
	else if(raw_parm5 == "PASSWORD")
	{
		win_buf_add_str(ga, ci, raw_parm4, oINFOn xRED "Nie posiadasz uprawnień do zmiany hasła w pokoju " + raw_parm4);
	}

	// CS SET #pokój PRIVATE ON/OFF - przy braku uprawnień
	// :ChanServ!service@service.onet NOTICE ucieszony86 :463 #zua_zuy_zuo PRIVATE :permission denied, insufficient privileges
	else if(raw_parm5 == "PRIVATE")
	{
		win_buf_add_str(ga, ci, raw_parm4, oINFOn xRED "Nie posiadasz uprawnień do zmiany statusu prywatności w pokoju " + raw_parm4);
	}

	// CS SET #pokój TOPIC ... - w pokoju bez uprawnień
	// :ChanServ!service@service.onet NOTICE ucc_test :463 #zua_zuy_zuo TOPIC :permission denied, insufficient privileges
	else if(raw_parm5 == "TOPIC")
	{
		win_buf_add_str(ga, ci, raw_parm4, oINFOn xRED "Nie posiadasz uprawnień do zmiany tematu w pokoju " + raw_parm4);
	}

	// nieznany lub niezaimplementowany powód zmiany ustawień
	else
	{
		std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

		win_buf_add_str(ga, ci, raw_parm4, oINFOn xRED + nick_who + ": " + raw_parm5 + " - nie posiadasz uprawnień do zmiany tego ustawienia.");
	}
}


/*
	NOTICE 464 (np. CS SET #pokój TOPIC za długi temat)
	:ChanServ!service@service.onet NOTICE ucc_test :464 TOPIC :invalid argument
	:ChanServ!service@service.onet NOTICE ucieszony86 :464 MODERATED :invalid argument
*/
void raw_notice_464(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED + nick_who + ": nieprawidłowy argument dla " + raw_parm4);
}


/*
	NOTICE 465 (CS SET #pokój nieznane_ustawienie)
	:ChanServ!service@service.onet NOTICE ucc_test :465 ABC :no such setting
*/
void raw_notice_465(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_arg = get_value_from_buf(raw_buf, ":465 ", " :");

	std::string nick_who = get_value_from_buf(raw_buf, ":", "!");

	win_buf_add_str(ga, ci, ci[ga.current]->channel,
			oINFOn xRED + nick_who + (raw_arg.size() > 0
			? ": " + raw_arg + " - nie ma takiego ustawienia."
			: ": brak wystarczającej liczby parametrów."));
}


/*
	NOTICE 467 (CS TRANSFER #pokój nick - gdy nie jestem właścicielem pokoju)
	:ChanServ!service@service.onet NOTICE ucieszony86 :467 #ucc :permission denied, you are not a channel owner
*/
void raw_notice_467(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Dostęp zabroniony, nie jesteś właścicielem pokoju " + raw_parm4);
}


/*
	NOTICE 468 (CS opcja #pokój ADD nick - przy braku uprawnień)
	:ChanServ!service@service.onet NOTICE ucieszony86 :468 #Learning_English :permission denied, insufficient privileges
*/
void raw_notice_468(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Dostęp zabroniony, nie posiadasz wystarczających uprawnień w pokoju " + raw_parm4);
}


/*
	NOTICE 470 (CS REGISTER #pokój - wulgarne słowo w nazwie pokoju)
	:ChanServ!service@service.onet NOTICE ucc_test :470 #bluzg :channel name is vulgar
*/
void raw_notice_470(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Nazwa pokoju " + raw_parm4 + " jest wulgarna, wybierz inną.");
}


/*
	NOTICE 472 (CS REGISTER #pokój - zbyt szybkie zakładanie pokoi po sobie)
	:ChanServ!service@service.onet NOTICE ucieszony86 :472 #ert :wait 60 seconds before next REGISTER
*/
void raw_notice_472(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf)
{
	std::string raw_parm4 = get_raw_parm(raw_buf, 4);
	std::string raw_parm6 = get_raw_parm(raw_buf, 6);

	win_buf_add_str(ga, ci, ci[ga.current]->channel, oINFOn xRED "Musisz odczekać " + raw_parm6 + "s przed próbą utworzenia pokoju " + raw_parm4);
}
