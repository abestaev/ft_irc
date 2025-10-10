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
	std::string outbuf;  // Output buffer for non-blocking writes
	bool is_operator;
	
	// User modes
	bool is_invisible;
	bool is_wallops;
	bool is_restricted;
	bool is_global_operator;
	bool is_local_operator;
	
	// Registration requirements
	bool ongoing_negociation;
	bool password_is_valid;
	bool nick_given;
	bool user_given;
	bool is_fully_registered;

	/*

	if CAP LS
	set ongoing_negociation and reply CAP * LS
	until CAP END is received, then unset ongoing_negociation

	if !ongoing_negociation and received CAP END
		ignore
	
	if received CAP REQ :cap_name
	respond with CAP NAK :cap_name

	ready_for_registration: has received nick and user

	if ready_for_negociation and !ongoing_negociation
		attempt registration

	registration attempt:
		- check if password is valid. if it isnt, send error 464 and close connection.
		- check if nickname is valid (no collision or )
	*/

	Client();
	Client(const Client& other);
	~Client();
	
	Client& operator=(const Client& other);
	
	// Utility methods
	void setNick(const std::string& new_nick);
	void setUser(const std::string& username, const std::string& realname);
	// bool isRegistered() const;
	bool isReadyForRegistration() const;
	std::string getUserModes() const;
};

#endif
