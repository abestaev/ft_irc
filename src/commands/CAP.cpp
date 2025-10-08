#include "Commands.hpp"
#include <unistd.h>

int Commands::cmd_cap(const Message& msg, Client& sender)
{
    if (msg.getParamCount() < 1) {
        send_error(sender, 461, "CAP :Not enough parameters");
        return -1;
    }

    if (msg.getParams()[0] == "LS") {
        std::string response = "CAP * LS :\r\n";
        write(sender.fd, response.c_str(), response.length());
    } else if (msg.getParams()[0] == "LIST") {
        std::string response = "CAP * LIST :\r\n";
        write(sender.fd, response.c_str(), response.length());
    } else if (msg.getParams()[0] == "REQ") {
        std::string requested = msg.hasTrailing() ? msg.getTrailing() : (msg.getParamCount() >= 2 ? msg.getParams()[1] : "");
        std::string response = "CAP * NAK :" + requested + "\r\n";
        write(sender.fd, response.c_str(), response.length());
    } else if (msg.getParams()[0] == "END") {
        // nothing to do
    }
    return 0;
}


