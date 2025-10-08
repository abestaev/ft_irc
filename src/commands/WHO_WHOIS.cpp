#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>

int Commands::cmd_who(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) { send_error(sender, 461, "WHO :Not enough parameters"); return -1; }
    std::string target = msg.getParams()[0];
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
        Channel* ch = _server->find_channel(target);
        if (!ch) { send_error(sender, 403, target + " :No such channel"); return -1; }
        Client* clients = _server->getClients();
        for (int i = 0; i < _server->getMaxClients(); i++) {
            if (clients[i].fd != -1 && ch->has_client(clients[i])) {
                std::string prefix = clients[i].nick; if (!clients[i].username.empty()) prefix += "!" + clients[i].username; prefix += "@localhost";
                std::string who_line = ":localhost 352 " + sender.nick + " " + target + " " + clients[i].username + " localhost ircserv " + clients[i].nick + " H"; if (ch->isOperator(clients[i])) who_line += "@"; who_line += " :0 " + clients[i].realname + "\r\n"; write(sender.fd, who_line.c_str(), who_line.length());
            }
        }
        send_reply(sender, 315, target + " :End of /WHO list");
    } else {
        Client* clients = _server->getClients();
        for (int i = 0; i < _server->getMaxClients(); i++) {
            if (clients[i].fd != -1 && clients[i].nick == target) {
                std::string who_line = ":localhost 352 " + sender.nick + " * " + clients[i].username + " localhost ircserv " + clients[i].nick + " H :0 " + clients[i].realname + "\r\n"; write(sender.fd, who_line.c_str(), who_line.length()); break;
            }
        }
        send_reply(sender, 315, target + " :End of /WHO list");
    }
    return 0;
}

int Commands::cmd_whois(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) { send_error(sender, 461, "WHOIS :Not enough parameters"); return -1; }
    std::string target_nick = msg.getParams()[0]; Client* clients = _server->getClients(); Client* target = NULL;
    for (int i = 0; i < _server->getMaxClients(); i++) { if (clients[i].fd != -1 && clients[i].nick == target_nick) { target = &clients[i]; break; } }
    if (!target) { send_error(sender, 401, target_nick + " :No such nick/channel"); return -1; }
    std::string whois_line = ":localhost 311 " + sender.nick + " " + target_nick + " ~" + target->username + " localhost * :" + target->realname + "\r\n"; write(sender.fd, whois_line.c_str(), whois_line.length());
    std::string channels; std::vector<Channel>& chans = _server->getChannels(); for (size_t i = 0; i < chans.size(); ++i) { if (chans[i].has_client(*target)) { if (!channels.empty()) channels += " "; if (chans[i].isOperator(*target)) channels += "@"; channels += chans[i].getName(); } }
    if (!channels.empty()) { std::string chan_line = ":localhost 319 " + sender.nick + " " + target_nick + " :" + channels + "\r\n"; write(sender.fd, chan_line.c_str(), chan_line.length()); }
    send_reply(sender, 318, target_nick + " :End of /WHOIS list"); return 0;
}


