#include "Commands.hpp"
#include "Server.hpp"
#include "utils.hpp"

int Commands::cmd_mode(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; }
    std::string target = msg.getParams()[0];
    if (!target.empty() && (target[0] == '#' || target[0] == '&' || target[0] == '!' || target[0] == '+')) {
        Channel* ch = _server->find_channel(target);
        if (!ch) { send_error(sender, 403, target + " :No such channel"); return -1; }
        if (msg.getParamCount() == 1) {
            std::string modes = "+";
            if (ch->isInviteOnly()) modes += "i";
            if (ch->isTopicRestricted()) modes += "t";
            if (ch->hasKey()) modes += "k";
            if (ch->getUserLimit() > 0) modes += "l";
            if (ch->isSecret()) modes += "s";
            if (ch->isNoExternalMessages()) modes += "n";
            if (ch->isChannelOperatorTopic()) modes += "T";
            std::string reply = target + " " + modes;
            if (ch->hasKey()) reply += " *";
            if (ch->getUserLimit() > 0) reply += " " + int_to_string(ch->getUserLimit());
            send_reply(sender, 324, reply);
            return 0;
        }
        if (!ch->isOperator(sender)) { send_error(sender, 482, target + " :You're not channel operator"); return -1; }
        std::string modespec = msg.getParams()[1];
        int paramIndex = 2; bool adding = true; char lastSign = 0; std::string appliedSpec; std::vector<std::string> appliedParams;
        for (size_t i = 0; i < modespec.size(); ++i) {
            char m = modespec[i]; if (m == '+') { adding = true; continue; } if (m == '-') { adding = false; continue; }
            if ((adding ? '+' : '-') != lastSign) { appliedSpec += (adding ? '+' : '-'); lastSign = (adding ? '+' : '-'); }
            if (m == 'i') { ch->setInviteOnly(adding); appliedSpec += 'i'; continue; }
            if (m == 't') { ch->setTopicRestricted(adding); appliedSpec += 't'; continue; }
            if (m == 's') { ch->setSecret(adding); appliedSpec += 's'; continue; }
            if (m == 'n') { ch->setNoExternalMessages(adding); appliedSpec += 'n'; continue; }
            if (m == 'T') { ch->setChannelOperatorTopic(adding); appliedSpec += 'T'; continue; }
            if (m == 'k') { if (adding) { if ((size_t)paramIndex > msg.getParamCount()-1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; } std::string key = msg.getParams()[paramIndex++]; ch->setKey(key); appliedSpec += 'k'; appliedParams.push_back(key);} else { ch->clearKey(); appliedSpec += 'k'; } continue; }
            if (m == 'l') { if (adding) { if ((size_t)paramIndex > msg.getParamCount()-1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; } int lim = std::atoi(msg.getParams()[paramIndex++].c_str()); if (lim < 0) lim = 0; ch->setUserLimit(lim); appliedSpec += 'l'; appliedParams.push_back(int_to_string(lim)); } else { ch->setUserLimit(0); appliedSpec += 'l'; } continue; }
            if (m == 'o') { if ((size_t)paramIndex > msg.getParamCount()-1) { send_error(sender, 461, "MODE :Not enough parameters"); return -1; } std::string nick = msg.getParams()[paramIndex++]; Client* clients = _server->getClients(); Client targetClient; bool found = false; for (int ci = 0; ci < _server->getMaxClients(); ci++) { if (clients[ci].fd != -1 && clients[ci].nick == nick) { targetClient = clients[ci]; found = true; break; } } if (!found || !ch->has_client(targetClient)) { send_error(sender, 441, nick + " " + target + " :They aren't on that channel"); continue; } ch->setOperator(targetClient, adding); appliedSpec += 'o'; appliedParams.push_back(nick); continue; }
            send_error(sender, 472, std::string(1, m) + " :is unknown mode char to me");
        }
        if (!appliedSpec.empty()) {
            std::string prefix = sender.nick; if (!sender.username.empty() && !sender.hostname.empty()) prefix += "!" + sender.username + "@" + sender.hostname;
            std::string wire = ":" + prefix + " MODE " + target + " " + appliedSpec; for (size_t pi = 0; pi < appliedParams.size(); ++pi) wire += " " + appliedParams[pi]; wire += "\r\n"; ch->broadcast(wire, -1);
        }
        return 0;
    }
    if (msg.getParamCount() == 1) { send_reply(sender, 221, sender.getUserModes()); return 0; }
    std::string modespec = msg.getParams()[1]; bool adding = true; char lastSign = 0; std::string appliedSpec;
    for (size_t i = 0; i < modespec.size(); ++i) {
        char m = modespec[i]; if (m == '+') { adding = true; continue; } if (m == '-') { adding = false; continue; }
        if ((adding ? '+' : '-') != lastSign) { appliedSpec += (adding ? '+' : '-'); lastSign = (adding ? '+' : '-'); }
        if (m == 'i') { sender.is_invisible = adding; appliedSpec += 'i'; continue; }
        if (m == 'w') { sender.is_wallops = adding; appliedSpec += 'w'; continue; }
        if (m == 'r') { sender.is_restricted = adding; appliedSpec += 'r'; continue; }
        if (m == 'o') { if (sender.is_operator) { sender.is_global_operator = adding; appliedSpec += 'o'; } continue; }
        if (m == 'O') { if (sender.is_global_operator) { sender.is_local_operator = adding; appliedSpec += 'O'; } continue; }
        send_error(sender, 501, std::string(1, m) + " :Unknown MODE flag");
    }
    if (!appliedSpec.empty()) {
        std::string prefix = sender.nick; if (!sender.username.empty() && !sender.hostname.empty()) prefix += "!" + sender.username + "@" + sender.hostname;
        std::string wire = ":" + prefix + " MODE " + sender.nick + " " + appliedSpec + "\r\n";
        Client* clients = _server->getClients(); for (int i = 0; i < _server->getMaxClients(); i++) { if (clients[i].fd != -1) { write(clients[i].fd, wire.c_str(), wire.length()); } }
    }
    return 0;
}


