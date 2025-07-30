#ifndef FT_IRC_HPP
#define FT_IRC_HPP

#include <iostream>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <sstream>

#define DEFAULT_PORT 6667
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// enum reg_step
// {
// 	ACCEPTED,
// 	CAP_NEG,
// 	PASS,
// 	NICK,
// 	USER,
// 	END
// };

class Client //not a big fan of how everything is public but so much simpler. will change later
{
private:
public:
	int fd;
	// struct pollfd &pfd;
	socklen_t addrlen;
	struct sockaddr_in addr;
	std::string nick;
	bool	is_operator;
	// enum reg_step registration_step;

	// registration requirements:
	// CAP not requiered
	bool password_is_valid; //reqired before anything else (exept CAP)
	bool nick_given;
	bool user_given;


	bool	is_fully_registered; //has it given pass capls nick user
	Client();
	// Client(int fd, std::string nick);
	~Client();
// private:
// 	int _fd;
// 	// struct pollfd &pfd;
// 	struct sockaddr_in _addr;
// 	std::string _nick;
// 	bool	_operator;
// 	bool	_is_fully_registered; //has it given pass nick user
// public:
// 	Client();
// 	// Client(int fd, std::string nick);
// 	~Client();
// 	void	set_fd(int &);
// 	int		get_fd();
// 	void	set_nick(std::string);
// 	int		get_nick();
// 	bool	is_operator();
// 	void	set_operator(bool);
//	struct sockaddr_in get_addr();
//	void set_addr(struct sockaddr_in)
};

class Server
{
private:
	int _port;
	std::string _pass;

	int _sockfd;
	static bool _sig;
	struct sockaddr_in _addr;
	// std::vector<Client> _clients;
	Client _clients[MAX_CLIENTS];
	int	_nfds;
	struct pollfd _pfds[MAX_CLIENTS + 1];
	
	void accept_new_clients();
	size_t find_empty_slot();
	void parse_message(std::string, Client);
	// void add_client(int clifd, struct sockaddr_in cliaddr);
	// Server();
public:
	Server(int, std::string);
	~Server();

	void init(); //used to be in the constructor
	void run(); //start server and loop
};

class Channel
{
private:
	std::string _owner;
};

void error(std::string msg);

#endif