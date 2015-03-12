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


#include <string>		// std::string
#include <cstring>		// std::memcpy()
#include <iconv.h>		// konwersja kodowania znaków

#include "enc_str.hpp"


std::string buf_utf2iso(std::string &in_buf)
{
	size_t in_buf_len = in_buf.size();

	char in_buf_tmp[in_buf_len + 1];	// + 1 dla NULL
	char out_buf_tmp[in_buf_len * 6 + 1];	// * 6 dla bezpieczeństwa ze względu na TRANSLIT oraz + 1 dla NULL

	std::memcpy(in_buf_tmp, in_buf.c_str(), in_buf_len);
	in_buf_tmp[in_buf_len] = '\x00';

	char *in_buf_tmp_ptr = in_buf_tmp;
	size_t in_buf_tmp_len = in_buf_len + 1;
	char *out_buf_tmp_ptr = out_buf_tmp;
	size_t out_buf_tmp_len = in_buf_len * 6 + 1;

	iconv_t cd = iconv_open("ISO-8859-2//TRANSLIT", "UTF-8");
	iconv(cd, &in_buf_tmp_ptr, &in_buf_tmp_len, &out_buf_tmp_ptr, &out_buf_tmp_len);
	*out_buf_tmp_ptr = '\x00';
	iconv_close(cd);

	return std::string(out_buf_tmp);
}


std::string buf_iso2utf(std::string &in_buf)
{
	size_t in_buf_len = in_buf.size();

	char in_buf_tmp[in_buf_len + 1];	// + 1 dla NULL
	char out_buf_tmp[in_buf_len * 6 + 1];	// przyjęto najgorszy możliwy przypadek, gdzie są same 6-bajtowe znaki po konwersji na UTF-8 + NULL

	std::memcpy(in_buf_tmp, in_buf.c_str(), in_buf_len);
	in_buf_tmp[in_buf_len] = '\x00';

	char *in_buf_tmp_ptr = in_buf_tmp;
	size_t in_buf_tmp_len = in_buf_len + 1;
	char *out_buf_tmp_ptr = out_buf_tmp;
	size_t out_buf_tmp_len = in_buf_len * 6 + 1;

	iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");
	iconv(cd, &in_buf_tmp_ptr, &in_buf_tmp_len, &out_buf_tmp_ptr, &out_buf_tmp_len);
	*out_buf_tmp_ptr = '\x00';
	iconv_close(cd);

	return std::string(out_buf_tmp);
}
