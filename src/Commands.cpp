#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <iostream>

// Utility function for C++98 compatibility
std::string int_to_string(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

Commands::Commands(Server* server) : _server(server) {}
const std::string& Commands::getServerCreatedAt() const { return _server->getCreatedAt(); }

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
	else if (command == "WHO")
		return cmd_who(msg, sender);
	else if (command == "WHOIS")
		return cmd_whois(msg, sender);
	
	std::cout << "\033[31m[ERROR]\033[0m Unknown command: \"" << command << "\"" << std::endl;
	send_error(sender, 421, "Unknown command");
	return -1;
}

static void maybe_complete_registration(Client &sender, Commands *self)
{
	if (sender.isRegistered() && !sender.is_fully_registered) {
		sender.is_fully_registered = true;
		std::cout << "\033[32m[AUTH]\033[0m " << sender.nick << " registered successfully" << std::endl;
        // Minimal but friendlier registration burst for better client compatibility
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        self->send_reply(sender, 001, "Welcome to the Internet Relay Network " + prefix);
        self->send_reply(sender, 002, "Your host is ircserv, running version 1.0");
        {
            self->send_reply(sender, 003, std::string("This server was created ") + self->getServerCreatedAt());
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
		std::cout << "\033[31m[AUTH]\033[0m Connection refused (wrong password)" << std::endl;
		send_error(sender, 464, "Password incorrect");
		close(sender.fd);
		return -1;
	}
	return 0;
}

int Commands::cmd_nick(const Message& msg, Client& sender)
{
	
	if (msg.getParamCount() < 1) {
		std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for NICK: not enough parameters" << std::endl;
		send_error(sender, 461, "NICK :Not enough parameters");
		return -1;
	}
	
	std::string new_nick = msg.getParams()[0];
	
	// Check if nickname is valid
	if (!is_nick_valid(new_nick)) {
		std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for NICK: \"" << new_nick << "\"" << std::endl;
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
	
	// Broadcast change to channels
	std::string old_nick = sender.nick;
	if (!old_nick.empty() && old_nick != new_nick) {
		std::string prefix = old_nick;
		if (!sender.username.empty() && !sender.hostname.empty())
			prefix += "!" + sender.username + "@" + sender.hostname;
		std::string wire = ":" + prefix + " NICK :" + new_nick + "\r\n";
		std::vector<Channel>& chans = _server->getChannels();
		for (size_t i = 0; i < chans.size(); ++i) {
			if (chans[i].has_client(sender)) {
				chans[i].broadcast(wire, sender.fd);
			}
		}
	}
	
	// Set the nickname
	sender.setNick(new_nick);
	std::cout << "\033[32m[AUTH]\033[0m Client fd:" << sender.fd << " set nickname: \"" << new_nick << "\"" << std::endl;
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
	
	std::string username = msg.getParams()[0];
	std::string password = msg.getParams()[1];
	
	// Simple operator authentication (username: "admin", password: "admin123")
	if (username == "admin" && password == "admin123") {
		sender.is_operator = true;
		send_reply(sender, 381, ":You are now an IRC operator");
		
		// Broadcast to all channels that this user is now an operator
		std::string prefix = sender.nick;
		if (!sender.username.empty() && !sender.hostname.empty())
			prefix += "!" + sender.username + "@" + sender.hostname;
		std::string wire = ":" + prefix + " MODE " + sender.nick + " +o\r\n";
		
		std::vector<Channel>& chans = _server->getChannels();
		for (size_t i = 0; i < chans.size(); ++i) {
			if (chans[i].has_client(sender)) {
				chans[i].broadcast(wire, -1);
			}
		}
		return 0;
	}
	
	send_error(sender, 464, ":Password incorrect");
	return -1;
}

int Commands::cmd_quit(const Message& msg, Client& sender)
{
	std::string reason = msg.getTrailing();
	if (reason.empty()) {
		reason = "Client Quit";
	}
	
	// Broadcast QUIT to all channels
	std::string prefix = sender.nick;
	if (!sender.username.empty() && !sender.hostname.empty())
		prefix += "!" + sender.username + "@" + sender.hostname;
	std::string wire = ":" + prefix + " QUIT :" + reason + "\r\n";
	std::vector<Channel>& chans = _server->getChannels();
	for (size_t i = 0; i < chans.size(); ++i) {
		if (chans[i].has_client(sender)) {
			std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick << " left " << chans[i].getName() << std::endl;
			chans[i].broadcast(wire, sender.fd);
			chans[i].remove_client(sender);
			
			// Check if channel should be deleted (no more users)
			if (chans[i].get_user_count() == 0) {
				std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << chans[i].getName() << std::endl;
				chans.erase(chans.begin() + i);
				i--; // adjust index since we removed an element
			}
		}
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
        std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for JOIN: not enough parameters" << std::endl;
        send_error(sender, 461, "JOIN :Not enough parameters");
        return -1;
    }

    // Helpers to split comma-separated lists
    std::string channels_csv = msg.getParams()[0];
    std::vector<std::string> channels;
    {
        std::string current;
        for (size_t i = 0; i < channels_csv.size(); ++i) {
            if (channels_csv[i] == ',') { channels.push_back(current); current.clear(); }
            else { current += channels_csv[i]; }
        }
        channels.push_back(current);
    }

    std::vector<std::string> keys;
    if (msg.getParamCount() >= 2) {
        std::string keys_csv = msg.getParams()[1];
        std::string current;
        for (size_t i = 0; i < keys_csv.size(); ++i) {
            if (keys_csv[i] == ',') { keys.push_back(current); current.clear(); }
            else { current += keys_csv[i]; }
        }
        keys.push_back(current);
    }

    int success_count = 0;
    for (size_t idx = 0; idx < channels.size(); ++idx) {
        std::string channel_name = channels[idx];
        if (!is_channel_valid(channel_name)) {
            std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for JOIN: \"" << channel_name << "\"" << std::endl;
            send_error(sender, 403, channel_name + " :Invalid channel name");
            continue;
        }

        std::string provided_key;
        if (idx < keys.size()) provided_key = keys[idx];

        Channel* ch = _server->get_or_create_channel(channel_name);

        bool is_new_channel = (ch->get_user_count() == 0);

        if (ch->getUserLimit() > 0 && ch->get_user_count() >= ch->getUserLimit()) {
            send_error(sender, 471, channel_name + " :Cannot join channel (+l)");
            continue;
        }
        if (ch->isInviteOnly() && !ch->isNickInvited(sender.nick)) {
            send_error(sender, 473, channel_name + " :Cannot join channel (+i)");
            continue;
        }
        if (ch->hasKey() && ch->getKey() != provided_key) {
            send_error(sender, 475, channel_name + " :Cannot join channel (+k)");
            continue;
        }

        ch->add_client(sender);

        if (is_new_channel) {
            std::cout << "\033[33m[CHANNEL]\033[0m Created channel " << channel_name << " by " << sender.nick << std::endl;
            // Show server state when a new channel is created
            _server->display_server_state();
        } else {
            std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick << " joined " << channel_name << std::endl;
        }
        if (ch->isNickInvited(sender.nick)) ch->removeNickInvite(sender.nick);
        if (ch->isOperator(sender)) {
            std::string modeWire = ":ircserv MODE " + channel_name + " +o " + (sender.nick.empty() ? std::string("*") : sender.nick) + "\r\n";
            ch->broadcast(modeWire, -1);
        }

        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        std::string joinWire = ":" + prefix + " JOIN " + channel_name + "\r\n";
        ch->broadcast(joinWire, -1);

        const std::string& topic = ch->getTopic();
        if (topic.empty())
            send_reply(sender, 331, channel_name + " :No topic is set");
        else
            send_reply(sender, 332, channel_name + " :" + topic);

        std::string names = ch->build_names_list();
        std::string r353 = ":ircserv 353 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " = " + channel_name + " :" + names + "\r\n";
        write(sender.fd, r353.c_str(), r353.length());
        send_reply(sender, 366, channel_name + " :End of /NAMES list");

        success_count++;
    }

    return success_count > 0 ? 0 : -1;
}

int Commands::cmd_part(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "PART :Not enough parameters");
        return -1;
    }

    // Split channel list by comma
    std::string channels_csv = msg.getParams()[0];
    std::vector<std::string> channels;
    {
        std::string current;
        for (size_t i = 0; i < channels_csv.size(); ++i) {
            if (channels_csv[i] == ',') { channels.push_back(current); current.clear(); }
            else { current += channels_csv[i]; }
        }
        channels.push_back(current);
    }

    std::string reason = msg.getTrailing();
    int success_count = 0;
    for (size_t idx = 0; idx < channels.size(); ++idx) {
        std::string channel_name = channels[idx];
        Channel* ch = _server->find_channel(channel_name);
        if (!ch) {
            send_error(sender, 403, channel_name + " :No such channel");
            continue;
        }
        if (!ch->has_client(sender)) {
            send_error(sender, 442, channel_name + " :You're not on that channel");
            continue;
        }

        std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick << " left " << channel_name << std::endl;

        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        std::string wire = ":" + prefix + " PART " + channel_name;
        if (!reason.empty()) wire += " :" + reason;
        wire += "\r\n";
        ch->broadcast(wire, -1);
        ch->remove_client(sender);

        if (ch->get_user_count() == 0) {
            std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << channel_name << std::endl;
            std::vector<Channel>& channels_vec = _server->getChannels();
            for (size_t i = 0; i < channels_vec.size(); ++i) {
                if (channels_vec[i].getName() == channel_name) {
                    channels_vec.erase(channels_vec.begin() + i);
                    break;
                }
            }
            // Show server state when a channel is deleted
            _server->display_server_state();
        }

        success_count++;
    }

    return success_count > 0 ? 0 : -1;
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
        if (ch->isTopicRestricted() && !ch->isOperator(sender)) {
            send_error(sender, 482, channel_name + " :You're not channel operator");
            return -1;
        }
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
    // 321 header
    {
        std::string line = ":ircserv 321 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " Channel :Users Name\r\n";
        write(sender.fd, line.c_str(), line.length());
    }
    std::vector<Channel>& chans = _server->getChannels();
    for (size_t i = 0; i < chans.size(); ++i) {
        Channel &c = chans[i];
        std::string line = ":ircserv 322 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " " + c.getName() + " " + int_to_string(c.get_user_count()) + " :" + c.getTopic() + "\r\n";
        write(sender.fd, line.c_str(), line.length());
    }
    // 323 end
    send_reply(sender, 323, ":End of /LIST");
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
    std::string nick = msg.getParams()[0];
    std::string channel_name = msg.getParams()[1];
    Channel* ch = _server->find_channel(channel_name);
    if (!ch) { send_error(sender, 403, channel_name + " :No such channel"); return -1; }
    if (!ch->isOperator(sender)) { send_error(sender, 482, channel_name + " :You're not channel operator"); return -1; }
    Client* clients = _server->getClients();
    Client target;
    bool found = false;
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == nick) { target = clients[i]; found = true; break; }
    }
    if (!found) { send_error(sender, 401, nick + " :No such nick/channel"); return -1; }
    ch->inviteNick(nick);
    // 341 RPL_INVITING
    {
        std::string r341 = ":ircserv 341 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " " + nick + " " + channel_name + "\r\n";
        write(sender.fd, r341.c_str(), r341.length());
    }
    // Notify target
    {
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        // Proper RFC format: INVITE <nick> <channel>
        std::string wire = ":" + prefix + " INVITE " + nick + " " + channel_name + "\r\n";
        write(target.fd, wire.c_str(), wire.length());
    }
    return 0;
}

