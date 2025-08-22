#include "Commands.hpp"
#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <sstream>

// Utility function for C++98 compatibility
std::string int_to_string(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

Commands::Commands(Server* server) : _server(server) {}

Commands::~Commands() {}

int Commands::execute_command(const Message& msg, Client& sender)
{
	const std::string& command = msg.getCommand();
	
	std::cout << "Executing command: " << command << " from client " << sender.fd << std::endl;
	
	if (command == "CAP")
		return cmd_cap(msg, sender);
	else if (command == "PASS")
		return cmd_pass(msg, sender);
	else if (command == "NICK")
		return cmd_nick(msg, sender);
	else if (command == "USER")
		return cmd_user(msg, sender);
	else if (command == "PING")
		return cmd_ping(msg, sender);
	else if (command == "PONG")
		return cmd_pong(msg, sender);
	else if (command == "OPER")
		return cmd_oper(msg, sender);
	else if (command == "QUIT")
		return cmd_quit(msg, sender);
	else if (command == "ERROR")
		return cmd_error(msg, sender);
	else if (command == "JOIN")
		return cmd_join(msg, sender);
	else if (command == "PART")
		return cmd_part(msg, sender);
	else if (command == "TOPIC")
		return cmd_topic(msg, sender);
	else if (command == "LIST")
		return cmd_list(msg, sender);
	else if (command == "NAMES")
		return cmd_names(msg, sender);
	else if (command == "INVITE")
		return cmd_invite(msg, sender);
	else if (command == "KICK")
		return cmd_kick(msg, sender);
	else if (command == "MODE")
		return cmd_mode(msg, sender);
	else if (command == "PRIVMSG")
		return cmd_privmsg(msg, sender);
	else if (command == "KILL")
		return cmd_kill(msg, sender);
	
	// Unknown command
	std::cout << "Unknown command: " << command << std::endl;
	send_error(sender, 421, "Unknown command");
	return -1;
}

int Commands::cmd_cap(const Message& msg, Client& sender)
{
	std::cout << "CAP command received with " << msg.getParamCount() << " parameters" << std::endl;
	
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "CAP :Not enough parameters");
		return -1;
	}
	
	if (msg.getParams()[0] == "LS") {
		std::cout << "Sending CAP * LS response" << std::endl;
		write(sender.fd, "CAP * LS\r\n", 11);
	}
	// Server will not support capability negotiation, so ignore other CAP messages
	return 0;
}

int Commands::cmd_pass(const Message& msg, Client& sender)
{
	std::cout << "PASS command received with " << msg.getParamCount() << " parameters" << std::endl;
	
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "PASS :Not enough parameters");
		return -1;
	}
	
	std::string password = msg.getParams()[0];
	std::cout << "Password received: '" << password << "' vs expected: '" << _server->getPassword() << "'" << std::endl;
	
	if (password == _server->getPassword()) {
		sender.password_is_valid = true;
		std::cout << "Password accepted for client " << sender.fd << std::endl;
		send_reply(sender, 001, "Password accepted");
	} else {
		std::cout << "Password incorrect for client " << sender.fd << std::endl;
		send_error(sender, 464, "Password incorrect");
		close(sender.fd);
		return -1;
	}
	return 0;
}

int Commands::cmd_nick(const Message& msg, Client& sender)
{
	std::cout << "NICK command received with " << msg.getParamCount() << " parameters" << std::endl;
	
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "NICK :Not enough parameters");
		return -1;
	}
	
	std::string new_nick = msg.getParams()[0];
	std::cout << "Setting nickname to: " << new_nick << " for client " << sender.fd << std::endl;
	
	// Check if nickname is valid
	if (!is_nick_valid(new_nick)) {
		send_error(sender, 432, new_nick + " :Erroneous nickname");
		return -1;
	}
	
	// Check if nickname is already in use
	Client* clients = _server->getClients();
	for (int i = 0; i < _server->getMaxClients(); i++) {
		if (clients[i].fd != -1 && clients[i].nick == new_nick && clients[i].fd != sender.fd) {
			send_error(sender, 433, new_nick + " :Nickname is already in use");
			return -1;
		}
	}
	
	// Set the nickname
	sender.setNick(new_nick);
	std::cout << "Nickname set successfully to: " << new_nick << std::endl;
	send_reply(sender, 001, "Welcome to the Internet Relay Network");
	return 0;
}

int Commands::cmd_user(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 3) {
		send_error(sender, 461, "USER :Not enough parameters");
		return -1;
	}
	
	std::string username = msg.getParams()[0];
	std::string realname = msg.getTrailing();
	
	if (realname.empty() && msg.getParamCount() >= 4) {
		realname = msg.getParams()[3];
	}
	
	sender.setUser(username, realname);
	send_reply(sender, 001, "Welcome to the Internet Relay Network");
	return 0;
}

