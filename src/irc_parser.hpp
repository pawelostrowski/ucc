#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

#define RAW_PARM_MAX 7

std::string get_value_from_buf(std::string buffer_str, std::string expr_before, std::string expr_after);

void get_raw_parm(std::string &buffer_irc_raw, std::string *raw_parm);

void irc_parser(struct global_args &ga, struct channel_irc *chan_parm[]);

/*
	Poniżej są funkcje do obsługi RAW.
*/

void raw_error(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_ping(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_invignore(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_invite(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_invreject(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_join(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_kick(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_mode(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw, bool normal_user = false);

void raw_modermsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_part(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_privmsg(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_pong(struct global_args &ga);

void raw_quit(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_topic(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_001(struct global_args &ga);

void raw_002();

void raw_003();

void raw_004();

void raw_005();

void raw_251();

void raw_252();

void raw_253();

void raw_254();

void raw_255();

void raw_256(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_257(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_258(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_259(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_265();

void raw_266();

void raw_301(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_303(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_304(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_305(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_306(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_307(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_311(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_312(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_314(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_317(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_318();

void raw_319(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_332(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_333(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_341();

void raw_353(struct global_args &ga, std::string &buffer_irc_raw);

void raw_366(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_369();

void raw_391(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_396(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_372(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_375(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_376(struct channel_irc *chan_parm[]);

void raw_378(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_402(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_403(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_405(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_412(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_433(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_451(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_461(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_473(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_482(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_530(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_600(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_601(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_602();

void raw_604(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_605(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_801(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_807(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_808(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_809(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_811(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_812(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_815(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_816(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_817(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_951(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_100(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_109(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_111(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_112(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_121();

void raw_notice_122();

void raw_notice_131();

void raw_notice_132();

void raw_notice_141(struct global_args &ga, std::string &buffer_irc_raw);

void raw_notice_142(struct global_args &ga, struct channel_irc *chan_parm[]);

void raw_notice_151(struct global_args &ga, std::string &buffer_irc_raw);

void raw_notice_152(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_notice_210(struct global_args &ga, struct channel_irc *chan_parm[], std::string &buffer_irc_raw);

void raw_notice_211(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_220(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_221(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_240(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_241(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_255();

void raw_notice_256(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_257();

void raw_notice_258(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_260();

void raw_notice_401(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_402(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_403(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_404(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_406(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_407(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_408(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_411(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

void raw_notice_415(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_416(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_420(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_421(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm);

void raw_notice_468(struct global_args &ga, struct channel_irc *chan_parm[], std::string *raw_parm, std::string &buffer_irc_raw);

#endif		// IRC_PARSER_HPP
