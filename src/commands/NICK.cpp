#include "Commands.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include <iostream>

int Commands::cmd_nick(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for NICK: not enough parameters" << std::endl;
        send_error(sender, 461, "NICK :Not enough parameters");
        return -1;
    }

    std::string new_nick = msg.getParams()[0];

    if (!is_nick_valid(new_nick)) {
        std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for NICK: \"" << new_nick << "\"" << std::endl;
        send_error(sender, 432, new_nick + " :Erroneous nickname");
        return -1;
    }

    Client* clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == new_nick && clients[i].fd != sender.fd) {
            send_error(sender, 433, new_nick + " :Nickname is already in use");
            return -1;
        }
    }

    std::string old_nick = sender.nick;
    if (!old_nick.empty() && old_nick != new_nick) {
        std::string prefix = old_nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        std::string wire = ":" + prefix + " NICK :" + new_nick + "\r\n";
        std::vector<Channel>& chans = _server->getChannels();
        for (size_t i = 0; i < chans.size(); ++i) {
            if (chans[i].has_client(sender)) {
                chans[i].broadcastQueued(wire, sender.fd, _server);
            }
        }
    }

    sender.setNick(new_nick);
    std::cout << "\033[32m[AUTH]\033[0m Client fd:" << sender.fd << " set nickname: \"" << new_nick << "\"" << std::endl;
    maybe_complete_registration(sender);
    return 0;
}


