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

#define DEFAULT_PORT 6667
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

class Client
{
private:
	int _fd;
	// struct pollfd &pfd;
	struct sockaddr_in _addr;
	std::string _nick;
	bool	_operator;
	bool	_is_fully_connected; //has it given pass nick user
public:
	Client();
	// Client(int fd, std::string nick);
	~Client();
	void	set_fd(int &);
	int		get_fd();
	void	set_nick(std::string);
	int		get_nick();
	bool	is_operator();
	void	set_operator(bool);
};

class Server
{
private:
	int _port;
	std::string _pass;

	int _sockfd;
	static bool _sig;
	struct sockaddr_in _addr;
	std::vector<Client> _clients;
	int	_nfds;
	struct pollfd _pfds[MAX_CLIENTS + 1];
	
	void accept_new_clients();
	void add_client(int clifd);
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