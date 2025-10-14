#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>

int Commands::cmd_invite(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 2)
    {
        send_error(sender, 461, "INVITE :Not enough parameters");
        return -1;
    }

    const std::string nick = msg.getParams()[0];
    const std::string channel_name = msg.getParams()[1];

    Channel* ch = _server->find_channel(channel_name);
    if (!ch)
    {
        send_error(sender, 403, channel_name + " :No such channel");
        return -1;
    }
    if (!ch->isOperator(sender))
    {
        send_error(sender, 482, channel_name + " :You're not channel operator");
        return -1;
    }

    // Find target client by nick
    Client* clients = _server->getClients();
    Client target;
    bool found = false;
    for (int i = 0; i < _server->getMaxClients(); i++)
    {
        if (clients[i].fd != -1 && clients[i].nick == nick)
        {
            target = clients[i];
            found = true;
            break;
        }
    }
    if (!found)
    {
        send_error(sender, 401, nick + " :No such nick/channel");
        return -1;
    }

    // Mark invite and acknowledge to inviter (341)
    ch->inviteNick(nick);
    {
        const std::string inviter = sender.nick.empty() ? std::string("*") : sender.nick;
        const std::string r341 = ":ircserv 341 " + inviter + " " + nick + " " + channel_name + "\r\n";
        sendToClient(sender, r341);
    }

    // Notify target with proper prefix
    {
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        const std::string wire = ":" + prefix + " INVITE " + nick + " " + channel_name + "\r\n";
        sendToClient(target, wire);
    }
    return 0;
}

int Commands::cmd_kick(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 2)
    {
        send_error(sender, 461, "KICK :Not enough parameters");
        return -1;
    }

    const std::string channel_name = msg.getParams()[0];
    const std::string nick = msg.getParams()[1];

    Channel* ch = _server->find_channel(channel_name);
    if (!ch)
    {
        send_error(sender, 403, channel_name + " :No such channel");
        return -1;
    }
    // Server operators can kick anywhere, channel operators only in their channel
    if (!sender.is_operator && !ch->isOperator(sender))
    {
        send_error(sender, 482, channel_name + " :You're not channel operator");
        return -1;
    }

    // Find target client by nick
    Client* clients = _server->getClients();
    Client target;
    bool found = false;
    for (int i = 0; i < _server->getMaxClients(); i++)
    {
        if (clients[i].fd != -1 && clients[i].nick == nick)
        {
            target = clients[i];
            found = true;
            break;
        }
    }
    if (!found || !ch->has_client(target))
    {
        send_error(sender, 441, nick + " " + channel_name + " :They aren't on that channel");
        return -1;
    }

    // Build and broadcast KICK
    const std::string reason = msg.getTrailing();
    std::string prefix = sender.nick;
    if (!sender.username.empty() && !sender.hostname.empty())
        prefix += "!" + sender.username + "@" + sender.hostname;

    std::string wire = ":" + prefix + " KICK " + channel_name + " " + nick;
    if (!reason.empty())
        wire += " :" + reason;
    wire += "\r\n";

    ch->broadcast(wire, -1);
    ch->remove_client(target);
    return 0;
}


