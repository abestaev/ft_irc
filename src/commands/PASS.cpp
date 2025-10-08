#include "Commands.hpp"
#include <unistd.h>
#include <iostream>

int Commands::cmd_pass(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "PASS :Not enough parameters");
        return -1;
    }

    std::string password = msg.getParams()[0];

    if (password == _server->getPassword()) {
        sender.password_is_valid = true;
        maybe_complete_registration(sender);
    } else {
        std::cout << "\033[31m[AUTH]\033[0m Connection refused (wrong password)" << std::endl;
        send_error(sender, 464, "Password incorrect");
        close(sender.fd);
        return -1;
    }
    return 0;
}


