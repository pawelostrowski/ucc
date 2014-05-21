#include <string>		// std::string

#include "irc_parser.hpp"
#include "window_utils.hpp"
#include "network.hpp"
#include "ucc_global.hpp"


std::string get_value_raw(std::string &buffer_irc_raw, std::string expr_before, std::string expr_after)
{
	size_t pos_expr_before, pos_expr_after;		// pozycja początkowa i końcowa szukanych wyrażeń
	std::string value_raw;

	// znajdź pozycję początku szukanego wyrażenia
	pos_expr_before = buffer_irc_raw.find(expr_before);

	if(pos_expr_before != std::string::npos)
	{
		// znajdź pozycję końca szukanego wyrażenia, zaczynając od znalezionego początku + jego jego długości
		pos_expr_after = buffer_irc_raw.find(expr_after, pos_expr_before + expr_before.size());

		if(pos_expr_after != std::string::npos)
		{
			// wstaw szukaną wartość
			value_raw.clear();	// wyczyść bufor szukanej wartości
			value_raw.insert(0, buffer_irc_raw, pos_expr_before + expr_before.size(), pos_expr_after - pos_expr_before - expr_before.size());
		}
	}

	return value_raw;	// zwróć szukaną wartość między wyrażeniami lub pusty bufor, gdy nie znaleziono początku lub końca
}


void get_raw_parm(std::string &buffer_irc_raw, std::string *raw_parm)
{
/*
	Pobierz 6 pierwszych wartości między spacjami z RAW.
*/

	int buffer_irc_raw_len = buffer_irc_raw.size() - 1;	// bez kodu \n (musi być zawsze na końcu bufora)
	int raw_nr = 0;

	// wyczyść tablice
	for(int i = 0; i < 6; ++i)	// 6 tablic
	{
		raw_parm[i].clear();	// 'i' odwołuje się do wskaźnika tablicy
	}

	// pobierz kolejne wartości do tablic
	for(int i = 0; i < buffer_irc_raw_len && raw_nr < 6; ++i)
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
	for(int i = 0; i < 6 && raw_parm[i].size() > 0; ++i)
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
	std::string raw_parm[6];

	// pobierz odpowiedź z serwera
	if(! irc_recv(ga.socketfd_irc, ga.irc_ok, buffer_irc_recv, ga.msg_err))
	{
		// w przypadku błędu pokaż, co się stało
		add_show_win_buf(ga, chan_parm, ga.msg_err);
	}

	// jeśli serwer się rozłączył, zakończ, bo bufor wtedy będzie najprawdopodobniej pusty
	if(! ga.irc_ok)
	{
		return;
	}

	// konwersja formatowania fontu, kolorów i emotek
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

			add_show_win_buf(ga, chan_parm, xRED "# Błąd w buforze IRC!");

			return;		// POPRAWIĆ TO WYCHODZENIE!!!
		}

		// wstaw aktualnie obsługiwany wiersz (raw)
		buffer_irc_raw.clear();
		buffer_irc_raw.insert(0, buffer_irc_recv, pos_raw_start, pos_raw_end - pos_raw_start + 1);

		// przyjmij, że kolejny wiersz jest za kodem \n, a jeśli to koniec bufora, wykryte to będzie na końcu pętli do{} while()
		pos_raw_start = pos_raw_end + 1;

		// pobierz parametry RAW, aby wykryć dalej, jaki to rodzaj RAW
		get_raw_parm(buffer_irc_raw, raw_parm);



/*
	Zależnie od rodzaju RAWa, wywołaj odpowiednią funkcję.
*/
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

		// nieznany lub niezaimplementowany jeszcze RAW
		else
		{
			// RAW 353 zamiast pokoju ma '=' (być może inne też)
			if(raw_parm[3] == "=")
			{
				add_show_chan(ga, chan_parm, raw_parm[4], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}

			else if(raw_parm[2].size() > 0 && raw_parm[2][0] != '#')
			{
				add_show_chan(ga, chan_parm, raw_parm[3], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
			}

			else
			{
				add_show_chan(ga, chan_parm, raw_parm[2], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
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
	add_show_win_buf(ga, chan_parm, xYELLOW "* " + get_value_raw(buffer_irc_raw, "ERROR :", "\n") + "\n" xRED "# Rozłączono.");
}


/*
	PING
	PING :cf1f1.onet
*/
void raw_ping(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm)
{
	if(! irc_send(ga.socketfd_irc, ga.irc_ok, "PONG :" + raw_parm[1], ga.msg_err))
	{
		// w przypadku błędu pokaż, co się stało
		add_show_win_buf(ga, chan_parm, ga.msg_err);
	}
}


/*
	Poniżej obsługa RAW z nazwami, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności alfabetycznej).
*/


/*
	INVITE
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc INVITE ucc_test :^cf1f1551082
*/
void raw_invite(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	add_show_win_buf(ga, chan_parm, xYELLOW "" xBOLD_ON "* " + get_value_raw(buffer_irc_raw, ":", "!")
			+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] zaprasza do rozmowy prywatnej. Aby dołączyć, wpisz /join " + raw_parm[3]);
}


/*
	JOIN
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c JOIN #ucc :rx,0
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc JOIN ^cf1f1551083 :rx,0
*/
void raw_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	// jeśli to ja wchodzę, utwórz nowy kanał
	if(get_value_raw(buffer_irc_raw, ":", "!") == ga.zuousername)
	{
		new_chan(ga, chan_parm, raw_parm[2], true);
	}

	// jeśli jest ^ (rozmowa prywatna), wyświetl odpowiedni komunikat
	if(raw_parm[2].size() > 0 && raw_parm[2][0] == '^')
	{
		// jeśli to ja dołączam do rozmowy prywatnej, komunikat będzie inny, niż jeśli to ktoś dołącza
		if(get_value_raw(buffer_irc_raw, ":", "!") == ga.zuousername)
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xGREEN "* Dołączasz do rozmowy prywatnej.");
		}

		else
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xGREEN "* " + get_value_raw(buffer_irc_raw, ":", "!")
					+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] dołącza do rozmowy prywatnej.");
		}
	}

	// w przeciwnym razie wyświetl komunikat dla wejścia do pokoju
	else
	{
		// jeśli to ja wchodzę do pokoju, komunikat będzie inny, niż jeśli ktoś wchodzi
		if(get_value_raw(buffer_irc_raw, ":", "!") == ga.zuousername)
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xGREEN "* Wchodzisz do pokoju " + raw_parm[2]);
		}

		else
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xGREEN "* " + get_value_raw(buffer_irc_raw, ":", "!")
					+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] wchodzi do pokoju " + raw_parm[2]);
		}
	}
}


