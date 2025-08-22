#include "ft_irc.hpp"

/*
list:
NOT SURE WHICH ONES ARE REQUIRED BY THE SUBJECT

CAP <subcomand> [:<capabilities>]
// AUTHENTICATE ...
PASS <password>
NICK <nickname>
USER <username> * <realname>
PING <token>
PONG [<server>] <token>
OPER <name> <password>
QUIT <reason>
ERROR <reason>
JOIN <channel>{,<channel>} [<key>{, <key>}]
PART <channel>{,<channel>} [<reason>]
TOPIC <channel> [<topic>]
LIST [<channel>{,<channel>}]
NAMES <channel>{,<channel>} [<elistcond>{,<elistcond>}]
INVITE <nickname> <channel>
KICK <channel> <user> *( "," <user> ) [<comment>]
// MOTD ...
// VERSION <target>
// ADMIN [<target>]
// LUSERS
// TIME [<server>] // no gettimeofday so not possible
// STATS ...
// HELP [<subject>]
// INFO
MODE <target> [<modestring> [<mode arguments>...]]

PRIVMSG <target>{,<target>} <text to be sent>
// NOTICE <target>{,<target>} <text to be sent>
// WHO <mask>
// WHOIS [<target>] <nick>
// WHOWAS <nick> [<count>]

KILL <nickname> <comment>
// REHASH
// RESTART
// AWAY
// USERHOST <nickname>{,<nickname>}
// WALLOPS <text>

*/

int Server::msg_cap(int argc, std::string * args, Client & sender)
{
	if (argc < 2) {
		//  ERR_NEEDMOREPARAMS (461)
		return -1;
	}
	if (args[1] == "LS") {
		write(sender.fd, "CAP * LS\r\n", 11);
		//	SEND CAP * LS because server will not support capability negociation
	}
	// else ignore CAP messages because ^
}

// AUTHENTICATE ?
//

int Server::msg_pass(int argc, std::string * args, Client & sender)
{
	if (argc < 2) {
		// ERR_NEEDMOREPARAMS (461)
		return -1;
	}
	if (args[1] != _pass) {
		//error wrong password, close connection
	}
}

int Server::msg_nick(int argc, std::string * args, Client & sender)
{
	if (argc < 2) {
		// ERR_NEEDMOREPARAMS (461)
		return -1;
	}
	// if nickname already in use
	//  error
	// if nickname not valid
	//  error
	sender.nick = args[1];
}