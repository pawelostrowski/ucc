#include <string>		// std::string

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
	Pobierz 7 pierwszych wartości między spacjami z RAW (lub mniej, jeśli tyle nie ma w buforze).
*/

	int buffer_irc_raw_len = buffer_irc_raw.size() - 1;	// bez kodu \n (musi być zawsze na końcu bufora)
	int raw_nr = 0;

	// wyczyść tablice
	for(int i = 0; i < 7; ++i)	// 7 tablic
	{
		raw_parm[i].clear();	// 'i' odwołuje się do wskaźnika tablicy
	}

	// pobierz kolejne wartości do tablic
	for(int i = 0; i < buffer_irc_raw_len && raw_nr < 7; ++i)
	{
		if(buffer_irc_raw[i] == ' ')
		{
			++raw_nr;
		}

		else
		{
			raw_parm[raw_nr] += buffer_irc_raw[i];	// tu 'raw_nr' odwołuje się do wskaźnika na tablicę std::string, nie mylić z numerem elementu
		}
	}

	// jeśli w zwróconych wartościach jest dwukropek na początku, to go usuń
	for(int i = 0; i < 7 && raw_parm[i].size() > 0; ++i)
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

	std::string raw_parm[7];
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

			win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "# Błąd w buforze IRC!");

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
		raw_numeric = std::stol("0" + raw_parm[1]);	// std::string na int ("0" - zabezpieczenie się przed próbą zabronionej konwersji znaków
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

		else if(raw_parm[1] == "INVITE")
		{
			raw_invite(ga, chan_parm, raw_parm, buffer_irc_raw);
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

		else if(raw_parm[1] == "PART")
		{
			raw_part(ga, chan_parm, raw_parm, buffer_irc_raw);
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
		else if(raw_numeric)
		{
			switch(raw_numeric)
			{
			case 001:
				raw_001();
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

			case 265:
				raw_265();
				break;

			case 266:
				raw_266();
				break;

			case 304:
				raw_304(ga, chan_parm, buffer_irc_raw);
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
				raw_372(ga, buffer_irc_raw);
				break;

			case 375:
				raw_375(ga);
				break;

			case 376:
				raw_376(ga, chan_parm);
				break;

			case 378:
				raw_378(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			case 401:
				raw_401(ga, chan_parm, raw_parm);
				break;

			case 404:
				raw_404(ga, chan_parm, raw_parm);
				break;

			case 406:
				raw_406(ga, chan_parm, raw_parm);
				break;

			case 421:
				raw_421(ga, chan_parm, raw_parm);
				break;

			case 461:
				raw_461(ga, chan_parm, raw_parm);
				break;

			case 801:
				raw_801(ga, chan_parm, raw_parm);
				break;

			case 809:
				raw_809(ga, chan_parm, raw_parm);
				break;

			case 817:
				raw_817(ga, chan_parm, raw_parm, buffer_irc_raw);
				break;

			// nieznany lub niezaimplementowany jeszcze RAW numeryczny
			default:
				raw_unknown = true;
			}

		}	// else if(raw_num)

		// NOTICE ma specjalne znaczenie, bo może również zawierać RAW numeryczny
		else if(raw_parm[1] == "NOTICE")
		{
			if(raw_parm[2] == "Auth")
			{
				raw_notice_auth(ga, chan_parm, buffer_irc_raw);
			}

			// nieznany lub niezaimplementowany jeszcze RAW NOTICE
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

		// jeśli były nieznane lub niezaimplementowane RAW (każdego typu), to je wyświetl
		if(raw_unknown)
		{
			if(raw_parm[3] == "=")
			{
				win_buf_add_str(ga, chan_parm, raw_parm[4], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}

			else if(raw_parm[2].size() > 0 && raw_parm[2][0] != '#')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[3], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}

			else
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}
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

	// wyświetl komunikat
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			xYELLOW "* " + get_value_from_buf(buffer_irc_raw, " :", "\n") + "\n" xRED "# Rozłączono.");
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
	INVITE
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc INVITE ucc_test :^cf1f1551082
	:ucieszony86!50256503@87edcc.6bc2d5.ee917f.54dae7 INVITE ucc_test :#ucc
*/
void raw_invite(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	if(raw_parm[3].size() > 0 && raw_parm[3][0] == '^')
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				xYELLOW "" xBOLD_ON "* " + get_value_from_buf(buffer_irc_raw, ":", "!") + " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
				+ "] zaprasza do rozmowy prywatnej. Aby dołączyć, wpisz /join " + raw_parm[3]);
	}

	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				xYELLOW "" xBOLD_ON "* " + get_value_from_buf(buffer_irc_raw, ":", "!") + " [" + get_value_from_buf(buffer_irc_raw, "!", " ")
				+ "] zaprasza do pokoju " + raw_parm[3] + ", aby wejść, wpisz /join " + raw_parm[3]);
	}
}