int Commands::cmd_kick(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 2) {
		send_error(sender, 461, "KICK :Not enough parameters");
		return -1;
	}
    std::string channel_name = msg.getParams()[0];
    std::string nick = msg.getParams()[1];
    Channel* ch = _server->find_channel(channel_name);
    if (!ch) { send_error(sender, 403, channel_name + " :No such channel"); return -1; }
    if (!ch->isOperator(sender)) { send_error(sender, 482, channel_name + " :You're not channel operator"); return -1; }
    Client* clients = _server->getClients();
    Client target;
    bool found = false;
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == nick) { target = clients[i]; found = true; break; }
    }
    if (!found || !ch->has_client(target)) { send_error(sender, 441, nick + " " + channel_name + " :They aren't on that channel"); return -1; }
    std::string reason = msg.getTrailing();
    std::string prefix = sender.nick;
    if (!sender.username.empty() && !sender.hostname.empty())
        prefix += "!" + sender.username + "@" + sender.hostname;
    std::string wire = ":" + prefix + " KICK " + channel_name + " " + nick;
    if (!reason.empty()) wire += " :" + reason;
    wire += "\r\n";
    ch->broadcast(wire, -1);
    ch->remove_client(target);
    return 0;
}

int Commands::cmd_mode(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "MODE :Not enough parameters");
		return -1;
	}
    std::string target = msg.getParams()[0];
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
        Channel* ch = _server->find_channel(target);
        if (!ch) { send_error(sender, 403, target + " :No such channel"); return -1; }

        // Query current modes
        if (msg.getParamCount() == 1) {
            std::string modes = "+";
            if (ch->isInviteOnly()) modes += "i";
            if (ch->isTopicRestricted()) modes += "t";
            if (ch->hasKey()) modes += "k";
            if (ch->getUserLimit() > 0) modes += "l";
            if (ch->isSecret()) modes += "s";
            if (ch->isNoExternalMessages()) modes += "n";
            if (ch->isChannelOperatorTopic()) modes += "T";
            std::string reply = target + " " + modes;
            if (ch->hasKey()) reply += " *";
            if (ch->getUserLimit() > 0) reply += " " + int_to_string(ch->getUserLimit());
            send_reply(sender, 324, reply);
            return 0;
        }

        // Modify modes: require operator
        if (!ch->isOperator(sender)) {
            send_error(sender, 482, target + " :You're not channel operator");
            return -1;
        }
        std::string modespec = msg.getParams()[1];
        int paramIndex = 2;
        bool adding = true;
        char lastSign = 0; // track for appliedSpec
        std::string appliedSpec;
        std::vector<std::string> appliedParams;
        for (size_t i = 0; i < modespec.size(); ++i) {
            char m = modespec[i];
            if (m == '+') { adding = true; continue; }
            if (m == '-') { adding = false; continue; }
            // append sign if changed or at start
            if ((adding ? '+' : '-') != lastSign) {
                appliedSpec += (adding ? '+' : '-');
                lastSign = (adding ? '+' : '-');
            }
            if (m == 'i') { ch->setInviteOnly(adding); appliedSpec += 'i'; continue; }
            if (m == 't') { ch->setTopicRestricted(adding); appliedSpec += 't'; continue; }
            if (m == 's') { ch->setSecret(adding); appliedSpec += 's'; continue; }
            if (m == 'n') { ch->setNoExternalMessages(adding); appliedSpec += 'n'; continue; }
            if (m == 'T') { ch->setChannelOperatorTopic(adding); appliedSpec += 'T'; continue; }
            if (m == 'k') {
                if (adding) {
                    if ((size_t)paramIndex > msg.getParamCount()-1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; }
                    std::string key = msg.getParams()[paramIndex++];
                    ch->setKey(key);
                    appliedSpec += 'k';
                    appliedParams.push_back(key);
                } else {
                    ch->clearKey();
                    appliedSpec += 'k';
                }
                continue;
            }
            if (m == 'l') {
                if (adding) {
                    if ((size_t)paramIndex > msg.getParamCount()-1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; }
                    int lim = std::atoi(msg.getParams()[paramIndex++].c_str());
                    if (lim < 0) lim = 0;
                    ch->setUserLimit(lim);
                    appliedSpec += 'l';
                    appliedParams.push_back(int_to_string(lim));
                } else {
                    ch->setUserLimit(0);
                    appliedSpec += 'l';
                }
                continue;
            }
            if (m == 'o') {
                if ((size_t)paramIndex > msg.getParamCount()-1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; }
                std::string nick = msg.getParams()[paramIndex++];
                Client* clients = _server->getClients();
                Client targetClient;
                bool found = false;
                for (int ci = 0; ci < _server->getMaxClients(); ci++) {
                    if (clients[ci].fd != -1 && clients[ci].nick == nick) { targetClient = clients[ci]; found = true; break; }
                }
                if (!found || !ch->has_client(targetClient)) { send_error(sender, 441, nick + " " + target + " :They aren't on that channel"); continue; }
                ch->setOperator(targetClient, adding);
                appliedSpec += 'o';
                appliedParams.push_back(nick);
                continue;
            }
            // Unknown mode
            send_error(sender, 472, std::string(1, m) + " :is unknown mode char to me");
        }
        // Broadcast applied mode changes if any
        if (!appliedSpec.empty()) {
            std::string prefix = sender.nick;
            if (!sender.username.empty() && !sender.hostname.empty())
                prefix += "!" + sender.username + "@" + sender.hostname;
            std::string wire = ":" + prefix + " MODE " + target + " " + appliedSpec;
            for (size_t pi = 0; pi < appliedParams.size(); ++pi) {
                wire += " " + appliedParams[pi];
            }
            wire += "\r\n";
            ch->broadcast(wire, -1);
        }
        return 0;
    }
    
    // User MODE: handle user mode changes
    if (msg.getParamCount() == 1) {
        // Query current user modes
        send_reply(sender, 221, sender.getUserModes());
        return 0;
    }
    
    // Modify user modes
    std::string modespec = msg.getParams()[1];
    bool adding = true;
    char lastSign = 0;
    std::string appliedSpec;
    
    for (size_t i = 0; i < modespec.size(); ++i) {
        char m = modespec[i];
        if (m == '+') { adding = true; continue; }
        if (m == '-') { adding = false; continue; }
        
        // append sign if changed or at start
        if ((adding ? '+' : '-') != lastSign) {
            appliedSpec += (adding ? '+' : '-');
            lastSign = (adding ? '+' : '-');
        }
        
        if (m == 'i') { 
            sender.is_invisible = adding; 
            appliedSpec += 'i'; 
            continue; 
        }
        if (m == 'w') { 
            sender.is_wallops = adding; 
            appliedSpec += 'w'; 
            continue; 
        }
        if (m == 'r') { 
            sender.is_restricted = adding; 
            appliedSpec += 'r'; 
            continue; 
        }
        if (m == 'o') { 
            // Only operators can set global operator mode
            if (sender.is_operator) {
                sender.is_global_operator = adding; 
                appliedSpec += 'o'; 
            }
            continue; 
        }
        if (m == 'O') { 
            // Only global operators can set local operator mode
            if (sender.is_global_operator) {
                sender.is_local_operator = adding; 
                appliedSpec += 'O'; 
            }
            continue; 
        }
        
        // Unknown mode
        send_error(sender, 501, std::string(1, m) + " :Unknown MODE flag");
    }
    
    // Broadcast applied mode changes if any
    if (!appliedSpec.empty()) {
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        std::string wire = ":" + prefix + " MODE " + sender.nick + " " + appliedSpec + "\r\n";
        
        // Send to all clients
        Client* clients = _server->getClients();
        for (int i = 0; i < _server->getMaxClients(); i++) {
            if (clients[i].fd != -1) {
                write(clients[i].fd, wire.c_str(), wire.length());
            }
        }
    }
    
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
	
	// Only server operators can use KILL
	if (!sender.is_operator) {
		send_error(sender, 481, ":Permission Denied- You're not an IRC operator");
		return -1;
	}
	
	std::string target_nick = msg.getParams()[0];
	std::string reason = msg.getTrailing();
	if (reason.empty()) {
		reason = "Killed by " + sender.nick;
	}
	
	// Find target client
	Client* clients = _server->getClients();
	Client* target = NULL;
	for (int i = 0; i < _server->getMaxClients(); i++) {
		if (clients[i].fd != -1 && clients[i].nick == target_nick) {
			target = &clients[i];
			break;
		}
	}
	
	if (!target) {
		send_error(sender, 401, target_nick + " :No such nick/channel");
		return -1;
	}
	
	// Broadcast KILL to all channels the target is in
	std::vector<Channel>& chans = _server->getChannels();
	for (size_t i = 0; i < chans.size(); ++i) {
		if (chans[i].has_client(*target)) {
			std::string prefix = sender.nick;
			if (!sender.username.empty() && !sender.hostname.empty())
				prefix += "!" + sender.username + "@" + sender.hostname;
			std::string wire = ":" + prefix + " KILL " + target_nick + " :" + reason + "\r\n";
			chans[i].broadcast(wire, -1);
			chans[i].remove_client(*target);
            // Delete channel if now empty
            if (chans[i].get_user_count() == 0) {
                std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << chans[i].getName() << std::endl;
                chans.erase(chans.begin() + i);
                i--;
                continue;
            }
		}
	}
	
	// Send KILL message to target and close connection
	std::string kill_msg = ":" + sender.nick + " KILL " + target_nick + " :" + reason + "\r\n";
	write(target->fd, kill_msg.c_str(), kill_msg.length());
	close(target->fd);
	target->fd = -1;
	
	return 0;
}

