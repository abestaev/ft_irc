#include "Commands.hpp"

int Commands::cmd_user(const Message& msg, Client& sender)
{
    if (sender.is_fully_registered)
    {
        send_error(sender, 462, "USER :This command can only be used at registration");
        return -1;
    }

    if (msg.getParamCount() < 3) {
        send_error(sender, 461, "USER :Not enough parameters");
        return -1;
    }

    std::string username = msg.getParams()[0];
    std::string realname = msg.getTrailing();

    if (realname.empty() && msg.getParamCount() >= 4) {
        realname = msg.getParams()[3];
    }

    sender.setUser(username, realname);
    if (sender.isReadyForRegistration())
        attempt_registration(sender);
    // maybe_complete_registration(sender);
    return 0;
}


