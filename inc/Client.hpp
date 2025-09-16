#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <poll.h>
#include <netinet/in.h>

class Client
{
public:
	int fd;
	struct pollfd *pfdp;
	socklen_t addrlen;
	struct sockaddr_in addr;
	std::string nick;
	std::string username;
	std::string realname;
	std::string hostname;
	std::string inbuf;
	bool is_operator;
	
	// Registration requirements
	bool password_is_valid;
	bool nick_given;
	bool user_given;
	bool is_fully_registered;

	Client();
	Client(const Client& other);
	~Client();
	
	Client& operator=(const Client& other);
	
	// Utility methods
	void setNick(const std::string& new_nick);
	void setUser(const std::string& username, const std::string& realname);
	bool isRegistered() const;
};

#endif