/*
	JOIN
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c JOIN #ucc :rx,0
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc JOIN ^cf1f1551083 :rx,0
*/
void raw_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli to ja wchodzę, utwórz nowy kanał
	if(get_value_from_buf(buffer_irc_raw, ":", "!") == ga.zuousername && ! new_chan_chat(ga, chan_parm, raw_parm[2]))
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "# Nie udało się utworzyć nowego kanału (brak pamięci)!");
		return;
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	else if(raw_parm[2].size() > 0 && raw_parm[2][0] == '^')
	{
		// jeśli to ja dołączam do rozmowy prywatnej, komunikat będzie inny, niż jeśli to ktoś dołącza
		if(get_value_from_buf(buffer_irc_raw, ":", "!") == ga.zuousername)
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

	// aktywność typu 1
	chan_act_add(chan_parm, raw_parm[2], 1);
}


/*
	KICK
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :Zachowuj sie!
*/
void raw_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string reason;

	// jeśli podano powód, pobierz go z bufora (w raw_parm[4] co prawda nie jest cały powód, bo tekst może być dłuższy, ale informuje, że w ogóle jest)
	if(raw_parm[4].size() > 0)
	{
		reason = " [Powód: " + get_value_from_buf(buffer_irc_raw, " :", "\n") + "]";
	}

	// jeśli to mnie wyrzucono, pokaż inny komunikat
	if(raw_parm[3] == ga.zuousername)
	{
		// usuń kanał z programu
		del_chan_chat(ga, chan_parm, raw_parm[2]);

		// powód wyrzucenia pokaż w kanale "Status"
		win_buf_add_str(ga, chan_parm, "Status", xRED "* Zostajesz wyrzucony(a) z pokoju " + raw_parm[2]
				+ " przez " + get_value_from_buf(buffer_irc_raw, ":", "!") + reason);
	}

	else
	{
		// wyświetl powód wyrzucenia w kanale, w którym wyrzucono nick
		win_buf_add_str(ga, chan_parm, raw_parm[2], xRED "* " + raw_parm[3] + " zostaje wyrzucony(a) z pokoju " + raw_parm[2]
				+ " przez " + get_value_from_buf(buffer_irc_raw, ":", "!") + reason);
	}

	// aktywność typu 1
	chan_act_add(chan_parm, raw_parm[2], 1);
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
	:NickServ!service@service.onet MODE ucc_test +r
