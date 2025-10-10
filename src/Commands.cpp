#include "Commands.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include <unistd.h>
#include <cctype>
#include <iostream>

// int_to_string moved to utils.cpp / utils.hpp

// void Commands::maybe_complete_registration(Client& sender)
// {
//     if (sender.isRegistered() && !sender.is_fully_registered) {
//         sender.is_fully_registered = true;
//         std::cout << "\033[32m[AUTH]\033[0m " << sender.nick << " registered successfully" << std::endl;
//         std::string prefix = sender.nick;
//         if (!sender.username.empty() && !sender.hostname.empty())
//             prefix += "!" + sender.username + "@" + sender.hostname;
//         send_reply(sender, 001, "Welcome to the Internet Relay Network " + prefix);
//         send_reply(sender, 002, "Your host is ircserv, running version 1.0");
//         send_reply(sender, 003, std::string("This server was created ") + getServerCreatedAt());
//         send_reply(sender, 004, "ircserv 1.0 o o");
//         send_reply(sender, 375, ":ircserv Message of the day");
//         send_reply(sender, 372, ":- Welcome to ft_irc");
//         send_reply(sender, 376, ":End of /MOTD command");
//     }
// }

void Commands::attempt_registration(Client& sender)
{
    // done TODO: attempt registration
    
    // done TODO: check password
    if (!sender.password_is_valid)
    {
        std::cout << "\033[31m[AUTH]\033[0m Connection refused (wrong password)" << std::endl;
        send_error(sender, 464, "Password incorrect");
        // shutdown(sender.fd, SHUT_WR);
        close(sender.fd); //TODO: mark fd for closure instead of closing it right away (so that the error is actually send it) (valid for other closes) and close them all at next loop iteration (MAYBE)
        return ;
    }

    if (!is_nick_valid(sender.nick)) {
		std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for NICK: \"" << sender.nick << "\"" << std::endl;
		send_error(sender, 432, sender.nick + " :Erroneous nickname");
        sender.nick_given = false;
		return ; //done TODO: maybe return something
	}

    Client* clients = _server->getClients();
	for (int i = 0; i < _server->getMaxClients(); i++) {
		if (clients[i].fd != -1 && clients[i].nick == sender.nick && clients[i].fd != sender.fd) {
			send_error(sender, 433, sender.nick + " :Nickname is already in use");
            sender.nick_given = false;
			return ; //done TODO: maybe return something
		}
	}

    sender.is_fully_registered = true;
    std::cout << "\033[32m[AUTH]\033[0m " << sender.nick << " registered successfully" << std::endl;
    std::string prefix = sender.nick;
    if (!sender.username.empty() && !sender.hostname.empty())
        prefix += "!" + sender.username + "@" + sender.hostname;
    send_reply(sender, 001, "Welcome to the Internet Relay Network " + prefix);
    send_reply(sender, 002, "Your host is ircserv, running version 1.0");
    send_reply(sender, 003, std::string("This server was created ") + getServerCreatedAt());
    send_reply(sender, 004, "ircserv 1.0 o o");
    send_reply(sender, 375, ":ircserv Message of the day");
    send_reply(sender, 372, ":- Welcome to ft_irc");
    send_reply(sender, 376, ":End of /MOTD command");

}

// Core wiring and utilities

Commands::Commands(Server* server) : _server(server) {}

Commands::~Commands() {}

const std::string& Commands::getServerCreatedAt() const { return _server->getCreatedAt(); }

