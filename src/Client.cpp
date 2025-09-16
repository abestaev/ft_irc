#include "Client.hpp"
#include <cstring>
#include <unistd.h>

Client::Client(): 
	fd(-1), 
	pfdp(NULL), 
	addrlen(sizeof(addr)), 
	nick(""), 
	inbuf(""),
	is_operator(false),
	password_is_valid(false),
	nick_given(false),
	user_given(false),
	is_fully_registered(false)
{
	// Initialize sockaddr_in structure
	memset(&addr, 0, sizeof(addr));
}

Client::~Client() 
{
}

// Copy constructor
Client::Client(const Client& other)
{
	fd = other.fd;
	pfdp = other.pfdp;
	addrlen = other.addrlen;
	addr = other.addr;
	nick = other.nick;
	username = other.username;
	realname = other.realname;
	hostname = other.hostname;
	is_operator = other.is_operator;
	password_is_valid = other.password_is_valid;
	nick_given = other.nick_given;
	user_given = other.user_given;
	is_fully_registered = other.is_fully_registered;
}

// Assignment operator
Client& Client::operator=(const Client& other)
{
	if (this != &other)
	{
		fd = other.fd;
		pfdp = other.pfdp;
		addrlen = other.addrlen;
		addr = other.addr;
		nick = other.nick;
		username = other.username;
		realname = other.realname;
		hostname = other.hostname;
		is_operator = other.is_operator;
		password_is_valid = other.password_is_valid;
		nick_given = other.nick_given;
		user_given = other.user_given;
		is_fully_registered = other.is_fully_registered;
	}
	return *this;
}

// Utility methods
void Client::setNick(const std::string& new_nick)
{
	nick = new_nick;
	nick_given = true;
}

void Client::setUser(const std::string& new_username, const std::string& new_realname)
{
	username = new_username;
	realname = new_realname;
	user_given = true;
}

bool Client::isRegistered() const
{
	return password_is_valid && nick_given && user_given;
}