/*
	KICK
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 KICK #ucc ucc_test :Zachowuj sie!
*/
void raw_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string buf_tmp;

	std::string reason = get_value_raw(buffer_irc_raw, raw_parm[3] + " :", "\n");

	// jeśli to mnie wyrzucono, pokaż inny komunikat
	if(raw_parm[3] == ga.zuousername)
	{
		buf_tmp = xRED "* Zostajesz wyrzucony z pokoju " + raw_parm[2] + " przez " + get_value_raw(buffer_irc_raw, ":", "!")
				+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "]";

		// jeśli podano powód, dodaj go w nawiasie kwadratowym
		if(reason.size() > 0)
		{
			buf_tmp += " [Powód: " + reason + "]";
		}

		// usuń też kanał z programu
		del_chan_chat(ga, chan_parm);

		// powód wyrzucenia pokaż w kanale "Status"
		add_show_win_buf(ga, chan_parm, buf_tmp);
	}

	else
	{
		buf_tmp = xRED "* " + raw_parm[3] + " zostaje wyrzucony z pokoju " + raw_parm[2] + " przez " + get_value_raw(buffer_irc_raw, ":", "!")
				+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "]";

		// jeśli podano powód, dodaj go w nawiasie kwadratowym
		if(reason.size() > 0)
		{
			buf_tmp += " [Powód: " + reason + "]";
		}

		// wyświetl powód wyrzucenia w kanale, w którym wyrzucono nick
		add_show_chan(ga, chan_parm, raw_parm[2], buf_tmp);
	}
}


