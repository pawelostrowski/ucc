#ifndef CHAT_UTILS_HPP
#define CHAT_UTILS_HPP

void new_chan_status(struct global_args &ga, struct channel_irc *chan_parm[]);

void new_chan_debug_irc(struct global_args &ga, struct channel_irc *chan_parm[]);

void new_chan_raw_unknown(struct global_args &ga, struct channel_irc *chan_parm[]);

bool new_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, bool active);

void del_chan_chat(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name);

void del_all_chan(struct channel_irc *chan_parm[]);

void new_or_update_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &chan_name, std::string nick, std::string zuo);

void update_nick_flags_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string &chan_name, std::string nick, struct nick_flags flags);

void del_nick_chan(struct global_args &ga, struct channel_irc *chan_parm[], std::string chan_name, std::string nick);

void hist_erase_password(std::string &kbd_buf, std::string &hist_buf, std::string &hist_ignore);

void destroy_my_password(std::string &buf);

#endif		// CHAT_UTILS_HPP
