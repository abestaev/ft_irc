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
#include <signal.h>
#include <vector>

// Initialize static member
bool Server::_sig = false;

Server::Server(int port, std::string pass): _port(port), _pass(pass), _sockfd(-1), _nfds(0)
{
	// Initialize client array
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		_clients[i] = Client();
	}
	
	// Initialize commands handler
	_commands = new Commands(this);
	
	// Setup signal handlers
	setup_signal_handlers();
}

Server::~Server() 
{
	if (_sockfd != -1)
		close(_sockfd);
	if (_commands)
		delete _commands;
}

void Server::signal_handler(int sig)
{
	(void)sig;
	std::cout << "\nReceived signal, shutting down server..." << std::endl;
	_sig = true;
}

void Server::setup_signal_handlers()
{
	signal(SIGINT, signal_handler);   // Ctrl+C
	signal(SIGTERM, signal_handler);  // kill command
}

void Server::init()
{
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
		error("socket error");
	
	int opt = 1;
	setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	// Make listening socket non-blocking
	int lflags = fcntl(_sockfd, F_GETFL, 0);
	if (lflags != -1)
		fcntl(_sockfd, F_SETFL, lflags | O_NONBLOCK);
	

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
	std::cout << "Address: " << inet_ntoa(_addr.sin_addr) << ":" << _port << std::endl;
	std::cout << "Press Ctrl+C to stop the server" << std::endl;

	while (!_sig)
	{
		int poll_result = poll(_pfds, _nfds, 1000);
		
		if (poll_result < 0)
		{
			if (errno == EINTR)
			{
				if (_sig)
					break;
				continue;
			}
			error("poll error");
		}
		else if (poll_result > 0)
		{
			if (_pfds[0].revents & POLLIN)
			{
				accept_new_clients();
				_pfds[0].revents = 0;
			}

			if (_nfds > 1) {
				process_client_messages();
			}
		}
	}
	
	std::cout << "Server shutting down..." << std::endl;
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
				if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
				{
					// No data available on non-blocking socket
				}
				else
				{
					handle_client_disconnect(i);
					i--; // adjust index since we shifted arrays
				}
			} 
			else 
			{
				if (bytes >= BUFFER_SIZE)
					bytes = BUFFER_SIZE - 1;
				buf[bytes] = '\0';
				if (i - 1 < MAX_CLIENTS) {
					_clients[i - 1].inbuf.append(buf);
					std::string::size_type pos;
					while ((pos = _clients[i - 1].inbuf.find("\r\n")) != std::string::npos) {
						std::string line = _clients[i - 1].inbuf.substr(0, pos + 2);
						_clients[i - 1].inbuf.erase(0, pos + 2);
						parse_message(line, _clients[i - 1]);
					}
				}
			}
		}
		
		if (_pfds[i].revents & POLLOUT)
		{
			_pfds[i].events &= ~POLLOUT;
		}
		
		if (_pfds[i].revents & POLLERR)
		{
			handle_client_disconnect(i);
		}
		
		if (_pfds[i].revents != 0) {
			_pfds[i].revents = 0;
		}
	}
}

void Server::handle_client_disconnect(int client_index)
{
	if (client_index < 1 || client_index >= _nfds) {
		return;
	}
	
	close(_pfds[client_index].fd);
	_pfds[client_index].fd = -1;
	_clients[client_index - 1] = Client();
	
	for (int j = client_index; j < _nfds - 1; j++)
	{
		_pfds[j] = _pfds[j + 1];
		if (j > 0 && j < MAX_CLIENTS) {
			_clients[j - 1] = _clients[j];
		}
	}
	_nfds--;
}

void Server::parse_message(std::string message, Client& sender)
{
	if (message.empty() || message.find_first_not_of(" \t\r\n") == std::string::npos) {
		return;
	}
	
	Message irc_message = parse_irc_message(message);
	
	if (!irc_message.getCommand().empty()) {
		handle_command(irc_message, sender);
	}
}

Message Server::parse_irc_message(const std::string& raw_message)
{
	return Message(raw_message);
}

void Server::handle_command(const Message& msg, Client& sender)
{
	_commands->execute_command(msg, sender);
}
Channel* Server::find_channel(const std::string& name)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (_channels[i].getName() == name)
			return &(_channels[i]);
	}
	return NULL;
}

Channel* Server::get_or_create_channel(const std::string& name)
{
	Channel* ch = find_channel(name);
	if (ch)
		return ch;
	_channels.push_back(Channel(name));
	return &(_channels.back());
}

size_t Server::find_empty_slot()
{
	for (size_t i = 1; i <= MAX_CLIENTS; i++)
	{
		if (_pfds[i].fd == -1)
			return i;
	}
	return 0;
}

void Server::accept_new_clients()
{
	int connections_processed = 0;
	const int MAX_CONNECTIONS_PER_ITERATION = 5;
	
	while (_pfds[0].revents & POLLIN && connections_processed < MAX_CONNECTIONS_PER_ITERATION)
	{
		Client client;
		client.addrlen = sizeof(client.addr);
		
		client.fd = accept(_sockfd, (struct sockaddr *)&client.addr, &client.addrlen);
		if (client.fd < 0)
		{
			perror("accept error");
			break;
		}
		// accepted new connection
		
		int flags = fcntl(client.fd, F_GETFL, 0);
		fcntl(client.fd, F_SETFL, flags | O_NONBLOCK);
		
		if ((_nfds - 1) < MAX_CLIENTS)
		{
			size_t slot = find_empty_slot();
			if (slot > 0 && slot <= MAX_CLIENTS)
			{
				_pfds[slot].fd = client.fd;
				_pfds[slot].events = POLLIN;
				client.pfdp = &(_pfds[slot]);
				_clients[slot - 1] = client;
				_nfds++;
				connections_processed++;
				// client assigned to slot
			}
			else
			{
				close(client.fd);
			}
		}
		else
		{
			write(client.fd, "ERROR :Server full, try again later\r\n", 38);
			close(client.fd);
		}
	}
	
	_pfds[0].revents = 0;
	
}