*/
void raw_mode(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string chan_join, nick_mode;

	// jeśli to typowy nick w stylu ChanServ!service@service.onet, to pobierz część przed !
	if(raw_parm[0].find("!") != std::string::npos)
	{
		nick_mode = get_value_from_buf(buffer_irc_raw, ":", "!");
	}

	// w przeciwnym razie (np. cf1f1.onet) pobierz całą część
	else
	{
		nick_mode = raw_parm[0];
	}

	for(int i = 1; i < static_cast<int>(raw_parm[3].size()); ++i)	// od 1, bo pomijamy +/-
	{
		if(raw_parm[3][i] == 'W')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
						+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] włącza publiczną kamerkę.");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
						+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wyłącza publiczną kamerkę.");
			}
		}

		else if(raw_parm[3][i] == 'V')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
						+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] włącza prywatną kamerkę.");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xWHITE "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
						+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wyłącza prywatną kamerkę.");
			}
		}

		// wykrycie bana można rozpoznać po # w trzecim parametrze RAW
		else if(raw_parm[3][i] == 'b' && raw_parm[2][0] == '#')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xRED "* " + raw_parm[i + 3] + " otrzymuje bana w pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE "* " + raw_parm[i + 3] + " nie posiada już bana w pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		else if(raw_parm[3][i] == 'q')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " jest teraz właścicielem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " nie jest już właścicielem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		else if(raw_parm[3][i] == 'o')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " jest teraz superoperatorem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " nie jest już superoperatorem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		else if(raw_parm[3][i] == 'h')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " jest teraz operatorem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " nie jest już operatorem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		else if(raw_parm[3][i] == 'e')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xTERMC "* " + raw_parm[i + 3] + " posiada teraz wyjątek od bana w pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xTERMC "* " + raw_parm[i + 3] + " nie posiada już wyjątku od bana w pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		else if(raw_parm[3][i] == 'v')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xBLUE "* " + raw_parm[i + 3] + " jest teraz gościem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xBLUE "* " + raw_parm[i + 3] + " nie jest już gościem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		else if(raw_parm[3][i] == 'X')
		{
			if(raw_parm[3][0] == '+')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " jest teraz moderatorem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}

			else if(raw_parm[3][0] == '-')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[i + 3] + " nie jest już moderatorem pokoju "
											+ raw_parm[2] + " (ustawia " + nick_mode + ")");
			}
		}

		// niezaimplementowane RAW z MODE wyświetl bez z mian (z wyjątkiem zmian busy)
		else
		{
			if(raw_parm[3][i] != 'b' && raw_parm[2][0] == '#')
			{
				win_buf_add_str(ga, chan_parm, raw_parm[2], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}
		}

	}	// for(int i = 1; i < static_cast<int>(raw_parm[3].size()); ++i)

	// pokaż aktywność pokoju (to trzeba jeszcze dopracować)
	if(raw_parm[3][1] != 'b' && raw_parm[2][0] == '#')
	{
		// aktywność typu 1
		chan_act_add(chan_parm, raw_parm[2], 1);
	}

	// jeśli wylogowaliśmy się, ale nie zamknęliśmy programu i były otwarte jakieś pokoje, wejdź do nich ponownie po zalogowaniu,
	// w tym celu wykryj RAW:
	// :NickServ!service@service.onet MODE ucc_test +r
	for(int i = 1; i < CHAN_MAX - 1; ++i)	// od 1, bo pomijamy "Status" oraz - 1, bo pomijamy "Debug"
	{
		if(chan_parm[i] && chan_parm[i]->channel.size() > 0 && raw_parm[2] == ga.zuousername && raw_parm[3][1] == 'r')
		{
			// pierwszy kanał bez przecinka
			if(chan_join.size() == 0)
			{
				chan_join = chan_parm[i]->channel;
			}

			// do pozostałych dopisz przecinek za poprzednim
			else
			{
				chan_join += "," + chan_parm[i]->channel;
			}
		}
	}

	if(chan_join.size() > 0)
	{
		irc_send(ga, chan_parm, "JOIN " + buf_utf2iso(chan_join));
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
	std::string reason;

	// jeśli podano powód, pobierz go z bufora (w raw_parm[3] co prawda nie jest cały powód, bo tekst może być dłuższy, ale informuje, że w ogóle jest)
	if(raw_parm[3].size() > 0)
	{
		reason = " [Powód: " + get_value_from_buf(buffer_irc_raw, " :", "\n") + "]";
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	if(raw_parm[2].size() > 0 && raw_parm[2][0] == '^')
	{
		if(reason.size() == 0)
		{
			reason = ".";
		}

		//jeśli to ja opuszczam rozmowę prywatną, komunikat będzie inny, niż jeśli to ktoś opuszcza
		if(get_value_from_buf(buffer_irc_raw, ":", "!") == ga.zuousername)
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
	if(get_value_from_buf(buffer_irc_raw, ":", "!") == ga.zuousername)
	{
		del_chan_chat(ga, chan_parm, raw_parm[2]);
	}

	// aktywność typu 1
	chan_act_add(chan_parm, raw_parm[2], 1);
}


/*
	PRIVMSG
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 PRIVMSG #ucc :Hello.
*/
void raw_privmsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string form_start, form_end;
	size_t nick_call = buffer_irc_raw.find(ga.zuousername);

	// jeśli ktoś mnie woła, pogrub jego nick i wyświetl w żółtym kolorze
	if(nick_call != std::string::npos)
	{
		form_start = xYELLOW "" xBOLD_ON;
		form_end = xTERMC "" xBOLD_OFF;

		// gdy ktoś mnie woła, pokaż aktywność typu 3
		chan_act_add(chan_parm, raw_parm[2], 3);
	}

	else
	{
		// gdy ktoś pisze, ale mnie nie woła, pokaż aktywność typu 2
		chan_act_add(chan_parm, raw_parm[2], 2);
	}

	win_buf_add_str(ga, chan_parm, raw_parm[2], form_start + "<" + get_value_from_buf(buffer_irc_raw, ":", "!") + "> " + form_end
					+ get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	QUIT
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Client exited
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Quit: Do potem
*/
void raw_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xYELLOW "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
			+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] wychodzi z czata ["
			+ get_value_from_buf(buffer_irc_raw, " :", "\n") + "]");
}


/*
	TOPIC
	:ucc_test!76995189@87edcc.30c29e.b9c507.d5c6b7 TOPIC #ucc :nowy temat
*/
void raw_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, raw_parm[2], xMAGENTA "* " + get_value_from_buf(buffer_irc_raw, ":", "!")
			+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] ustawia nowy temat pokoju " + raw_parm[2]);

	// wpisz temat również do bufora tematu kanału, aby wyświetlić go na górnym pasku (reszta jest identyczna jak w obsłudze RAW 332,
	// trzeba tylko przestawić parametry w raw_parm)
	raw_parm[3] = raw_parm[2];	// przesuń nazwę pokoju, reszta parametrów w raw_parm dla raw_332() nie jest istotna
	raw_332(ga, chan_parm, raw_parm, buffer_irc_raw);
}


