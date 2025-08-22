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
	
	// Mark all client slots as empty - CORRECTION: Change <= to <
	for (int i = 1; i < MAX_CLIENTS; i++)
		_pfds[i].fd = -1;
}

void Server::run()
{
	listen(_sockfd, SOMAXCONN);
	std::cout << "Server is now listening on socket " << _sockfd << std::endl;
	std::cout << "Address: " << inet_ntoa(_addr.sin_addr) << ":" << _port << std::endl;
	std::cout << "Press Ctrl+C to stop the server" << std::endl;

	while (!_sig)  // Changed from while(true) to while(!_sig)
	{
		std::cout << "Poll loop iteration - nfds: " << _nfds << " (max: " << MAX_CLIENTS << ")" << std::endl;
		
		// Use a timeout of 1000ms (1 second) instead of -1 (blocking)
		int poll_result = poll(_pfds, _nfds, 1000);
		std::cout << "Poll result: " << poll_result << std::endl;
		
		if (poll_result < 0)
		{
			if (errno == EINTR)
			{
				// Interrupted by signal, check if we should stop
				if (_sig)
					break;
				continue;
			}
			error("poll error");
		}
		else if (poll_result == 0)
		{
			std::cout << "Poll timeout (no events)" << std::endl;
		}
		else
		{
			std::cout << "Poll returned " << poll_result << " events" << std::endl;
			
			// CRITICAL FIX: Check for new connections FIRST, then process messages
			if (_pfds[0].revents & POLLIN)
			{
				std::cout << "New connection event detected" << std::endl;
				accept_new_clients();
				// CRITICAL FIX: Reset the event flag to prevent infinite loop
				_pfds[0].revents = 0;
			}

			// CRITICAL FIX: Only process client messages if there are clients
			if (_nfds > 1) {
				std::cout << "Processing messages from " << (_nfds - 1) << " clients" << std::endl;
				process_client_messages();
			} else {
				std::cout << "No clients to process messages from" << std::endl;
			}
		}
	}
	
	std::cout << "Server shutting down..." << std::endl;
}

