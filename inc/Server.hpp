#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <poll.h>
#include <netinet/in.h>
#include "Client.hpp"
#include "Message.hpp"
#include "Commands.hpp"
#include "Channel.hpp"
#include <vector>

#include "config.hpp"

class Server
{
private:
	int _port;
	std::string _pass;
	int _sockfd;
	static bool _sig;
	struct sockaddr_in _addr;
	Client _clients[MAX_CLIENTS];
	int _nfds;
	struct pollfd _pfds[MAX_CLIENTS + 1];
	Commands* _commands;
	std::vector<Channel> _channels;
	
	void accept_new_clients();
	size_t find_empty_slot();
	void parse_message(std::string, Client&);
	void handle_client_disconnect(int client_index);
	void process_client_messages();
	
	// New parsing methods
	Message parse_irc_message(const std::string& raw_message);
	void handle_command(const Message& msg, Client& sender);
	
	// Signal handling
	static void signal_handler(int sig);
	void setup_signal_handlers();

public:
	Server(int port, std::string pass);
	~Server();

	void init();
	void run();
	
	// Getters for Commands class
	const std::string& getPassword() const { return _pass; }
	Client* getClients() { return _clients; }
	int getMaxClients() const { return MAX_CLIENTS; }

	// Channel management
	Channel* find_channel(const std::string& name);
	Channel* get_or_create_channel(const std::string& name);
	
	// Signal handling
	static bool should_stop() { return _sig; }
};

#endif
