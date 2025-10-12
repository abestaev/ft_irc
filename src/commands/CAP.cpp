#include "Commands.hpp"
#include <unistd.h>

int Commands::cmd_cap(const Message& msg, Client& sender)
{
    if (sender.is_fully_registered)
    {
        send_error(sender, 462, "CAP :This command can only be used at registration");
        return -1;
    }

    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "CAP :Not enough parameters");
        return -1;
    }

    if (msg.getParams()[0] == "LS") {
        sender.ongoing_negociation = true;
        std::string response = "CAP * LS :\r\n";
        sendToClient(sender, response);
    } else if (msg.getParams()[0] == "LIST") {
        std::string response = "CAP * LIST :\r\n";
        sendToClient(sender, response);
    } else if (msg.getParams()[0] == "REQ") {
        std::string requested = msg.getParamCount() >= 2 ? msg.getParams()[1] : "";
        std::string response = "CAP * NAK :" + requested + "\r\n";
        sendToClient(sender, response);
    } else if (msg.getParams()[0] == "END") {
        sender.ongoing_negociation = false;
        if (sender.isReadyForRegistration())
            attempt_registration(sender);
        // nothing to do
    }
    return 0;
}


