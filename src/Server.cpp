#include "Server.hpp"
#include "ft_irc.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>

Server::Server(int port, std::string pass): _port(port), _pass(pass), _sockfd(-1), _nfds(0)
{
	// Initialize client array
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		_clients[i] = Client();
	}
}

Server::~Server() 
{
	if (_sockfd != -1)
		close(_sockfd);
}

void Server::init()
{
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
		error("socket error");
	
	// Allow port reuse to avoid TIME_WAIT state
	int opt = 1;
	setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	
	// Make socket non-blocking
	int flags = fcntl(_sockfd, F_GETFL, 0);
	fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);

	// Configure server address
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons(_port);

	if (bind(_sockfd, (struct sockaddr *) &_addr, sizeof(_addr)) < 0)
		error("bind error");

	// Initialize poll array
	_nfds = 1;
	_pfds[0].fd = _sockfd;
	_pfds[0].events = POLLIN;
	
	// Mark all client slots as empty
	for (int i = 1; i <= MAX_CLIENTS; i++)
		_pfds[i].fd = -1;
}

void Server::run()
{
	listen(_sockfd, SOMAXCONN);
	std::cout << "Server is now listening on socket " << _sockfd << std::endl;
	std::cout << "Address: " << inet_ntoa(_addr.sin_addr) << ":" << _port << std::endl;

	while (true)
	{
		if (poll(_pfds, _nfds, -1) < 0)
			error("poll error");
		
		// Handle new connections
		accept_new_clients();

		// Process messages from existing clients
		process_client_messages();
	}
}

void Server::process_client_messages()
{
	char buf[BUFFER_SIZE];
	
	for (int i = 1; i < _nfds; i++)
	{
		if (_pfds[i].revents & POLLIN)
		{
			int bytes = read(_pfds[i].fd, buf, BUFFER_SIZE);
			if (bytes <= 0)
			{
				handle_client_disconnect(i);
			} 
			else 
			{
				buf[bytes] = '\0';
				std::cout << "Message received: <" << buf << ">" << std::endl;
				parse_message(std::string(buf), _clients[i - 1]);
			}
		}
	}
}

void Server::handle_client_disconnect(int client_index)
{
	close(_pfds[client_index].fd);
	_pfds[client_index].fd = -1;
	_clients[client_index - 1] = Client();
	
	// Compact the poll array
	for (int j = client_index; j < _nfds - 1; j++)
	{
		_pfds[j] = _pfds[j + 1];
		_clients[j - 1] = _clients[j];
	}
	_nfds--;
	
	std::cout << "Client disconnected" << std::endl;
}

void Server::parse_message(std::string message, Client& sender)
{
	// Parse the IRC message
	Message irc_message = parse_irc_message(message);
	
	// Handle the parsed command
	handle_command(irc_message, sender);
}

Message Server::parse_irc_message(const std::string& raw_message)
{
	return Message(raw_message);
}

void Server::handle_command(const Message& msg, Client& sender)
{
	const std::string& command = msg.getCommand();
	
	// Handle CAP LS command
	if (command == "CAP" && msg.getParamCount() >= 1 && msg.getParams()[0] == "LS") {
		write(sender.fd, "CAP * LS\r\n", 11);
		return;
	}

	// Handle PASS command
	if (command == "PASS") {
		if (msg.getParamCount() >= 1) {
			std::string password = msg.getParams()[0];
			if (password == _pass) {
				sender.password_is_valid = true;
				write(sender.fd, "PASS :Password accepted\r\n", 25);
			} else {
				write(sender.fd, "ERROR :Wrong Password\r\n", 23);
				close(sender.fd);
				return;
			}
		} else {
			write(sender.fd, "ERROR :Password required\r\n", 25);
			close(sender.fd);
			return;
		}
		return;
	}

	// Require password for other commands
	if (!sender.password_is_valid) {
		write(sender.fd, "ERROR :Password required\r\n", 25);
		return;
	}

	// TODO: Implement other IRC commands (NICK, USER, JOIN, etc.)
	std::cout << "Unhandled command: " << command << " with " << msg.getParamCount() << " params" << std::endl;
	if (msg.hasTrailing()) {
		std::cout << "Unhandled command: " << command << " with " << msg.getParamCount() << " params" << std::endl;
	}
}

size_t Server::find_empty_slot()
{
	for (size_t i = 1; i <= MAX_CLIENTS; i++)
	{
		if (_pfds[i].fd == -1)
			return i;
	}
	return 0; // No empty slot available
}

void Server::accept_new_clients()
{
	while (_pfds[0].revents & POLLIN)
	{
		Client client;
		client.addrlen = sizeof(client.addr);
		
		client.fd = accept(_sockfd, (struct sockaddr *)&client.addr, &client.addrlen);
		if (client.fd < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				break;
			perror("accept error");
			break;
		}
		
		if (_nfds <= MAX_CLIENTS)
		{
			size_t slot = find_empty_slot();
			if (slot > 0)
			{
				_pfds[slot].fd = client.fd;
				_pfds[slot].events = POLLIN;
				client.pfdp = &(_pfds[slot]);
				_clients[slot - 1] = client;
				_nfds++;
				std::cout << "Connection accepted: " << client.fd << std::endl;
			}
			else
			{
				std::cout << "No available slots for new connection" << std::endl;
				close(client.fd);
			}
		}
		else
		{
			std::cout << "Connection refused. Server is full." << std::endl;
			write(client.fd, "ERROR :Server full, try again later\r\n", 38);
			close(client.fd);
		}
	}
}