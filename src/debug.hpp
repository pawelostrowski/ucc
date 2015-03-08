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


#ifndef DEBUG_HPP
#define DEBUG_HPP

void http_dbg_to_file_header(struct global_args &ga, std::string dbg_header);

void http_dbg_to_file(struct global_args &ga, std::string dbg_sent, std::string dbg_recv, std::string &host, uint16_t port, std::string &stock);

#endif		// DEBUG_HPP
