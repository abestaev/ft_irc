#include "Server.hpp"
#include "ft_irc.hpp"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <arpa/inet.h>
#include <cstdio>
#include <signal.h>
#include <ctime>
#include <netinet/tcp.h>

// Initialize static member
bool Server::_sig = false;

Server::Server(int port, std::string pass) : _port(port), _pass(pass), _sockfd(-1), _nfds(0)
{
	// Initialize client array
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		_clients[i] = Client();
	}

	// Initialize commands handler
	_commands = new Commands(this);

	// Capture server creation time (human readable)
	{
		char timebuf[64];
		time_t now = time(NULL);
		struct tm *ptm = localtime(&now);
		if (ptm)
			strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S %Z", ptm);
		else
			snprintf(timebuf, sizeof(timebuf), "%ld", (long)now);
		_createdAt = std::string(timebuf);
	}

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
	signal(SIGINT, signal_handler);	 // Ctrl+C
	signal(SIGTERM, signal_handler); // kill command
	signal(SIGPIPE, SIG_IGN);		 // ignore SIGPIPE on send() to closed sockets
}

void Server::init()
{
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0)
	{
		std::cout << "\033[31m[ERROR]\033[0m Failed to create socket: " << strerror(errno) << std::endl;
		error("socket error");
	}

	int opt = 1;
	setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	// Make listening socket non-blocking
	fcntl(_sockfd, F_SETFL, O_NONBLOCK);

	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons(_port);

	if (bind(_sockfd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
	{
		std::cout << "\033[31m[ERROR]\033[0m Failed to bind socket: " << strerror(errno) << std::endl;
		error("bind error");
	}

	_nfds = 1;
	_pfds[0].fd = _sockfd;
	_pfds[0].events = POLLIN;
	_pfds[0].revents = 0;

	for (int i = 1; i <= MAX_CLIENTS; i++)
	{
		_pfds[i].fd = -1;
		_pfds[i].events = 0;
		_pfds[i].revents = 0;
	}
}

void Server::run()
{
	if (listen(_sockfd, SOMAXCONN) < 0)
	{
		std::cout << "\033[31m[ERROR]\033[0m Failed to listen on socket: " << strerror(errno) << std::endl;
		error("listen error");
	}

	std::cout << "\033[32m[INFO]\033[0m Listening on " << inet_ntoa(_addr.sin_addr) << ":" << _port << std::endl;
	std::cout << "Press Ctrl+C to stop the server" << std::endl;

	while (!_sig)
	{
		int poll_result = poll(_pfds, _nfds, 100);

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

			if (_nfds > 1)
			{
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
		if (_clients[i - 1].marked_for_death && _clients[i - 1].outbuf.empty())
			handle_client_disconnect(i);

	for (int i = 1; i < _nfds; i++)
	{
		if (_pfds[i].revents & POLLIN)
		{
			// Read once per poll() call (subject requirement)
			int bytes = read(_pfds[i].fd, buf, BUFFER_SIZE);
			buf[bytes] = 0;
			// if (buf[bytes - 1] == '\n')
			// 	buf[bytes - 1] = 0;
			if (bytes > 0)
			{
				if (i - 1 < MAX_CLIENTS)
				{
					_clients[i - 1].inbuf.append(buf, bytes);
					std::cout << "[INCOMPLETE] (fd: " << _pfds[i].fd << ") -> " << buf << std::endl; 
				}
			}
			else if (bytes == 0)
			{
				// Peer closed connection
				handle_client_disconnect(i);
				i--;
				continue;
			}
			else
			{
				// Read error (bytes < 0)
				// Subject forbids using errno to trigger actions; treat as transient
				std::cout << "\033[31m[ERROR]\033[0m Failed to read from socket fd:" << _pfds[i].fd
					<< " (" << strerror(errno) << ")" << std::endl;
				// Do not disconnect here; rely on POLLERR/POLLHUP to close
				continue;
			}

			// Parse all complete lines from buffer
			if (i - 1 < MAX_CLIENTS)
			{
				std::string &inbuf = _clients[i - 1].inbuf;
				while (true)
				{
					std::string::size_type posCRLF = inbuf.find("\r\n");
					if (posCRLF != std::string::npos)
					{
						std::string raw = inbuf.substr(0, posCRLF + 2);
						inbuf.erase(0, posCRLF + 2);
						parse_message(raw, _clients[i - 1]);
						continue;
					}
					// std::string::size_type posLF = inbuf.find('\n');
					// if (posLF != std::string::npos)
					// {
					// 	std::string line = inbuf.substr(0, posLF);
					// 	// strip optional preceding CR
					// 	if (!line.empty() && line[line.size() - 1] == '\r')
					// 	{
					// 		line.erase(line.size() - 1);
					// 	}
					// 	// normalize to CRLF for parser
					// 	std::string raw = line + "\r\n";
					// 	inbuf.erase(0, posLF + 1);
					// 	parse_message(raw, _clients[i - 1]);
					// 	continue;
					// }
					break;
				}
			}
		}

		if (_pfds[i].revents & POLLOUT)
		{
			// Socket is writable, send once per poll() call (subject requirement)
			if (i - 1 < MAX_CLIENTS && !_clients[i - 1].outbuf.empty())
			{
				ssize_t sent = send(_clients[i - 1].fd, _clients[i - 1].outbuf.c_str(), _clients[i - 1].outbuf.length(), 0);
				if (sent > 0)
				{
					_clients[i - 1].outbuf.erase(0, sent);

					// If buffer now empty, disable POLLOUT
					if (_clients[i - 1].outbuf.empty())
					{
						_pfds[i].events &= ~POLLOUT;

						// If this was a "server full" temp client, disconnect now
						if (_clients[i - 1].nick == "FULL_SERVER_TEMP")
						{
							handle_client_disconnect(i);
							i--;
							continue;
						}
					}
				}
				else
				{
					// Send error (sent <= 0)
					// Subject forbids using errno to trigger actions; treat as transient
					// Keep data in outbuf, retry on next POLLOUT or close on POLLERR/POLLHUP
					continue;
				}
			}
			else
			{
				// No data to send, disable POLLOUT
				_pfds[i].events &= ~POLLOUT;
			}
		}

		if (_pfds[i].revents & POLLERR)
		{
			handle_client_disconnect(i);
			i--;
			continue;
		}

		if (_pfds[i].revents & (POLLHUP | POLLNVAL))
		{
			handle_client_disconnect(i);
			i--;
			continue;
		}

		if (_pfds[i].revents != 0)
		{
			_pfds[i].revents = 0;
		}
	}
}

// void Server::handle_client_disconnect(int client_index)
// {
// 	if (client_index < 1 || client_index >= MAX_CLIENTS || _pfds[client_index].fd == -1) {
// 		return;
// 	}

//     // Preserve client before wiping to clean channels correctly
//     Client departing = _clients[client_index - 1];

// 	// Log disconnection
//     std::string client_nick = _clients[client_index - 1].nick;
// 	if (client_nick.empty()) {
// 		client_nick = "unregistered";
// 	}
// 	std::cout << "\033[33m[DISCONNECT]\033[0m Client " << client_nick
// 			  << " (fd:" << _pfds[client_index].fd << ") disconnected" << std::endl;

//     // Remove the client from all channels and delete empty channels
//     for (size_t i = 0; i < _channels.size(); ++i) {
//         if (_channels[i].has_client(departing)) {
//             _channels[i].remove_client(departing);
//             if (_channels[i].get_user_count() == 0) {
//                 std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << _channels[i].getName() << std::endl;
//                 _channels.erase(_channels.begin() + i);
//                 i--;
//             }
//         }
//     }

// 	close(_pfds[client_index].fd);
// 	_pfds[client_index].fd = -1;
// 	_clients[client_index - 1] = Client();

// 	for (int j = client_index; j < _nfds - 1; j++)
// 	{
// 		_pfds[j] = _pfds[j + 1];
// 		_pfds[j + 1].fd = -1;
// 		if (j > 0 && j < MAX_CLIENTS) {
// 			_clients[j - 1] = _clients[j];
// 			// Re-bind the pfd pointer to the shifted pollfd slot
// 			_clients[j - 1].pfdp = &(_pfds[j]);
// 			_clients[j] = Client();
// 		}
// 	}
// 	_nfds--;
// }

void Server::handle_client_disconnect(int client_index)
{
	if (client_index < 1 || client_index >= MAX_CLIENTS)
		return;

	Client client = _clients[client_index - 1];

	// LOG DISCONNECTION
	std::cout << "\033[33m[DISCONNECT]\033[0m Client " << (client.is_fully_registered ? client.nick : "unregistered")
			  << " (fd:" << _pfds[client_index].fd << ") disconnected" << std::endl;

	std::vector<Channel>::iterator it;
	for (it = _channels.begin(); it != _channels.end(); it++)
	{
		if (it->has_client(client))
		{
			it->remove_client(client);
			if (it->get_user_count() == 0)
			{
				std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << it->getName() << std::endl;
				it = _channels.erase(it);
			}
		}
	}

	close(_pfds[client_index].fd);
	_pfds[client_index].fd = -1;
	_clients[client_index - 1] = Client();

	for (int j = client_index; j < _nfds - 1; j++)
	{
		_pfds[j] = _pfds[j + 1];
		_pfds[j + 1].fd = -1;
		if (j > 0 && j < MAX_CLIENTS)
		{
			_clients[j - 1] = _clients[j];
			// Re-bind the pfd pointer to the shifted pollfd slot
			_clients[j - 1].pfdp = &(_pfds[j]);
			_clients[j] = Client();
		}
	}

	_nfds--;
}

void Server::parse_message(std::string message, Client &sender)
{
	if (message.empty() || message.find_first_not_of(" \t\r\n") == std::string::npos)
	{
		return;
	}

	Message irc_message(message);

	if (!irc_message.getCommand().empty())
	{
		handle_command(irc_message, sender);
	}
}

void Server::handle_command(const Message &msg, Client &sender)
{
	// Log the command received
	std::string sender_nick = sender.nick.empty() ? "unregistered" : sender.nick;
	std::cout << "\033[35m[CMD]\033[0m " << sender_nick << " -> " << msg.getCommand();

	// Add parameters if any
	for (size_t i = 0; i < msg.getParamCount(); i++)
	{
		std::cout << " " << (msg.hasTrailing() && i == msg.getParamCount() - 1 ? ":" : "") << "[" << msg.getParams()[i] << "]";
	}

	std::cout << std::endl;

	_commands->execute_command(msg, sender);
}
Channel *Server::find_channel(const std::string &name)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (_channels[i].getName() == name)
			return &(_channels[i]);
	}
	return NULL;
}

Channel *Server::get_or_create_channel(const std::string &name)
{
	Channel *ch = find_channel(name);
	if (ch)
		return ch;
	_channels.push_back(Channel(name));
	// Show state when a new channel is created
	display_server_state();
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
	// Accept once per poll() call (subject requirement)
	Client client;
	client.addrlen = sizeof(client.addr);

	client.fd = accept(_sockfd, (struct sockaddr *)&client.addr, &client.addrlen);
	if (client.fd < 0)
	{
		// Accept failed (no errno check per subject requirement)
		// Will try again on next poll()
		_pfds[0].revents = 0;
		return;
	}

	// accepted new connection
	fcntl(client.fd, F_SETFL, O_NONBLOCK);

	// Enable TCP_NODELAY to reduce latency (disable Nagle)
	int nodelay = 1;
	setsockopt(client.fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

	// Fill hostname field for client (IPv4)
	char hostbuf[INET_ADDRSTRLEN];
	const char *hp = inet_ntop(AF_INET, &client.addr.sin_addr, hostbuf, sizeof(hostbuf));
	if (hp)
	{
		client.hostname = std::string(hostbuf);
	}

	// Get client port for logging
	int client_port = ntohs(client.addr.sin_port);
	std::cout << "\033[36m[CONNECT]\033[0m New client connected from " << client.hostname
			  << ":" << client_port << " (fd: " << client.fd << ")" << std::endl;

	if ((_nfds - 1) < MAX_CLIENTS)
	{
		size_t slot = find_empty_slot();
		if (slot > 0 && slot <= MAX_CLIENTS)
		{
			_pfds[slot].fd = client.fd;
			_pfds[slot].events = POLLIN;
			_pfds[slot].revents = 0;
			client.pfdp = &(_pfds[slot]);
			_clients[slot - 1] = client;
			_nfds++;
			// client assigned to slot
			// Show state when a new client is accepted
			display_server_state();
		}
		else
		{
			close(client.fd);
		}
	}
	else
	{
		// Server full - queue error message for sending
		std::string error_msg = "ERROR :Server full, try again later\r\n";
		client.outbuf = error_msg;
		// Add temporarily to send the error (will be closed after send)
		size_t slot = find_empty_slot();
		if (slot > 0 && slot <= MAX_CLIENTS)
		{
			_pfds[slot].fd = client.fd;
			_pfds[slot].events = POLLOUT; // Only write, will close after
			_pfds[slot].revents = 0;
			client.pfdp = &(_pfds[slot]);
			_clients[slot - 1] = client;
			_clients[slot - 1].nick = "FULL_SERVER_TEMP"; // Mark for cleanup
			_nfds++;
		}
		else
		{
			close(client.fd);
		}
	}

	_pfds[0].revents = 0;
}

void Server::display_server_state()
{
	int connected_clients = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (_clients[i].fd != -1)
		{
			connected_clients++;
		}
	}

	int active_channels = _channels.size();

	std::cout << "\033[34m[STATE]\033[0m Connected clients: " << connected_clients << std::endl;
	std::cout << "\033[34m[STATE]\033[0m Active channels: " << active_channels << std::endl;
}

void Server::queueSend(Client &client, const std::string &data)
{
	if (client.fd == -1)
		return;

	client.outbuf += data;

	// Enable POLLOUT to be notified when socket is writable
	if (client.pfdp)
	{
		client.pfdp->events |= POLLOUT;
	}
}