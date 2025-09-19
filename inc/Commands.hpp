#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>
#include "Client.hpp"
#include "Message.hpp"

class Server;

class Commands
{
private:
	Server* _server;

public:
	Commands(Server* server);
	~Commands();

	// Command execution
	int execute_command(const Message& msg, Client& sender);

	// Individual command handlers
	int cmd_cap(const Message& msg, Client& sender);
	int cmd_pass(const Message& msg, Client& sender);
	int cmd_nick(const Message& msg, Client& sender);
	int cmd_user(const Message& msg, Client& sender);
	int cmd_ping(const Message& msg, Client& sender);
	int cmd_pong(const Message& msg, Client& sender);
	int cmd_oper(const Message& msg, Client& sender);
	int cmd_quit(const Message& msg, Client& sender);
	int cmd_error(const Message& msg, Client& sender);
	int cmd_join(const Message& msg, Client& sender);
	int cmd_part(const Message& msg, Client& sender);
	int cmd_topic(const Message& msg, Client& sender);
	int cmd_list(const Message& msg, Client& sender);
	int cmd_names(const Message& msg, Client& sender);
	int cmd_invite(const Message& msg, Client& sender);
	int cmd_kick(const Message& msg, Client& sender);
	int cmd_mode(const Message& msg, Client& sender);
	int cmd_privmsg(const Message& msg, Client& sender);
	int cmd_notice(const Message& msg, Client& sender);
	int cmd_kill(const Message& msg, Client& sender);

	// Utility methods
	bool is_nick_valid(const std::string& nick) const;
	bool is_channel_valid(const std::string& channel) const;
	void send_error(Client& client, int error_code, const std::string& message) const;
	void send_reply(Client& client, int reply_code, const std::string& message) const;

    // Accessors needed by helpers
    const std::string& getServerCreatedAt() const;
};

#endif