int Commands::cmd_who(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "WHO :Not enough parameters");
		return -1;
	}
	
	std::string target = msg.getParams()[0];
	
	// Check if target is a channel
	if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
		Channel* ch = _server->find_channel(target);
		if (!ch) {
			send_error(sender, 403, target + " :No such channel");
			return -1;
		}
		
		// Send WHO information for channel members
		Client* clients = _server->getClients();
		for (int i = 0; i < _server->getMaxClients(); i++) {
			if (clients[i].fd != -1 && ch->has_client(clients[i])) {
				std::string prefix = clients[i].nick;
				if (!clients[i].username.empty())
					prefix += "!" + clients[i].username;
				prefix += "@localhost";
				
				std::string who_line = ":localhost 352 " + sender.nick + " " + target + " " + 
					clients[i].username + " localhost ircserv " + clients[i].nick + " H";
				if (ch->isOperator(clients[i]))
					who_line += "@";
				who_line += " :0 " + clients[i].realname + "\r\n";
				write(sender.fd, who_line.c_str(), who_line.length());
			}
		}
		send_reply(sender, 315, target + " :End of /WHO list");
	} else {
		// WHO for a specific user
		Client* clients = _server->getClients();
		for (int i = 0; i < _server->getMaxClients(); i++) {
			if (clients[i].fd != -1 && clients[i].nick == target) {
				std::string who_line = ":localhost 352 " + sender.nick + " * " + 
					clients[i].username + " localhost ircserv " + clients[i].nick + " H :0 " + 
					clients[i].realname + "\r\n";
				write(sender.fd, who_line.c_str(), who_line.length());
				break;
			}
		}
		send_reply(sender, 315, target + " :End of /WHO list");
	}
	return 0;
}

