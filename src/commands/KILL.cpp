#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_kill(const Message& msg, Client& sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "KILL :Not enough parameters");
        return -1;
    }
    if (!sender.is_operator) {
        send_error(sender, 481, ":Permission Denied- You're not an IRC operator");
        return -1;
    }
    std::string target_nick = msg.getParams()[0];
    std::string reason = msg.getParamCount() >= 2 ? msg.getParams()[1] : "";
    if (reason.empty())
        reason = "Killed by " + sender.nick;
    Client* clients = _server->getClients();
    Client* target = NULL; for (int i = 0; i < _server->getMaxClients(); i++) { if (clients[i].fd != -1 && clients[i].nick == target_nick) { target = &clients[i]; break; } }
    if (!target) { send_error(sender, 401, target_nick + " :No such nick/channel"); return -1; }
    std::vector<Channel>& chans = _server->getChannels();
    for (size_t i = 0; i < chans.size(); ++i) { if (chans[i].has_client(*target)) { std::string prefix = sender.nick; if (!sender.username.empty() && !sender.hostname.empty()) prefix += "!" + sender.username + "@" + sender.hostname; std::string wire = ":" + prefix + " KILL " + target_nick + " :" + reason + "\r\n"; chans[i].broadcastQueued(wire, -1, _server); chans[i].remove_client(*target); if (chans[i].get_user_count() == 0) { std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << chans[i].getName() << std::endl; chans.erase(chans.begin() + i); i--; } } }
    std::string kill_msg = ":" + sender.nick + " KILL " + target_nick + " :" + reason + "\r\n"; sendToClient(*target, kill_msg); close(target->fd); target->fd = -1; return 0;
}


