#include "Commands.hpp"
#include "Server.hpp"

int Commands::cmd_topic(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "TOPIC :Not enough parameters");
        return -1;
    }
    std::string channel_name = msg.getParams()[0];
    Channel* ch = _server->find_channel(channel_name);
    if (!ch) { send_error(sender, 403, channel_name + " :No such channel"); return -1; }
    if (msg.hasTrailing()) {
        std::string new_topic = msg.getTrailing();
        if (ch->isTopicRestricted() && !ch->isOperator(sender)) { send_error(sender, 482, channel_name + " :You're not channel operator"); return -1; }
        ch->setTopic(new_topic);
        std::string wire = ":" + (sender.nick.empty() ? std::string("*") : sender.nick) + " TOPIC " + channel_name + " :" + new_topic + "\r\n";
        ch->broadcast(wire, -1);
        send_reply(sender, 332, channel_name + " :" + new_topic);
        return 0;
    }
    const std::string& topic = ch->getTopic();
    if (topic.empty()) send_reply(sender, 331, channel_name + " :No topic is set");
    else send_reply(sender, 332, channel_name + " :" + topic);
    return 0;
}


