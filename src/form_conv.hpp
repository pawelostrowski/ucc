#ifndef FORM_CONV_HPP
#define FORM_CONV_HPP

bool onet_font_check(std::string &onet_font);

std::string onet_color_conv(std::string &onet_color);

std::string form_from_chat(std::string &irc_recv_buf);

std::string form_to_chat(std::string &kbd_buf);

std::string dayen2daypl(std::string &day_en);

std::string monthen2monthpl(std::string &month_en);

std::string remove_form(std::string &in_buf);

#endif		// FORM_CONV_HPP
