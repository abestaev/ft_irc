#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include "Client.hpp"
#include <vector>

#include "config.hpp"

class Channel
{
private:
	Client _owner;
	Client _clients[MAX_CLIENTS];
    bool _is_operator[MAX_CLIENTS];
	std::string _name;
	std::string _topic;
	std::string _password;
	int _user_limit;
	bool _invite_only;
	bool _topic_restricted;
	bool _moderated;
	bool _secret;
	bool _no_external_messages;
	bool _channel_operator_topic;
    std::vector<std::string> _invited_nicks;

public:
	Channel(const std::string& name);
	~Channel();
	
	void add_client(const Client& client);
	void remove_client(const Client& client);
	bool has_client(const Client& client) const;
	void broadcast(const std::string& message, int exclude_fd) const;
	std::string build_names_list() const;
    int get_user_count() const;
    int find_client_index(const Client& client) const;
	
	// Getters
	const std::string& getName() const { return _name; }
	const std::string& getTopic() const { return _topic; }
	
	// Setters
	void setTopic(const std::string& topic) { _topic = topic; }

    // Modes and accessors
    void setInviteOnly(bool enabled) { _invite_only = enabled; }
    bool isInviteOnly() const { return _invite_only; }
    void setTopicRestricted(bool enabled) { _topic_restricted = enabled; }
    bool isTopicRestricted() const { return _topic_restricted; }
    void setKey(const std::string& key) { _password = key; }
    void clearKey() { _password.clear(); }
    bool hasKey() const { return !_password.empty(); }
    const std::string& getKey() const { return _password; }
    void setUserLimit(int limit) { _user_limit = limit; }
    int getUserLimit() const { return _user_limit; }
    bool isOperator(const Client& client) const;
    void setOperator(const Client& client, bool enabled);
    void inviteNick(const std::string& nick);
    bool isNickInvited(const std::string& nick) const;
    void removeNickInvite(const std::string& nick);
    
    // Additional channel modes
    void setSecret(bool enabled) { _secret = enabled; }
    bool isSecret() const { return _secret; }
    void setNoExternalMessages(bool enabled) { _no_external_messages = enabled; }
    bool isNoExternalMessages() const { return _no_external_messages; }
    void setChannelOperatorTopic(bool enabled) { _channel_operator_topic = enabled; }
    bool isChannelOperatorTopic() const { return _channel_operator_topic; }
};

#endif