int Commands::cmd_whois(const Message& msg, Client& sender)
{
	if (msg.getParamCount() < 1) {
		send_error(sender, 461, "WHOIS :Not enough parameters");
		return -1;
	}
	
	std::string target_nick = msg.getParams()[0];
	Client* clients = _server->getClients();
	Client* target = NULL;
	
	// Find target client
	for (int i = 0; i < _server->getMaxClients(); i++) {
		if (clients[i].fd != -1 && clients[i].nick == target_nick) {
			target = &clients[i];
			break;
		}
	}
	
	if (!target) {
		send_error(sender, 401, target_nick + " :No such nick/channel");
		return -1;
	}
	
	// Send WHOIS information
	std::string whois_line = ":localhost 311 " + sender.nick + " " + target_nick + " ~" + 
		target->username + " localhost * :" + target->realname + "\r\n";
	write(sender.fd, whois_line.c_str(), whois_line.length());
	
	// Send channels the user is in
	std::string channels;
	std::vector<Channel>& chans = _server->getChannels();
	for (size_t i = 0; i < chans.size(); ++i) {
		if (chans[i].has_client(*target)) {
			if (!channels.empty()) channels += " ";
			if (chans[i].isOperator(*target)) channels += "@";
			channels += chans[i].getName();
		}
	}
	if (!channels.empty()) {
		std::string chan_line = ":localhost 319 " + sender.nick + " " + target_nick + " :" + channels + "\r\n";
		write(sender.fd, chan_line.c_str(), chan_line.length());
	}
	
	send_reply(sender, 318, target_nick + " :End of /WHOIS list");
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