void Server::process_client_messages()
{
	char buf[BUFFER_SIZE];
	
	std::cout << "DEBUG: process_client_messages() called with _nfds = " << _nfds << std::endl;
	
	for (int i = 1; i < _nfds; i++)
	{
		std::cout << "DEBUG: Checking client " << i << " (fd: " << _pfds[i].fd << ", revents: " << _pfds[i].revents << ")" << std::endl;
		
		// CRITICAL FIX: Handle all event types, not just POLLIN
		if (_pfds[i].revents & POLLIN)
		{
			std::cout << "Reading from client " << i << " (fd: " << _pfds[i].fd << ")" << std::endl;
			
			int bytes = read(_pfds[i].fd, buf, BUFFER_SIZE);
			if (bytes <= 0)
			{
				std::cout << "Client " << i << " disconnected (bytes: " << bytes << ")" << std::endl;
				handle_client_disconnect(i);
			} 
			else 
			{
				buf[bytes] = '\0';
				std::cout << "Message received from client " << i << ": <" << buf << ">" << std::endl;
				
				// CRITICAL FIX: Validate client index before accessing
				if (i - 1 < MAX_CLIENTS) {
					parse_message(std::string(buf), _clients[i - 1]);
				} else {
					std::cout << "ERROR: Invalid client index " << (i - 1) << " for client " << i << std::endl;
				}
			}
		}
		
		// CRITICAL FIX: Handle POLLOUT events (client ready to receive data)
		if (_pfds[i].revents & POLLOUT)
		{
			std::cout << "Client " << i << " ready to receive data (POLLOUT)" << std::endl;
			
			// TEST: Send a welcome message to the client
			std::string welcome_msg = "Welcome to ft-irc server!\r\n";
			int bytes_sent = write(_pfds[i].fd, welcome_msg.c_str(), welcome_msg.length());
			if (bytes_sent > 0) {
				std::cout << "Sent welcome message to client " << i << " (" << bytes_sent << " bytes)" << std::endl;
			} else {
				std::cout << "Failed to send welcome message to client " << i << std::endl;
			}
		}
		
		// CRITICAL FIX: Handle POLLERR events (client error)
		if (_pfds[i].revents & POLLERR)
		{
			std::cout << "Client " << i << " has error (POLLERR) - fd: " << _pfds[i].fd << std::endl;
			
			// Get socket error details
			int error = 0;
			socklen_t len = sizeof(error);
			if (getsockopt(_pfds[i].fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
				std::cout << "Socket error code: " << error << " (" << strerror(error) << ")" << std::endl;
			}
			
			handle_client_disconnect(i);
		}
		
		// CRITICAL FIX: Reset event flags ONLY if we processed them
		if (_pfds[i].revents != 0) {
			std::cout << "DEBUG: Resetting events for client " << i << " (was: " << _pfds[i].revents << ")" << std::endl;
			_pfds[i].revents = 0;
		}
	}
}

void Server::handle_client_disconnect(int client_index)
{
	// CRITICAL FIX: Validate client_index before processing
	if (client_index < 1 || client_index >= _nfds) {
		std::cout << "Invalid client index: " << client_index << std::endl;
		return;
	}
	
	close(_pfds[client_index].fd);
	_pfds[client_index].fd = -1;
	_clients[client_index - 1] = Client();
	
	// Compact the poll array - IMPROVED: Better bounds checking
	for (int j = client_index; j < _nfds - 1; j++)
	{
		_pfds[j] = _pfds[j + 1];
		// CRITICAL FIX: Ensure we don't access invalid array indices
		if (j > 0 && j < MAX_CLIENTS) {
			_clients[j - 1] = _clients[j];
		}
	}
	_nfds--;
	
	std::cout << "Client disconnected from slot " << client_index << std::endl;
}

void Server::parse_message(std::string message, Client& sender)
{
	std::cout << "Parsing message: <" << message << "> for client " << sender.fd << std::endl;
	
	// Parse the IRC message
	Message irc_message = parse_irc_message(message);
	
	std::cout << "Parsed command: " << irc_message.getCommand() << " with " << irc_message.getParamCount() << " params" << std::endl;
	
	// Handle the parsed command
	handle_command(irc_message, sender);
}

Message Server::parse_irc_message(const std::string& raw_message)
{
	return Message(raw_message);
}

void Server::handle_command(const Message& msg, Client& sender)
{
	// Use the Commands class to handle all IRC commands
	_commands->execute_command(msg, sender);
}

size_t Server::find_empty_slot()
{
	// CORRECTION: Change <= to < to prevent array overflow
	for (size_t i = 1; i < MAX_CLIENTS; i++)
	{
		if (_pfds[i].fd == -1)
			return i;
	}
	return 0; // No empty slot available
}

void Server::accept_new_clients()
{
	// CRITICAL FIX: Limit the number of connections processed per iteration
	int connections_processed = 0;
	const int MAX_CONNECTIONS_PER_ITERATION = 5;
	
	while (_pfds[0].revents & POLLIN && connections_processed < MAX_CONNECTIONS_PER_ITERATION)
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
		
		// IMPROVED: Better bounds checking
		if (_nfds < MAX_CLIENTS)
		{
			size_t slot = find_empty_slot();
			if (slot > 0)
			{
				_pfds[slot].fd = client.fd;
				_pfds[slot].events = POLLIN;
				client.pfdp = &(_pfds[slot]);
				_clients[slot - 1] = client;
				_nfds++;
				connections_processed++;
				
				// DEBUG: Check socket state after accept
				int flags = fcntl(client.fd, F_GETFL, 0);
				std::cout << "Connection accepted: " << client.fd << " in slot " << slot << " (nfds: " << _nfds << ")" << std::endl;
				std::cout << "DEBUG: Socket flags: " << flags << " (O_NONBLOCK: " << O_NONBLOCK << ")" << std::endl;
			}
			else
			{
				std::cout << "No available slots for new connection" << std::endl;
				close(client.fd);
			}
		}
		else
		{
			std::cout << "Connection refused. Server is full (nfds: " << _nfds << ")" << std::endl;
			write(client.fd, "ERROR :Server full, try again later\r\n", 38);
			close(client.fd);
		}
	}
	
	// CRITICAL FIX: Reset the event flag to prevent infinite loop
	_pfds[0].revents = 0;
	
	if (connections_processed > 0) {
		std::cout << "Processed " << connections_processed << " new connections" << std::endl;
	}
}