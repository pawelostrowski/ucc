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


#ifndef IRC_PARSER_HPP
#define IRC_PARSER_HPP

std::string get_value_from_buf(std::string &buf_str, std::string expr_before, std::string expr_after);

std::string get_rest_from_buf(std::string &in_buf, std::string expr_before);

std::string get_raw_parm(std::string &raw_buf, int raw_parm_number);

void irc_parser(struct global_args &ga, struct channel_irc *ci[], std::string dbg_irc_msg = "");

/*
	Poniżej są funkcje do obsługi RAW.
*/

void raw_error(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_ping(struct global_args &ga, struct channel_irc *ci[], std::string &raw_parm1);

void raw_invignore(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_invite(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_invreject(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_join(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_kick(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_mode(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_modermsg(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_part(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_privmsg(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_pong(struct global_args &ga, std::string &raw_buf);

void raw_quit(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_topic(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_001(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_002();

void raw_003();

void raw_004();

void raw_005();

void raw_251(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_252(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_253(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_254(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_255(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_256(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_257(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_258(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_259(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_265(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_266(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_301(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_303(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_304(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_305(struct global_args &ga, struct channel_irc *ci[]);

void raw_306(struct global_args &ga, struct channel_irc *ci[]);

void raw_307(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_311(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_312(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_313(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_314(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_317(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_318(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_319(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_332(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string raw_parm3);

void raw_333(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_335(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_341();

void raw_353(struct global_args &ga, std::string &raw_buf);

void raw_366(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_369(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_371(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_372(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_374();

void raw_375(struct global_args &ga, struct channel_irc *ci[]);

void raw_376();

void raw_378(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_391(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_396(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_401(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_402(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_403(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_404(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_405(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_406(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_412(struct global_args &ga, struct channel_irc *ci[]);

void raw_421(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_433(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_441(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_442(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_443(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_445(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_446(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_451(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_461(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_462(struct global_args &ga, struct channel_irc *ci[]);

void raw_473(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_474(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_475(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_480(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_481(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_482(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_484(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_492(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_495(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_530(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_531(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_600(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_601(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_602(struct global_args &ga, std::string &raw_buf);

void raw_604(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_605(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_607();

void raw_666(struct global_args &ga, struct channel_irc *ci[]);

void raw_801(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_807(struct global_args &ga, struct channel_irc *ci[]);

void raw_808(struct global_args &ga, struct channel_irc *ci[]);

void raw_809(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_811(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_812(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_815(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_816(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_817(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_942(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_950(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_951(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf, std::string &raw_parm0);

void raw_notice_100(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_109(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_111(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_112(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_121(struct global_args &ga, std::string &raw_buf);

void raw_notice_122(struct global_args &ga, struct channel_irc *ci[]);

void raw_notice_131(struct global_args &ga, std::string &raw_buf);

void raw_notice_132(struct global_args &ga, struct channel_irc *ci[]);

void raw_notice_141(struct global_args &ga, std::string &raw_buf);

void raw_notice_142(struct global_args &ga, struct channel_irc *ci[]);

void raw_notice_151(struct global_args &ga, std::string &raw_buf);

void raw_notice_152(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_210(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_211(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_220(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_221(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_230(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_231(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_240(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_241(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_250(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_251(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_252();

void raw_notice_253();

void raw_notice_254(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_255();

void raw_notice_256(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_257();

void raw_notice_258(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_259(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_260(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_261();

void raw_notice_400(struct global_args &ga, struct channel_irc *ci[]);

void raw_notice_401(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_402(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_403(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_404(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_406(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_407(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_408(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_409(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_411(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_415(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_416(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_420(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_421(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_430(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_431(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_440(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_441(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_452(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_453(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_454(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_458(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_459(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_461(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_463(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_464(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_465(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_467(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_468(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_470(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

void raw_notice_472(struct global_args &ga, struct channel_irc *ci[], std::string &raw_buf);

#endif		// IRC_PARSER_HPP
