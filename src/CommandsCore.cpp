#include "Commands.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include <iostream>

// int_to_string moved to utils.cpp / utils.hpp

void Commands::maybe_complete_registration(Client& sender)
{
    if (sender.isRegistered() && !sender.is_fully_registered) {
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
}


