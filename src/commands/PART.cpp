#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_part(const Message& msg, Client& sender)
{
    if (!sender.is_fully_registered) {
        send_error(sender, 451, ":You have not registered");
        return -1;
    }
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "PART :Not enough parameters");
        return -1;
    }
    std::string channels_csv = msg.getParams()[0];
    std::vector<std::string> channels;
    {
        std::string current;
        for (size_t i = 0; i < channels_csv.size(); ++i) {
            if (channels_csv[i] == ',') { channels.push_back(current); current.clear(); }
            else { current += channels_csv[i]; }
        }
        channels.push_back(current);
    }
    std::string reason = msg.getParamCount() >= 2 ? msg.getParams()[1] : "";
    int success_count = 0;
    for (size_t idx = 0; idx < channels.size(); ++idx) {
        std::string channel_name = channels[idx];
        Channel* ch = _server->find_channel(channel_name);
        if (!ch) { send_error(sender, 403, channel_name + " :No such channel"); continue; }
        if (!ch->has_client(sender)) { send_error(sender, 442, channel_name + " :You're not on that channel"); continue; }
        std::cout << "\033[33m[CHANNEL]\033[0m " << sender.nick << " left " << channel_name << std::endl;
        std::string prefix = sender.nick;
        if (!sender.username.empty() && !sender.hostname.empty()) prefix += "!" + sender.username + "@" + sender.hostname;
        std::string wire = ":" + prefix + " PART " + channel_name;
        if (!reason.empty()) wire += " :" + reason;
        wire += "\r\n";
        ch->broadcast(wire, -1);
        ch->remove_client(sender);
        if (ch->get_user_count() == 0) {
            std::cout << "\033[33m[CHANNEL]\033[0m Deleted channel " << channel_name << std::endl;
            std::vector<Channel>& channels_vec = _server->getChannels();
            for (size_t i = 0; i < channels_vec.size(); ++i) {
                if (channels_vec[i].getName() == channel_name) { channels_vec.erase(channels_vec.begin() + i); break; }
            }
            _server->display_server_state();
        }
        success_count++;
    }
    return success_count > 0 ? 0 : -1;
}


