/*
	Ucieszony Chat Client
	Copyright (C) 2013, 2014 Paweł Ostrowski

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


#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

// maksymalna liczba znaków w buforze klawiatury (za duża liczba przy pełnym wykorzystaniu może powodować rozłączenie z czatem podczas wysyłania wiadomości)
#define KBD_BUF_MAX_CHARS 390

// maksymalna ilość pozycji (wpisów) w buforze historii
#define HIST_BUF_MAX_ITEMS 100

// nick na pasku dolnym, gdy nie jesteśmy zalogowani do czata
#define NICK_NOT_LOGGED "Niezalogowany"

// szerokość listy nicków
#define NICKLIST_WIDTH 36

// co ile sekund wysyłać PING do serwera
#define PING_TIME 10

// po jakim czasie braku odpowiedzi zerwać połączenie (w sekundach)
#define PING_TIMEOUT 90

// plik debugowania HTTP
#define DEBUG_HTTP_FILE	"ucc_debug_http.txt"

int main_window(bool _use_colors, bool _debug_irc);

#endif		// MAIN_WINDOW_HPP
