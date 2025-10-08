#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>

int Commands::cmd_privmsg(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) { send_error(sender, 411, "No recipient given (PRIVMSG)"); return -1; }
    if (msg.getTrailing().empty()) { send_error(sender, 412, "No text to send"); return -1; }
    std::string target = msg.getParams()[0]; std::string text = msg.getTrailing();
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
        Channel* ch = _server->find_channel(target);
        if (!ch) { send_error(sender, 401, target + " :No such nick/channel"); return -1; }
        std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " PRIVMSG " + target + " :" + text + "\r\n";
        ch->broadcast(wire, sender.fd);
        return 0;
    }
    Client* clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == target) {
            std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick);
            if (!sender.username.empty() && !sender.hostname.empty()) wire += "!" + sender.username + "@" + sender.hostname;
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
    if (msg.getParamCount() < 1) { return -1; }
    if (msg.getTrailing().empty()) { return -1; }
    std::string target = msg.getParams()[0]; std::string text = msg.getTrailing();
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
        Channel* ch = _server->find_channel(target); if (!ch) { return -1; }
        std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " NOTICE " + target + " :" + text + "\r\n";
        ch->broadcast(wire, sender.fd); return 0;
    }
    Client* clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++) {
        if (clients[i].fd != -1 && clients[i].nick == target) {
            std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick);
            if (!sender.username.empty() && !sender.hostname.empty()) wire += "!" + sender.username + "@" + sender.hostname;
            wire += " NOTICE " + target + " :" + text + "\r\n";
            write(clients[i].fd, wire.c_str(), wire.length());
            return 0;
        }
    }
    return 0;
}


