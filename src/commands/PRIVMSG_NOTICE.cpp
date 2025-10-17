#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>

int Commands::cmd_privmsg(const Message &msg, Client &sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    if (msg.getParamCount() < 1)
        return send_error(sender, ERR_NORECIPIENT, "No recipient given (PRIVMSG)"), -1;
    if (msg.getParamCount() < 2)
        return send_error(sender, ERR_NOTEXTTOSEND, "No text to send"), -1;
    std::string target = msg.getParams()[0];
    std::string text = msg.getParams()[1];
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+'))
    {
        Channel *ch = _server->find_channel(target);
        if (!ch)
        {
            send_error(sender, ERR_NOSUCHCHANNEL, target + " :No such channel");
            return -1;
        }
        std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " PRIVMSG " + target + " :" + text + "\r\n";
        ch->broadcast(wire, sender.fd);
        return 0;
    }
    Client *clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++)
    {
        if (clients[i].fd != -1 && clients[i].nick == target)
        {
            std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick);
            if (!sender.username.empty() && !sender.hostname.empty())
                wire += "!" + sender.username + "@" + sender.hostname;
            wire += " PRIVMSG " + target + " :" + text + "\r\n";
            sendToClient(clients[i], wire);
            return 0;
        }
    }
    send_error(sender, ERR_NOSUCHNICK, target + " :No such nick");
    return -1;
}

int Commands::cmd_notice(const Message &msg, Client &sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    if (msg.getParamCount() < 1)
    {
        return -1;
    }
    if (msg.getParamCount() < 2)
    {
        return -1;
    }
    std::string target = msg.getParams()[0];
    std::string text = msg.getParams()[1];
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+'))
    {
        Channel *ch = _server->find_channel(target);
        if (!ch)
        {
            return -1;
        }
        std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " NOTICE " + target + " :" + text + "\r\n";
        ch->broadcast(wire, sender.fd);
        return 0;
    }
    Client *clients = _server->getClients();
    for (int i = 0; i < _server->getMaxClients(); i++)
    {
        if (clients[i].fd != -1 && clients[i].nick == target)
        {
            std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick);
            if (!sender.username.empty() && !sender.hostname.empty())
                wire += "!" + sender.username + "@" + sender.hostname;
            wire += " NOTICE " + target + " :" + text + "\r\n";
            sendToClient(clients[i], wire);
            return 0;
        }
    }
    return 0;
}
