#include "Commands.hpp"
#include <unistd.h>

int Commands::cmd_ping(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "PING :Not enough parameters");
        return -1;
    }
    std::string token = msg.getParams()[0];
    std::string response = ":ircserv PONG ircserv :" + token + "\r\n";
    sendToClient(sender, response);
    return 0;
}

int Commands::cmd_pong(const Message& msg, Client& sender)
{
    (void)msg;
    (void)sender;
    return 0;
}


