#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_quit(const Message& msg, Client& sender)
{
    std::string reason = msg.getTrailing();
    if (reason.empty()) {
        reason = "Client Quit";
    }

    std::string prefix = sender.nick;
    if (!sender.username.empty() && !sender.hostname.empty())
        prefix += "!" + sender.username + "@" + sender.hostname;
    std::string wire = ":" + prefix + " QUIT :" + reason + "\r\n";

    std::vector<Channel>& chans = _server->getChannels();
    for (size_t i = 0; i < chans.size(); ++i) {
        if (chans[i].has_client(sender)) {
            std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick << " left " << chans[i].getName() << std::endl;
            chans[i].broadcastQueued(wire, sender.fd, _server);
            chans[i].remove_client(sender);
            if (chans[i].get_user_count() == 0) {
                std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << chans[i].getName() << std::endl;
                chans.erase(chans.begin() + i);
                i--;
            }
        }
    }

    std::string response = "ERROR :Closing Link: " + reason + "\r\n";
    sendToClient(sender, response);
    close(sender.fd);
    return -1;
}