int Commands::cmd_ping(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "PING :Not enough parameters");
		return -1;
	}
	
	std::string token = msg.getParams()[0];
	write(sender.fd, ("PONG :" + token + "\r\n").c_str(), token.length() + 8);
	return 0;
}

int Commands::cmd_pong(const Message& msg, Client& sender)
{
	(void)msg;
	(void)sender;
	// PONG is usually just acknowledged, no specific action needed
	return 0;
}

int Commands::cmd_oper(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 2) {
		send_error(sender, 461, "OPER :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement operator authentication
	send_error(sender, 491, "No O: lines for your host");
	return -1;
}

int Commands::cmd_quit(const Message& msg, Client& sender)
{
	std::string reason = msg.getTrailing();
	if (reason.empty()) {
		reason = "Client Quit";
	}
	
	write(sender.fd, ("ERROR :Closing Link: " + reason + "\r\n").c_str(), reason.length() + 25);
	close(sender.fd);
	return -1;
}

int Commands::cmd_error(const Message& msg, Client& sender)
{
	(void)sender;
	// ERROR command from client, usually indicates a problem
	std::string reason = msg.getTrailing();
	std::cout << "Client error: " << reason << std::endl;
	return 0;
}

int Commands::cmd_join(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "JOIN :Not enough parameters");
		return -1;
	}
	
	std::string channel_name = msg.getParams()[0];
	if (!is_channel_valid(channel_name)) {
		send_error(sender, 403, channel_name + " :Invalid channel name");
		return -1;
	}
	
	// TODO: Implement channel joining logic
	send_reply(sender, 332, channel_name + " :No topic is set");
	return 0;
}

int Commands::cmd_part(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "PART :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement channel leaving logic
	return 0;
}

int Commands::cmd_topic(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "TOPIC :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement topic setting logic
	return 0;
}

int Commands::cmd_list(const Message& msg, Client& sender)
{
	(void)msg;
	// TODO: Implement channel listing
	send_reply(sender, 322, "End of /LIST");
	return 0;
}

int Commands::cmd_names(const Message& msg, Client& sender)
{
	(void)msg;
	// TODO: Implement names listing
	send_reply(sender, 366, "End of /NAMES list");
	return 0;
}

int Commands::cmd_invite(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 2) {
		send_error(sender, 461, "INVITE :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement invite logic
	return 0;
}

int Commands::cmd_kick(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 2) {
		send_error(sender, 461, "KICK :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement kick logic
	return 0;
}

int Commands::cmd_mode(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "MODE :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement mode logic
	return 0;
}

int Commands::cmd_privmsg(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 411, "No recipient given (PRIVMSG)");
		return -1;
	}
	
	if (msg.getTrailing().empty()) {
		send_error(sender, 412, "No text to send");
		return -1;
	}
	
	// TODO: Implement private messaging logic
	return 0;
}

int Commands::cmd_kill(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "KILL :Not enough parameters");
		return -1;
	}
	
	// TODO: Implement kill logic (operator only)
	return 0;
}

// Utility methods
bool Commands::is_nick_valid(const std::string& nick) const
{
	if (nick.empty() || nick.length() > 9)
		return false;
	
	// Nickname must start with a letter or special character
	if (!isalpha(nick[0]) && nick[0] != '[' && nick[0] != ']' && nick[0] != '\\' && 
		nick[0] != '`' && nick[0] != '^' && nick[0] != '{' && nick[0] != '}' && nick[0] != '|')
		return false;
	
	// Check for invalid characters
	for (size_t i = 1; i < nick.length(); i++) {
		if (!isalnum(nick[i]) && nick[i] != '-' && nick[i] != '[' && nick[i] != ']' && 
			nick[i] != '\\' && nick[i] != '`' && nick[i] != '^' && nick[i] != '{' && 
			nick[i] != '}' && nick[i] != '|')
			return false;
	}
	
	return true;
}

bool Commands::is_channel_valid(const std::string& channel) const
{
	if (channel.empty())
		return false;
	
	// Channel names must start with #, &, +, or !
	if (channel[0] != '#' && channel[0] != '&' && channel[0] != '+' && channel[0] != '!')
		return false;
	
	// Channel names cannot contain spaces, control characters, or commas
	for (size_t i = 1; i < channel.length(); i++) {
		if (isspace(channel[i]) || channel[i] == ',' || channel[i] < 32)
			return false;
	}
	
	return true;
}

void Commands::send_error(Client& client, int error_code, const std::string& message) const
{
	std::string error_msg = ":" + std::string("ircserv") + " " + 
		int_to_string(error_code) + " " + client.nick + " " + message + "\r\n";
	write(client.fd, error_msg.c_str(), error_msg.length());
}

void Commands::send_reply(Client& client, int reply_code, const std::string& message) const
{
	std::string reply_msg = ":" + std::string("ircserv") + " " + 
		int_to_string(reply_code) + " " + client.nick + " " + message + "\r\n";
	write(client.fd, reply_msg.c_str(), reply_msg.length());
}
