#include "ft_irc.hpp"

// Server::Server(): _port(-1), _pass("")
// {}

Server::Server(int port, std::string pass): _port(port), _pass(pass)
{
	//////// MOVED TO INIT_SERVER METHOD /////////
	// _sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// if (_sockfd < 0)
	// 	error("socket error");
	// // override "TIME_WAIT state" behavior / allows port to be reused immediatly
	// int opt = 1; //P
	// setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	// int flags = fcntl(_sockfd, F_GETFL, 0);
	// fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);

	// _addr.sin_family = AF_INET;
	// _addr.sin_addr.s_addr = INADDR_ANY;
	// _addr.sin_port = htons(_port);

	// if (bind(_sockfd, (struct sockaddr *) &_addr, sizeof(_addr)) < 0)
	// 	error("bind error");

	// _nfds = 1;
	// _pfds[0].fd = _sockfd;
	// _pfds[0].events = POLLIN;
	// for (int i = 1; i <= MAX_CLIENTS; i++)
	// 	_pfds[i].fd = -1;

	//listen(sockfd, SOMAXCONN);
}

void	Server::init()
{
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
		error("socket error");
	// override "TIME_WAIT state" behavior / allows port to be reused immediatly
	int opt = 1; //P
	setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	int flags = fcntl(_sockfd, F_GETFL, 0);
	fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);

	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons(_port);

	if (bind(_sockfd, (struct sockaddr *) &_addr, sizeof(_addr)) < 0)
		error("bind error");

	_nfds = 1;
	_pfds[0].fd = _sockfd;
	_pfds[0].events = POLLIN;
	for (int i = 1; i <= MAX_CLIENTS; i++)
		_pfds[i].fd = -1;
}

void Server::run()
{
	listen(_sockfd, SOMAXCONN);
	std::cout << "Server is now listening on socket " << _sockfd << std::endl;
	std::cout << "Address: " << inet_ntoa(_addr.sin_addr) << std::endl;

	while (true)
	{
		if (poll(_pfds, _nfds, -1) < 0)
			error("poll error");
		
		//new connections
		accept_new_clients();
	}
}

void Server::accept_new_clients()
{
	int newsockfd;
	struct sockaddr_in cli_addr;
	socklen_t addr_len = sizeof(cli_addr);

	while (_pfds[0].revents & POLLIN)
	{
		Client client;
		newsockfd = accept(_sockfd, (struct sockaddr *)&cli_addr, &addr_len);
		if (newsockfd < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				break ;
			perror("accept error");
			break ;
		}
		if (_nfds <= MAX_CLIENTS)
		{
			_pfds[_nfds].fd = newsockfd;
			_pfds[_nfds].events = POLLIN;
			_nfds++;
			
		}
	}
}