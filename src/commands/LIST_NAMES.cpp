#include "Commands.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include <unistd.h>

int Commands::cmd_list(const Message &msg, Client &sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    (void)msg;
    std::string line = ":ircserv 321 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " Channel :Users Name\r\n";
    sendToClient(sender, line);
    std::vector<Channel> &chans = _server->getChannels();
    for (size_t i = 0; i < chans.size(); ++i)
    {
        Channel &c = chans[i];
        std::string l2 = ":ircserv 322 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " " + c.getName() + " " + int_to_string(c.get_user_count()) + " :" + c.getTopic() + "\r\n";
        // send_reply(sender, RPL_LIST, );
        sendToClient(sender, l2);
    }
    send_reply(sender, 323, ":End of /LIST");
    return 0;
}

int Commands::cmd_names(const Message &msg, Client &sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    std::string channel_name;
    if (msg.getParamCount() >= 1)
        channel_name = msg.getParams()[0];
    if (!channel_name.empty())
    {
        Channel *ch = _server->find_channel(channel_name);
        if (!ch)
        {
            send_error(sender, ERR_NOSUCHCHANNEL, channel_name + " :No such channel");
            return -1;
        }
        std::string names = ch->build_names_list();
        std::string r353 = ":ircserv 353 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " = " + channel_name + " :" + names + "\r\n";
        sendToClient(sender, r353);
        send_reply(sender, 366, channel_name + " :End of /NAMES list");
    }
    else
    {
        send_reply(sender, 366, ":End of /NAMES list");
    }
    return 0;
}