/*
	MODE
	:cf1f4.onet MODE ucc_test +b
	:Kernel_Panic!78259658@87edcc.6bc2d5.9f815e.0d56cc MODE Kernel_Panic :+b
	:zagubiona_miedzy_wierszami!80541395@87edcc.6f9b99.6bd006.aee4fc MODE zagubiona_miedzy_wierszami +W
	:ChanServ!service@service.onet MODE #Towarzyski +b *!12345678@*
	:ChanServ!service@service.onet MODE #ucc +h ucc_test
	:ChanServ!service@service.onet MODE #scc +qo Merovingian Merovingian
	:NickServ!service@service.onet MODE ucc_test +r
*/
// na razie bez rewelacji, pokazuj tylko zmiany kamerki oraz bany/owna/sopa/opa, potem dopisać dodawanie flag do list nicków, gdy już będą w programie
void raw_mode(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string chan_join;

	// parametr czwarty musi istnieć i mieć przynajmniej dwa znaki, aby dalsze sprawdzanie miało sens
	if(raw_parm[3].size() < 2)
	{
		return;
	}

	bool flags_tmp = false;

	if(raw_parm[3][1] == 'W')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_win_buf(ga, chan_parm, xWHITE "* " + get_value_raw(buffer_irc_raw, ":", "!")
					+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] włącza publiczną kamerkę.");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_win_buf(ga, chan_parm, xWHITE "* " + get_value_raw(buffer_irc_raw, ":", "!")
					+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] wyłącza publiczną kamerkę.");

			flags_tmp = true;
		}
	}

	if(raw_parm[3][1] == 'V')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_win_buf(ga, chan_parm, xWHITE "* " + get_value_raw(buffer_irc_raw, ":", "!")
					+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] włącza prywatną kamerkę.");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_win_buf(ga, chan_parm, xWHITE "* " + get_value_raw(buffer_irc_raw, ":", "!")
					+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] wyłącza prywatną kamerkę.");

			flags_tmp = true;
		}
	}

	// wykrycie bana można rozpoznać po # w trzecim parametrze RAW
	if(raw_parm[3][1] == 'b' && raw_parm[2][0] == '#')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xTERMC "* " + raw_parm[4] + " dostaje bana w pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xTERMC "* " + raw_parm[4] + " nie ma już bana w pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}
	}

	if(raw_parm[3][1] == 'q')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " jest teraz właścicielem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " nie jest już właścicielem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}
	}

	if(raw_parm[3].size() > 2 && raw_parm[3][2] == 'q')	// flaga 'q' może też być na kolejnej pozycji (gdy serwer zwróci 'oq', zamiast 'qo')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[5] + " jest teraz właścicielem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[5] + " nie jest już właścicielem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}
	}

	if(raw_parm[3][1] == 'o')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " jest teraz superoperatorem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " nie jest już superoperatorem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}
	}

	if(raw_parm[3].size() > 2 && raw_parm[3][2] == 'o')	// analogicznie jak z 'q'
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[5] + " jest teraz superoperatorem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[5] + " nie jest już superoperatorem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}
	}

	if(raw_parm[3][1] == 'h')
	{
		if(raw_parm[3][0] == '+')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " jest teraz operatorem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}

		else if(raw_parm[3][0] == '-')
		{
			add_show_chan(ga, chan_parm, raw_parm[2], xMAGENTA "* " + raw_parm[4] + " nie jest już operatorem pokoju " + raw_parm[2]
					+ " (ustawia " + get_value_raw(buffer_irc_raw, ":", "!") + " [" + get_value_raw(buffer_irc_raw, "!", " ") + "])");

			flags_tmp = true;
		}
	}

	// niezaimplementowane RAW z MODE wyświetl bez z mian (z wyjątkiem zmian busy)
	if(raw_parm[3][1] != 'b' && raw_parm[2][0] != '#' && ! flags_tmp)
	{
		add_show_chan(ga, chan_parm, raw_parm[2], xWHITE + buffer_irc_raw.erase(buffer_irc_raw.size() - 1, 1));	// usuń \n
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
		if(! irc_send(ga.socketfd_irc, ga.irc_ok, "JOIN " + chan_join, ga.msg_err))
		{
			// w przypadku błędu pokaż, co się stało
			add_show_win_buf(ga, chan_parm, ga.msg_err);
		}
	}
}


/*
	PART
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc
	:ucc_test!76995189@e0c697.bbe735.fea2d4.23661c PART #ucc :Bye
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f3561508
	:ucc_test!76995189@87edcc.6bc2d5.9f815e.0d56cc PART ^cf1f1552723 :Koniec rozmowy
*/
// gdy już będzie zapis logów, zmienić komunikaty, gdy opuszczam pokój lub priv, aby w logu ładnie wyglądało
void raw_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string buf_tmp = xCYAN "* " + get_value_raw(buffer_irc_raw, ":", "!")
				+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] wychodzi z pokoju " + raw_parm[2];

	std::string reason = get_value_raw(buffer_irc_raw, "PART " + raw_parm[2] + " :", "\n");		// pobierz powód wyjścia (jeśli jest)

	// jeśli podano powód, dodaj go w nawiasie kwadratowym
	if(reason.size() > 0)
	{
		buf_tmp += " [" + reason + "]";
	}

	add_show_chan(ga, chan_parm, raw_parm[2], buf_tmp);

	// jeśli to ja wychodzę, usuń kanał z programu
	if(get_value_raw(buffer_irc_raw, ":", "!") == ga.zuousername)
	{
		del_chan_chat(ga, chan_parm);
	}
}


/*
	PRIVMSG
	:AT89S8253!70914256@aaa2a7.a7f7a6.88308b.464974 PRIVMSG #ucc :Hello.
*/
void raw_privmsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw)
{
	std::string form_start, form_end;
	size_t nick_call = buffer_irc_raw.find(ga.my_nick);

	// jeśli ktoś mnie woła, pogrub jego nick i wyświetl w żółtym kolorze
	if(nick_call != std::string::npos)
	{
		form_start = xYELLOW "" xBOLD_ON;
		form_end = xTERMC "" xBOLD_OFF;
	}

	add_show_chan(ga, chan_parm, raw_parm[2], form_start + "<" + get_value_raw(buffer_irc_raw, ":", "!") + "> " + form_end
					+ get_value_raw(buffer_irc_raw, raw_parm[2] + " :", "\n"));
}


/*
	QUIT
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Client exited
	:Kernel_Panic!78259658@e0c697.bbe735.fea2d4.23661c QUIT :Quit: Do potem
*/
void raw_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw)
{
	add_show_win_buf(ga, chan_parm, xYELLOW "* " + get_value_raw(buffer_irc_raw, ":", "!")
			+ " [" + get_value_raw(buffer_irc_raw, "!", " ") + "] wychodzi z czata [" + get_value_raw(buffer_irc_raw, "QUIT :", "\n") + "]");
}


/*
	RAWy z liczbami, które występują w odpowiedzi serwera na drugiej pozycji (w kolejności numerycznej).
*/