/*
	Poniżej obsługa RAW z liczbami, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności numerycznej).
*/

/*
	001
	:cf1f2.onet 001 ucc_test :Welcome to the OnetCzat IRC Network ucc_test!76995189@acvy210.neoplus.adsl.tpnet.pl
*/
void raw_001()
{
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
	251
	:cf1f2.onet 251 ucc_test :There are 1933 users and 5 invisible on 10 servers
*/
void raw_251()
{
}


/*
	252
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
	254
	:cf1f2.onet 254 ucc_test 2422 :channels formed
*/
void raw_254()
{
}


/*
	255
	:cf1f2.onet 255 ucc_test :I have 478 clients and 1 servers
*/
void raw_255()
{
}


/*
	265
	:cf1f2.onet 265 ucc_test :Current Local Users: 478  Max: 619
*/
void raw_265()
{
}


/*
	266
	:cf1f2.onet 266 ucc_test :Current Global Users: 1938  Max: 2487
*/
void raw_266()
{
}


/*
	304
	:cf1f3.onet 304 ucc_test :SYNTAX PRIVMSG <target>{,<target>} <message>
	:cf1f4.onet 304 ucc_test :SYNTAX NICK <newnick>
*/
void raw_304(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "* " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	307 (WHOIS)
	:cf1f1.onet 307 ucc_test ucc_test :is a registered nick
*/
void raw_307(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest zarejestrowanym użytkownikiem.");
}


/*
	311 (początek WHOIS, gdy nick jest na czacie)
	:cf1f2.onet 311 ucc_test AT89S8253 70914256 aaa2a7.a7f7a6.88308b.464974 * :tururu!
	:cf1f1.onet 311 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
*/
void raw_311(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// RAW 311 zapoczątkowuje wynik WHOIS
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xGREEN "" xBOLD_ON + raw_parm[3] + xNORMAL " to: " + raw_parm[4] + "@" + raw_parm[5] + "\n"
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
	// mało elegancki sposób na odróżnienie WHOIS od WHOWAS (we WHOWAS są spacje między datą)
	std::string whowas = get_value_from_buf(buffer_irc_raw, " :", "\n");

	size_t whois_whowas_type = whowas.find(" ");

	// jeśli WHOIS
	if(whois_whowas_type == std::string::npos)
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				"* " xGREEN + raw_parm[3] + xNORMAL " używa serwera: " + raw_parm[4] + " [" + raw_parm[5] + "]");
	}

	// jeśli WHOWAS
	else
	{
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				"* " xCYAN + raw_parm[3] + xNORMAL " używał(a) serwera: " + raw_parm[4] + "\n"
				+ "* " xCYAN + raw_parm[3] + xNORMAL " był(a) zalogowany(a): " + whowas);
	}

}


/*
	314 (początek WHOWAS, gdy nick był na czacie)
	:cf1f3.onet 314 ucc_test ucieszony86 50256503 e0c697.bbe735.1b1f7f.56f6ee * :ucieszony86
	:cf1f3.onet 314 ucc_test ucc_test 76995189 e0c697.bbe735.1b1f7f.56f6ee * :hmm test s
*/
void raw_314(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xCYAN "" xBOLD_ON + raw_parm[3] + xNORMAL " to: " + raw_parm[4] + "@" + raw_parm[5] + "\n"
			+ "* " xCYAN + raw_parm[3] + xNORMAL + " ircname: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	317 (WHOIS)
	:cf1f1.onet 317 ucc_test AT89S8253 532 1400636388 :seconds idle, signon time
*/
void raw_317(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
        win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest nieobecny(a): " + raw_parm[4] + "s\n"
			+ "* " xGREEN + raw_parm[3] + xNORMAL " jest zalogowany(a) od: " + time_unixtimestamp2local_full(raw_parm[5]));
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
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest w pokojach: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}


