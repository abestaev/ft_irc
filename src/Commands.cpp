#include "Commands.hpp"
#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <sstream>
#include <ctime>

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
	// Gate non-registered clients: allow only limited commands before full registration
	if (!sender.is_fully_registered) {
		if (command != "PASS" && command != "NICK" && command != "USER" &&
			command != "CAP" && command != "QUIT" && command != "PING") {
			send_error(sender, 451, ":You have not registered");
			return -1;
		}
	}
	
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
	else if (command == "NOTICE")
		return cmd_notice(msg, sender);
	else if (command == "KILL")
		return cmd_kill(msg, sender);
	
	send_error(sender, 421, "Unknown command");
	return -1;
}

static void maybe_complete_registration(Client &sender, Commands *self)
{
	if (sender.isRegistered() && !sender.is_fully_registered) {
		sender.is_fully_registered = true;
        // Minimal but friendlier registration burst for better client compatibility
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        self->send_reply(sender, 001, "Welcome to the Internet Relay Network " + prefix);
        self->send_reply(sender, 002, "Your host is ircserv, running version 1.0");
        {
            char timebuf[64];
            std::time_t now = std::time(NULL);
            std::tm *ptm = std::localtime(&now);
            if (ptm) {
                std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S %Z", ptm);
                self->send_reply(sender, 003, std::string("This server was created ") + timebuf);
            } else {
                self->send_reply(sender, 003, std::string("This server was created epoch=") + int_to_string((int)now));
            }
        }
        self->send_reply(sender, 004, "ircserv 1.0 o o");
        // Minimal MOTD
        self->send_reply(sender, 375, ":ircserv Message of the day");
        self->send_reply(sender, 372, ":- Welcome to ft_irc");
        self->send_reply(sender, 376, ":End of /MOTD command");
	}
}

int Commands::cmd_cap(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "CAP :Not enough parameters");
		return -1;
	}
	
    if (msg.getParams()[0] == "LS") {
        std::string response = "CAP * LS :\r\n"; // no capabilities advertised
        write(sender.fd, response.c_str(), response.length());
    } else if (msg.getParams()[0] == "LIST") {
        std::string response = "CAP * LIST :\r\n";
        write(sender.fd, response.c_str(), response.length());
    } else if (msg.getParams()[0] == "REQ") {
        // NAK all requested capabilities for now
        std::string requested = msg.hasTrailing() ? msg.getTrailing() : (msg.getParamCount() >= 2 ? msg.getParams()[1] : "");
        std::string response = "CAP * NAK :" + requested + "\r\n";
        write(sender.fd, response.c_str(), response.length());
    } else if (msg.getParams()[0] == "END") {
        // Nothing to do; registration completes when PASS/NICK/USER done
    }
	return 0;
}

int Commands::cmd_pass(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "PASS :Not enough parameters");
		return -1;
	}
	
	std::string password = msg.getParams()[0];
	
	if (password == _server->getPassword()) {
		sender.password_is_valid = true;
		maybe_complete_registration(sender, this);
	} else {
		send_error(sender, 464, "Password incorrect");
		close(sender.fd);
		return -1;
	}
	return 0;
}

int Commands::cmd_nick(const Message& msg, Client& sender)
{
	
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "NICK :Not enough parameters");
		return -1;
	}
	
	std::string new_nick = msg.getParams()[0];
	
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
	maybe_complete_registration(sender, this);
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
	maybe_complete_registration(sender, this);
	return 0;
}

int Commands::cmd_ping(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "PING :Not enough parameters");
		return -1;
	}
	
	std::string token = msg.getParams()[0];
	std::string response = "PONG :" + token + "\r\n";
	write(sender.fd, response.c_str(), response.length());
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
	
	std::string response = "ERROR :Closing Link: " + reason + "\r\n";
	write(sender.fd, response.c_str(), response.length());
	close(sender.fd);
	return -1;
}

int Commands::cmd_error(const Message& msg, Client& sender)
{
	(void)sender;
	// ERROR command from client, usually indicates a problem
	std::string reason = msg.getTrailing();
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
	
	Channel* ch = _server->get_or_create_channel(channel_name);
    ch->add_client(sender);
    // Broadcast JOIN
    std::string prefix = sender.nick;
    if (!sender.username.empty() && !sender.hostname.empty())
        prefix += "!" + sender.username + "@" + sender.hostname;
    std::string joinWire = ":" + prefix + " JOIN " + channel_name + "\r\n";
    ch->broadcast(joinWire, -1);
    // Topic (332/331)
    const std::string& topic = ch->getTopic();
    if (topic.empty())
        send_reply(sender, 331, channel_name + " :No topic is set");
    else
        send_reply(sender, 332, channel_name + " :" + topic);
    // NAMES (353/366)
    std::string names = ch->build_names_list();
    std::string r353 = ":ircserv 353 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " = " + channel_name + " :" + names + "\r\n";
    write(sender.fd, r353.c_str(), r353.length());
    send_reply(sender, 366, channel_name + " :End of /NAMES list");
	return 0;
}

