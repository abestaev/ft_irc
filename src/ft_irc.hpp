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

#define DEFAULT_PORT 6667
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

class Client
{
private:
	int fd;
	std::string addr;
public:
	Client();
	~Client();
};

class Server
{
private:
	int _port;
	int _socketfd;
	static bool _sig;
	std::vector<Client> _clients;

public:
	Server();
	~Server();

	// void init() i feel like this should be in the constructor
	// void create_socket();
};


#endif