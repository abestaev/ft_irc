#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include "Client.hpp"

#define MAX_CLIENTS 10

class Channel
{
private:
	Client _owner;
	Client _clients[MAX_CLIENTS];
	std::string _name;
	std::string _topic;
	std::string _password;
	int _user_limit;
	bool _invite_only;
	bool _topic_restricted;
	bool _moderated;

public:
	Channel(const std::string& name);
	~Channel();
	
	void add_client(const Client& client);
	void remove_client(const Client& client);
	bool has_client(const Client& client) const;
	
	// Getters
	const std::string& getName() const { return _name; }
	const std::string& getTopic() const { return _topic; }
	
	// Setters
	void setTopic(const std::string& topic) { _topic = topic; }
};

#endif