int Commands::cmd_part(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "PART :Not enough parameters");
		return -1;
	}
	std::string channel_name = msg.getParams()[0];
	Channel* ch = _server->find_channel(channel_name);
	if (!ch) {
		send_error(sender, 403, channel_name + " :No such channel");
		return -1;
	}
    if (!ch->has_client(sender)) {
        send_error(sender, 442, channel_name + " :You're not on that channel");
        return -1;
    }
    // Broadcast PART
    std::string reason = msg.getTrailing();
    std::string prefix = sender.nick;
    if (!sender.username.empty() && !sender.hostname.empty())
        prefix += "!" + sender.username + "@" + sender.hostname;
    std::string wire = ":" + prefix + " PART " + channel_name;
    if (!reason.empty()) wire += " :" + reason;
    wire += "\r\n";
    ch->broadcast(wire, -1);
    ch->remove_client(sender);
	return 0;
}

int Commands::cmd_topic(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "TOPIC :Not enough parameters");
		return -1;
	}
	std::string channel_name = msg.getParams()[0];
	Channel* ch = _server->find_channel(channel_name);
	if (!ch) {
		send_error(sender, 403, channel_name + " :No such channel");
		return -1;
	}
	if (msg.hasTrailing()) {
		std::string new_topic = msg.getTrailing();
		ch->setTopic(new_topic);
		std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " TOPIC " + channel_name + " :" + new_topic + "\r\n";
		ch->broadcast(wire, -1);
		send_reply(sender, 332, channel_name + " :" + new_topic);
		return 0;
	}
	const std::string& topic = ch->getTopic();
	if (topic.empty()) {
		send_reply(sender, 331, channel_name + " :No topic is set");
	} else {
		send_reply(sender, 332, channel_name + " :" + topic);
	}
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
	std::string channel_name;
	if (msg.getParamCount() >= 1) channel_name = msg.getParams()[0];
	if (!channel_name.empty()) {
		Channel* ch = _server->find_channel(channel_name);
		if (!ch) {
			send_error(sender, 403, channel_name + " :No such channel");
			return -1;
		}
		std::string names = ch->build_names_list();
		// Minimal numeric replies: 353 (names) then 366 (end)
		std::string r353 = ":ircserv 353 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " = " + channel_name + " :" + names + "\r\n";
		write(sender.fd, r353.c_str(), r353.length());
		send_reply(sender, 366, channel_name + " :End of /NAMES list");
	} else {
		send_reply(sender, 366, ":End of /NAMES list");
	}
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
	
	std::string target = msg.getParams()[0];
	std::string text = msg.getTrailing();
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
		Channel* ch = _server->find_channel(target);
		if (!ch) {
			send_error(sender, 401, target + " :No such nick/channel");
			return -1;
		}
		std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " PRIVMSG " + target + " :" + text + "\r\n";
		ch->broadcast(wire, sender.fd);
		// Also echo back to sender to confirm send
		write(sender.fd, wire.c_str(), wire.length());
		return 0;
	}
    // Deliver to a user nick
    Client* clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == target) {
            std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick);
            if (!sender.username.empty() && !sender.hostname.empty())
                wire += "!" + sender.username + "@" + sender.hostname;
            wire += " PRIVMSG " + target + " :" + text + "\r\n";
            write(clients[i].fd, wire.c_str(), wire.length());
            return 0;
        }
    }
    send_error(sender, 401, target + " :No such nick/channel");
    return -1;
}

int Commands::cmd_notice(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        return -1; // NOTICE ne renvoie pas d'erreurs
    }
    if (msg.getTrailing().empty()) {
        return -1; // pas d'erreurs pour NOTICE
    }
    std::string target = msg.getParams()[0];
    std::string text = msg.getTrailing();
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
        Channel* ch = _server->find_channel(target);
        if (!ch) {
            return -1; // NOTICE: silencieux
        }
        std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " NOTICE " + target + " :" + text + "\r\n";
        ch->broadcast(wire, sender.fd);
        return 0;
    }
    Client* clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == target) {
            std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick);
            if (!sender.username.empty() && !sender.hostname.empty())
                wire += "!" + sender.username + "@" + sender.hostname;
            wire += " NOTICE " + target + " :" + text + "\r\n";
            write(clients[i].fd, wire.c_str(), wire.length());
            return 0;
        }
    }
    return 0; // silencieux
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
    // Allow longer nicks for modern clients like irssi
    if (nick.empty() || nick.length() > 30)
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
	std::string nick = client.nick.empty() ? "*" : client.nick;
	std::string error_msg = ":" + std::string("ircserv") + " " + 
		int_to_string(error_code) + " " + nick + " " + message + "\r\n";
	write(client.fd, error_msg.c_str(), error_msg.length());
}

void Commands::send_reply(Client& client, int reply_code, const std::string& message) const
{
	std::string nick = client.nick.empty() ? "*" : client.nick;
	std::string reply_msg = ":" + std::string("ircserv") + " " + 
		int_to_string(reply_code) + " " + nick + " " + message + "\r\n";
	write(client.fd, reply_msg.c_str(), reply_msg.length());
}