int Commands::execute_command(const Message& msg, Client& sender)
{
    const std::string& command = msg.getCommand();
    // if (!sender.is_fully_registered) {
    //     if (command != "PASS" && command != "NICK" && command != "USER" &&
    //         command != "CAP" && command != "QUIT" && command != "PING") {
    //         send_error(sender, 451, ":You have not registered");
    //         return -1;
    //     }
    // }

    /*TODO reorganise

	if CAP LS
	set ongoing_negociation and reply CAP * LS
	until CAP END is received, then unset ongoing_negociation

	if !ongoing_negociation and received CAP END
		ignore
	
	if received CAP REQ :cap_name
	respond with CAP NAK :cap_name

	ready_for_registration: has received nick and user. and pass?

	if ready_for_negociation and !ongoing_negociation
		attempt registration

	registration attempt:
		- check if password is valid. if it isnt, send error 464 and close connection.
		- check if nickname is valid (no collision or invalid chars). if it isnt, send error 432 and wait for the client to send a new nickname, then register again.
	*/

    if (command == "CAP") return cmd_cap(msg, sender);
    if (command == "PASS") return cmd_pass(msg, sender);
    if (command == "NICK") return cmd_nick(msg, sender);
    if (command == "USER") return cmd_user(msg, sender);
    if (command == "QUIT") return cmd_quit(msg, sender);
    if (command == "PING") return cmd_ping(msg, sender);
    if (command == "PONG") return cmd_pong(msg, sender);

    if (!sender.is_fully_registered) return send_error(sender, 451, ":You are not registered"), -1;

    else if (command == "OPER") return cmd_oper(msg, sender);
    else if (command == "ERROR") return cmd_error(msg, sender);
    else if (command == "JOIN") return cmd_join(msg, sender);
    else if (command == "PART") return cmd_part(msg, sender);
    else if (command == "TOPIC") return cmd_topic(msg, sender);
    else if (command == "LIST") return cmd_list(msg, sender);
    else if (command == "NAMES") return cmd_names(msg, sender);
    else if (command == "INVITE") return cmd_invite(msg, sender);
    else if (command == "KICK") return cmd_kick(msg, sender);
    else if (command == "MODE") return cmd_mode(msg, sender);
    else if (command == "PRIVMSG") return cmd_privmsg(msg, sender);
    else if (command == "NOTICE") return cmd_notice(msg, sender);
    else if (command == "KILL") return cmd_kill(msg, sender);
    else if (command == "WHO") return cmd_who(msg, sender);
    else if (command == "WHOIS") return cmd_whois(msg, sender);

    std::cout << "\033[31m[ERROR]\033[0m Unknown command: \"" << command << "\"" << std::endl;
    send_error(sender, 421, "Unknown command");
    return -1;
}

bool Commands::is_nick_valid(const std::string& nick) const
{
    if (nick.empty() || nick.length() > 30) return false;
    if (!std::isalpha(nick[0]) && nick[0] != '[' && nick[0] != ']' && nick[0] != '\\' &&
        nick[0] != '`' && nick[0] != '^' && nick[0] != '{' && nick[0] != '}' && nick[0] != '|')
        return false;
    for (size_t i = 1; i < nick.length(); i++) {
        if (!std::isalnum(nick[i]) && nick[i] != '-' && nick[i] != '[' && nick[i] != ']' &&
            nick[i] != '\\' && nick[i] != '`' && nick[i] != '^' && nick[i] != '{' &&
            nick[i] != '}' && nick[i] != '|')
            return false;
    }
    return true;
}

bool Commands::is_channel_valid(const std::string& channel) const
{
    if (channel.empty()) return false;
    if (channel[0] != '#' && channel[0] != '&' && channel[0] != '+' && channel[0] != '!')
        return false;
    for (size_t i = 1; i < channel.length(); i++) {
        if (std::isspace(channel[i]) || channel[i] == ',' || channel[i] < 32)
            return false;
    }
    return true;
}

void Commands::sendToClient(Client& client, const std::string& data)
{
    _server->queueSend(client, data);
}

void Commands::send_error(Client& client, int error_code, const std::string& message) const
{
    std::string nick = client.nick.empty() ? "*" : client.nick;
    std::string error_msg = ":" + std::string("ircserv") + " " + int_to_string(error_code) + " " + nick + " " + message + "\r\n";
    const_cast<Commands*>(this)->sendToClient(client, error_msg);
}

void Commands::send_reply(Client& client, int reply_code, const std::string& message) const
{
    std::string nick = client.nick.empty() ? "*" : client.nick;
    std::string reply_msg = ":" + std::string("ircserv") + " " + int_to_string(reply_code) + " " + nick + " " + message + "\r\n";
    const_cast<Commands*>(this)->sendToClient(client, reply_msg);
}


