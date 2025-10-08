#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_oper(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 2) {
        send_error(sender, 461, "OPER :Not enough parameters");
        return -1;
    }

    std::string username = msg.getParams()[0];
    std::string password = msg.getParams()[1];

    if (username == "admin" && password == "admin123") {
        sender.is_operator = true;
        send_reply(sender, 381, ":You are now an IRC operator");

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


