#include <string>		// std::string
#include <cstring>		// memcpy()
#include <iconv.h>		// konwersja kodowania znaków


std::string buf_utf2iso(std::string &in_buf)
{
	char in_buf_tmp[in_buf.size() + 1];	// + 1 dla NULL
	char out_buf_tmp[in_buf.size() + 1];	// + 1 dla NULL

	memcpy(in_buf_tmp, in_buf.c_str(), in_buf.size());
	in_buf_tmp[in_buf.size()] = '\x00';

	char *in_buf_tmp_ptr = in_buf_tmp;
	size_t in_buf_tmp_len = in_buf.size() + 1;
	char *out_buf_tmp_ptr = out_buf_tmp;
	size_t out_buf_tmp_len = in_buf.size() + 1;

	iconv_t cd = iconv_open("ISO-8859-2", "UTF-8");
	iconv(cd, &in_buf_tmp_ptr, &in_buf_tmp_len, &out_buf_tmp_ptr, &out_buf_tmp_len);
	*out_buf_tmp_ptr = '\x00';
	iconv_close(cd);

	return std::string(out_buf_tmp);
}


std::string buf_iso2utf(std::string &in_buf)
{
	char in_buf_tmp[in_buf.size() + 1];	// + 1 dla NULL
	char out_buf_tmp[in_buf.size() * 6 + 1];	// przyjęto najgorszy możliwy przypadek, gdzie są same 6-bajtowe znaki po konwersji w UTF-8 + kod NULL

	memcpy(in_buf_tmp, in_buf.c_str(), in_buf.size());
	in_buf_tmp[in_buf.size()] = '\x00';

	char *in_buf_tmp_ptr = in_buf_tmp;
	size_t in_buf_tmp_len = in_buf.size() + 1;
	char *out_buf_tmp_ptr = out_buf_tmp;
	size_t out_buf_tmp_len = in_buf.size() * 6 + 1;

	iconv_t cd = iconv_open("UTF-8", "ISO-8859-2");
	iconv(cd, &in_buf_tmp_ptr, &in_buf_tmp_len, &out_buf_tmp_ptr, &out_buf_tmp_len);
	*out_buf_tmp_ptr = '\x00';
	iconv_close(cd);

	return std::string(out_buf_tmp);
}


std::string buf_lower2upper(std::string buf)
{
	for(unsigned int i = 0; i < buf.size(); ++i)
	{
		if(islower(buf[i]))
		{
			buf[i] = toupper(buf[i]);
		}
	}

	return buf;
}
