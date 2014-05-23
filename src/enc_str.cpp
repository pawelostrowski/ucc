#include <string>		// std::string
#include <cstring>		// memcpy()
#include <iconv.h>		// konwersja kodowania znaków


std::string buf_utf2iso(std::string &buffer_str)
{
	char c_in[buffer_str.size() + 1];	// + 1 dla NULL
	char c_out[buffer_str.size() + 1];	// + 1 dla NULL

	memcpy(c_in, buffer_str.c_str(), buffer_str.size());
	c_in[buffer_str.size()] = '\x00';

	char *c_in_ptr = c_in;
	size_t c_in_len = buffer_str.size() + 1;
	char *c_out_ptr = c_out;
	size_t c_out_len = buffer_str.size() + 1;

	iconv_t cd = iconv_open("ISO-8859-2", "UTF-8");
	iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
	*c_out_ptr = '\x00';
	iconv_close(cd);

	return std::string(c_out);
}


std::string buf_iso2utf(std::string &buffer_str)
{
	char c_in[buffer_str.size() + 1];	// + 1 dla NULL
	char c_out[buffer_str.size() * 6 + 1];	// przyjęto najgorszy możliwy przypadek, gdzie są same 6-bajtowe znaki po konwersji w UTF-8 + kod NULL

	memcpy(c_in, buffer_str.c_str(), buffer_str.size());
	c_in[buffer_str.size()] = '\x00';

	char *c_in_ptr = c_in;
	size_t c_in_len = buffer_str.size() + 1;
	char *c_out_ptr = c_out;
	size_t c_out_len = buffer_str.size() * 6 + 1;

	iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");
	iconv(cd, &c_in_ptr, &c_in_len, &c_out_ptr, &c_out_len);
	*c_out_ptr = '\x00';
	iconv_close(cd);

	return std::string(c_out);
}