/*
	332 (temat pokoju)
	:cf1f3.onet 332 ucc_test #ucc :Ucieszony Chat Client
*/
void raw_332(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// najpierw wyświetl temat
	win_buf_add_str(ga, chan_parm, raw_parm[3], "* Temat pokoju " xGREEN + raw_parm[3] + xNORMAL " - " + get_value_from_buf(buffer_irc_raw, " :", "\n"));

	// teraz znajdź, do którego pokoju należy temat, wpisz go do jego bufora "topic" i wyświetl na górnym pasku
	int topic_chan = -1;

	for(int i = 1; i < CHAN_MAX; ++i)
	{
		if(chan_parm[i] && chan_parm[i]->channel == raw_parm[3])	// znajdź kanał, do którego należy temat
		{
			topic_chan = i;
		}
	}

	if(topic_chan == -1)
	{
		return;
	}

	std::string topic_tmp1 = get_value_from_buf(buffer_irc_raw, " :", "\n");
	std::string topic_tmp2;

	// usuń z tematu formatowanie fontu i kolorów (na pasku nie jest obsługiwane)
	for(int i = 0; i < static_cast<int>(topic_tmp1.size()); ++i)
	{
		if(topic_tmp1[i] == '\x03')
		{
			++i;
			continue;
		}

		else if(topic_tmp1[i] == '\x04' || topic_tmp1[i] == '\x05' || topic_tmp1[i] == '\x11' || topic_tmp1[i] == '\x12'
			|| topic_tmp1[i] == '\x13' || topic_tmp1[i] == '\x14' || topic_tmp1[i] == '\x17')
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
			"* Temat pokoju " xGREEN + raw_parm[3] + xNORMAL " ustawiony przez " + get_value_from_buf(buffer_irc_raw, raw_parm[3] + " ", "!")
			+ " [" + get_value_from_buf(buffer_irc_raw, "!", " ") + "] (" + time_unixtimestamp2local_full(raw_parm[5]) + ").");
}


/*
	353 (NAMES)
	:cf1f1.onet 353 ucc_test = #scc :%ucc_test|rx,0 AT89S8253|brx,0 %Husar|rx,1 ~Ayindida|x,0 YouTube_Dekoder|rx,0 StyX1|rx,0 %Radowsky|rx,1 fml|rx,0
*/
void raw_353(struct global_args &ga, std::string &buffer_irc_raw)
{
	// rozwiązanie tymczasowe
	ga.names += get_value_from_buf(buffer_irc_raw, " :", "\n");
}


/*
	366 (koniec NAMES)
	:cf1f4.onet 366 ucc_test #ucc :End of /NAMES list.
*/
void raw_366(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	// rozwiązanie tymczasowe
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xYELLOW "* Osoby przebywające w pokoju " + raw_parm[3]);

	std::string names_tmp = xYELLOW "[";
	int how_names = 0;

	for(int i = 0; i < static_cast<int>(ga.names.size() - 1); ++i)	// - 1, bo pomijamy ostatnią spację w ga.names
	{
		if(ga.names[i] == ' ')
		{
			++how_names;

			if(how_names == 4)
			{
				how_names = 0;
				names_tmp += "]\n" xYELLOW "[";
			}

			else
			{
				names_tmp += "] [";
			}
		}

		else
		{
			names_tmp += ga.names[i];
		}
	}

	names_tmp += "]";

	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, names_tmp);

	ga.names.clear();	// po wyświetleniu nicków wyczyść bufor, aby kolejne użycie nie wyświetliło starej zawartości
}


/*
	369 (koniec WHOWAS)
	:cf1f3.onet 369 ucc_test devi85 :End of WHOWAS
*/
void raw_369()
{
}


/*
	372 (wiadomość dnia)
	:cf1f2.onet 372 ucc_test :- Onet Czat. Inny Wymiar Czatowania. Witamy!
	:cf1f2.onet 372 ucc_test :- UWAGA - Nie daj się oszukać! Zanim wyślesz jakikolwiek SMS, zapoznaj się z filmem: http://www.youtube.com/watch?v=4skUNAyIN_c
	:cf1f2.onet 372 ucc_test :-
*/
void raw_372(struct global_args &ga, std::string &buffer_irc_raw)
{
	// dopisz wiadomość dnia, którą zwrócił serwer
	ga.message_day += "\n" xYELLOW + get_value_from_buf(buffer_irc_raw, " :", "\n");
}


