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


#ifndef FORM_CONV_HPP
#define FORM_CONV_HPP

bool onet_font_check(std::string &onet_font);

bool onet_color_conv(std::string &onet_color, std::string &x_color, bool was_bold, bool &was_gray);

std::string form_from_chat(std::string in_buf);

std::string form_to_chat(std::string &kbd_buf);

std::string day_en_to_pl(std::string &day_en);

std::string month_en_to_pl(std::string &month_en);

std::string remove_form(std::string &in_buf);

#endif // FORM_CONV_HPP
