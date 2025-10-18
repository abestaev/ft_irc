#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_join(const Message &msg, Client &sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    if (msg.getParamCount() < 1)
    {
        std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for JOIN: not enough parameters" << std::endl;
        send_error(sender, 461, "JOIN :Not enough parameters");
        return -1;
    }
    std::string channels_csv = msg.getParams()[0];
    std::vector<std::string> channels;
    {
        std::string current;
        for (size_t i = 0; i < channels_csv.size(); ++i)
        {
            if (channels_csv[i] == ',')
            {
                channels.push_back(current);
                current.clear();
            }
            else
            {
                current += channels_csv[i];
            }
        }
        channels.push_back(current);
    }
    std::vector<std::string> keys;
    if (msg.getParamCount() >= 2)
    {
        std::string keys_csv = msg.getParams()[1];
        std::string current;
        for (size_t i = 0; i < keys_csv.size(); ++i)
        {
            if (keys_csv[i] == ',')
            {
                keys.push_back(current);
                current.clear();
            }
            else
            {
                current += keys_csv[i];
            }
        }
        keys.push_back(current);
    }
    int success_count = 0;
    for (size_t idx = 0; idx < channels.size(); ++idx)
    {
        std::string channel_name = channels[idx];
        if (!is_channel_valid(channel_name))
        {
            std::cout << "\033[31m[ERROR]\033[0m Invalid parameters for JOIN: \"" << channel_name << "\"" << std::endl;
            send_error(sender, 403, channel_name + " :Invalid channel name");
            continue;
        }
        std::string provided_key;
        if (idx < keys.size())
            provided_key = keys[idx];
        Channel *ch = _server->get_or_create_channel(channel_name);
        bool is_new_channel = (ch->get_user_count() == 0);
        if (ch->getUserLimit() > 0 && ch->get_user_count() >= ch->getUserLimit())
        {
            send_error(sender, 471, channel_name + " :Cannot join channel (+l)");
            continue;
        }
        if (ch->isInviteOnly() && !ch->isNickInvited(sender.nick))
        {
            send_error(sender, 473, channel_name + " :Cannot join channel (+i)");
            continue;
        }
        if (ch->hasKey() && ch->getKey() != provided_key)
        {
            send_error(sender, 475, channel_name + " :Cannot join channel (+k)");
            continue;
        }
        ch->add_client(sender);
        if (is_new_channel)
        {
            std::cout << "\033[33m[CHANNEL]\033[0m Created channel " << channel_name << " by " << sender.nick << std::endl;
            _server->display_server_state();
        }
        else
        {
            std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick << " joined " << channel_name << std::endl;
        }
        if (ch->isNickInvited(sender.nick))
            ch->removeNickInvite(sender.nick);
        if (ch->isOperator(sender))
        {
            std::string modeWire = ":ircserv MODE " + channel_name + " +o " + (sender.nick.empty() ? std::string("*") : sender.nick) + "\r\n";
            ch->broadcast(modeWire, -1);
        }
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;
        std::string joinWire = ":" + prefix + " JOIN " + channel_name + "\r\n";
        ch->broadcast(joinWire, -1);
        const std::string &topic = ch->getTopic();
        if (topic.empty())
            send_reply(sender, 331, channel_name + " :No topic is set");
        else
            send_reply(sender, 332, channel_name + " :" + topic);
        std::string names = ch->build_names_list();
        std::string r353 = ":ircserv 353 " + (sender.nick.empty() ? std::string("*") : sender.nick) + " = " + channel_name + " :" + names + "\r\n";
        sendToClient(sender, r353);
        send_reply(sender, 366, channel_name + " :End of /NAMES list");
        success_count++;
    }
    return success_count > 0 ? 0 : -1;
}