/*
	375
	:cf1f2.onet 375 ucc_test :cf1f2.onet message of the day
*/
void raw_375(struct global_args &ga)
{
	// RAW 375 to zapoczątkowanie wiadomości dnia, dlatego wyczyść bufor i wpisz początek (sama wiadomość zostanie dopisana w RAW 372)
	ga.message_day = xYELLOW "* Wiadomość dnia:";
}


/*
	376
	:cf1f2.onet 376 ucc_test :End of message of the day.
*/
void raw_376(struct global_args &ga, struct channel_irc *chan_parm[])
{
	// RAW ten informuje o końcu wiadomości dnia, można ją wyświetlić
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, ga.message_day);
}


/*
	378 (WHOIS)
	:cf1f1.onet 378 ucc_test ucc_test :is connecting from 76995189@adin155.neoplus.adsl.tpnet.pl 79.184.195.155
*/
void raw_378(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xGREEN + raw_parm[3] + xNORMAL " jest połączony(a) z: " + get_value_from_buf(buffer_irc_raw, "from ", "\n"));
}


/*
	401 (początek WHOIS, gdy nick/kanał nie został znaleziony, używany również przy podaniu nieprawidłowego pokoju, np. dla TOPIC, NAMES itd.)
	:cf1f4.onet 401 ucc_test abc :No such nick/channel
	:cf1f4.onet 401 ucc_test #ucc: :No such nick/channel
*/
void raw_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			"* " xGREEN "" xBOLD_ON + raw_parm[3] + xNORMAL " - nick lub kanał nie istnieje.");
}


/*
	404 (próba wysłania wiadomości do kanału, w którym nie przebywamy)
	:cf1f1.onet 404 ucc_test #ucc :Cannot send to channel (no external messages)
*/
void raw_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
			xRED "* Nie można wysłać wiadomości do kanału " + raw_parm[3] + " (nie przebywasz w nim).");
}


/*
	406 (początek WHOWAS, gdy nie było takiego nicka)
	:cf1f3.onet 406 ucc_test abc :There was no such nickname
*/
void raw_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, "* " xCYAN "" xBOLD_ON + raw_parm[3] + xNORMAL " - nie było takiego nicka.");
}


/*
	421
	:cf1f2.onet 421 ucc_test ABC :Unknown command
*/
void raw_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "* " + raw_parm[3] + " - nieznane polecenie [IRC]");
}


/*
	461
	:cf1f3.onet 461 ucc_test PRIVMSG :Not enough parameters.
	:cf1f4.onet 461 ucc_test NICK :Not enough parameters.
*/
void raw_461(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, xRED "* Brak wystarczającej liczby parametrów dla " + raw_parm[3]);
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
		win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel,
				xRED "# authKey nie zawiera oczekiwanych 16 znaków (zmiana autoryzacji?).");
		return;
	}

	// wyślij:
	// AUTHKEY <nowy_authKey>
	irc_send(ga, chan_parm, "AUTHKEY " + authkey);
}


/*
	809 (WHOIS)
	:cf1f2.onet 809 ucc_test AT89S8253 :is busy
*/
void raw_809(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, "* " xGREEN + raw_parm[3] + xNORMAL " jest oznaczony(a) jako zajęty(a).");
}


/*
	817 (historia)
	:cf1f4.onet 817 ucc_test #scc 1401032138 Damian - :bardzo korzystne oferty maja i w sumie dosc rozwiniety jak na Polskie standardy panel chmury
*/
void raw_817(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, raw_parm[3], time_unixtimestamp2local(raw_parm[4]) + xWHITE "<" + raw_parm[5] + "> " xTERMC
			+ get_value_from_buf(buffer_irc_raw, " :", "\n"), false);
}


/*
	Poniżej obsługa RAW NOTICE bez liczb.
*/

/*
	NOTICE Auth
	:cf1f4.onet NOTICE Auth :*** Looking up your hostname...
*/
void raw_notice_auth(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	win_buf_add_str(ga, chan_parm, chan_parm[ga.current_chan]->channel, "* Server: " + get_value_from_buf(buffer_irc_raw, " :", "\n"));
}
