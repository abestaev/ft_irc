#include "Commands.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_pass(const Message& msg, Client& sender)
{

    if (sender.is_fully_registered)
    {
        send_error(sender, 462, "PASS :This command can only be used at registration");
        return -1;
    }

    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "PASS :Not enough parameters");
        return -1;
    }

    std::string password = msg.getParams()[0];

    sender.password_is_valid = password == _server->getPassword();
    return 0;
}


