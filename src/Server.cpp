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
Server::~Server() {}

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

		// MOVE TO FUNCTION vvv
		char buf[BUFFER_SIZE];
		for (int i = 1; i < _nfds; i++)
		{
			if (_pfds[i].revents & POLLIN)
			{
				int bytes = read(_pfds[i].fd, buf, BUFFER_SIZE);
				if (bytes <= 0)
				{
					//client disconnected
					close(_pfds[i].fd);
					_pfds[i].fd = -1;
					_nfds--;
					_clients[i - 1] = Client();
					std::cout << "Client disconnected" << std::endl;
				} else {
					buf[bytes] = '\0';
					std::cout << "Message received: <" << buf << ">" << std::endl;
					parse_message(std::string(buf), _clients[i - 1]);
				}
			}
		}
		// MOVE TO FUNCTION ^^^
		
	}
}

void Server::parse_message(std::string message, Client& sender)
{
	// CURRENT CODE IS FOR TESTING PURPOSES, NOT IRC COMPLIANT AT ALL
	// ALSO DISGUSTING
	if (message.compare(0, 6, "CAP LS") == 0) {
		write(sender.fd, "CAP * LS\r\n", 11);
		return ;
	}

	if (message.compare(0, 5, "PASS ") == 0) {
		if (message.compare(5, message.size() - 5, _pass + "\r\n") == 0) {
			sender.password_is_valid = true;
			// right password
		} else {
			// wrong password
			write(sender.fd, "ERROR :Wrong Password", 22);
			close(sender.fd);
			sender.pfdp->fd = -1;
			_nfds--;
		}
		return ;
	} else if (message.compare(0, 4, "PASS") == 0) {
		// no password given
		write(sender.fd, "ERROR :Wrong Password", 22); // NOT THE CORRECT ERROR / TESTING PURPOSES
		close(sender.fd);
		sender.pfdp->fd = -1;
		_nfds--;
		return ;
	}

	if (!sender.password_is_valid) {
		write(sender.fd, "ERROR :Password required", 25); // NOT THE CORRECT ERROR / TESTING PURPOSES
		std::cout << "PASSWORD REQUIRED" << std::endl;
		close(sender.fd);
		sender.pfdp->fd = -1;
		_nfds--;
		return ;
	}

	//Parses the message received, acts on it and sends to appropriate response to sender ig
	//should make this different parts maybe

	// std::istringstream msgs(message);
	// std::string cmd;
	// std::getline(msgs, cmd, ' '); //gets first word

	// if (cmd == "")
}

size_t Server::find_empty_slot()
{
	for (size_t i = 1; i <= MAX_CLIENTS; i++)
	{
		if (_pfds[i].fd == -1)
			return i;
	}
	return -1; //should not happen because only called if server is not full
}

void Server::accept_new_clients()
{
	// int newsockfd;
	// struct sockaddr_in cli_addr;
	// socklen_t addr_len = sizeof(cli_addr);

	while (_pfds[0].revents & POLLIN)
	{
		Client client;
		client.fd = accept(_sockfd, (struct sockaddr *)&client.addr, &client.addrlen);
		// newsockfd = accept(_sockfd, (struct sockaddr *)&cli_addr, &addr_len);
		if (client.fd < 0)
		// if (newsockfd < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				break ;
			perror("accept error");
			break ;
		}
		if (_nfds <= MAX_CLIENTS)
		{
			_nfds++;
			size_t slot = find_empty_slot();
			_pfds[slot].fd = client.fd;
			// _pfds[_nfds].fd = newsockfd;
			_pfds[slot].events = POLLIN;
			client.pfdp = &(_pfds[slot]);
			_clients[slot - 1] = client;
			//add_client(newsockfd, cli_addr)
			std::cout << "Connection accepted: " << client.fd << std::endl;
		}
		else
		{
			std::cout << "Connection refused. Sever is full." << std::endl;
			write(client.fd, "ERROR :Server full, try again later\r\n", 38);
			close(client.fd);
			// close(newsockfd);
		}
	}
}