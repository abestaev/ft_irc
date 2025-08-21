#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <poll.h>
#include <netinet/in.h>
#include "Client.hpp"
#include "Message.hpp"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

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
	
	void accept_new_clients();
	size_t find_empty_slot();
	void parse_message(std::string, Client&);
	void handle_client_disconnect(int client_index);
	void process_client_messages();
	
	// New parsing methods
	Message parse_irc_message(const std::string& raw_message);
	void handle_command(const Message& msg, Client& sender);

public:
	Server(int port, std::string pass);
	~Server();

	void init();
	void run();
};

#endif
