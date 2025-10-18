#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_part(const Message& msg, Client& sender)
{
    // Check if user is registered
    if (!sender.is_fully_registered)
    {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    // Check for required parameters
    if (msg.getParamCount() < 1)
    {
        send_error(sender, 461, "PART :Not enough parameters");
        return -1;
    }
    // Parse comma-separated channel list
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
                current += channels_csv[i];
        }
        channels.push_back(current);
    }

    // Get optional part reason
    std::string reason = msg.getParamCount() >= 2 ? msg.getParams()[1] : "";

    // Process each channel
    int success_count = 0;
    for (size_t idx = 0; idx < channels.size(); ++idx)
    {
        std::string channel_name = channels[idx];
        Channel* ch = _server->find_channel(channel_name);

        // Check if channel exists
        if (!ch)
        {
            send_error(sender, 403, channel_name + " :No such channel");
            continue;
        }

        // Check if user is on the channel
        if (!ch->has_client(sender))
        {
            send_error(sender, 442, channel_name + " :You're not on that channel");
            continue;
        }

        // Log the PART
        std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick 
                  << " left " << channel_name << std::endl;

        // Build the PART message
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty())
            prefix += "!" + sender.username + "@" + sender.hostname;

        std::string wire = ":" + prefix + " PART " + channel_name;
        if (!reason.empty())
            wire += " :" + reason;
        wire += "\r\n";

        // Broadcast to all channel members
        ch->broadcast(wire, -1);

        // Remove client from channel
        ch->remove_client(sender);

        // Delete channel if empty
        if (ch->get_user_count() == 0)
        {
            std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " 
                      << channel_name << std::endl;

            std::vector<Channel>& channels_vec = _server->getChannels();
            for (size_t i = 0; i < channels_vec.size(); ++i)
            {
                if (channels_vec[i].getName() == channel_name)
                {
                    channels_vec.erase(channels_vec.begin() + i);
                    break;
                }
            }

            _server->display_server_state();
        }

        success_count++;
    }

    return success_count > 0 ? 0 : -1;
}


