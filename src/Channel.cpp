#include "Channel.hpp"
#include <unistd.h>

Channel::Channel(const std::string& name): 
    _name(name), 
    _topic(""), 
    _password(""), 
    _user_limit(0), 
    _invite_only(false), 
    _topic_restricted(false), 
    _moderated(false),
    _secret(false),
    _no_external_messages(false),
    _channel_operator_topic(false)
{
	// Initialize client arrays
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		_clients[i] = Client();
        _is_operator[i] = false;
	}
}

Channel::~Channel() {}

void Channel::add_client(const Client& client)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (_clients[i].fd == -1)
		{
			_clients[i] = client;
            // Make first user an operator
            if (get_user_count() == 1) {
                _is_operator[i] = true;
            }
			return;
		}
	}
}

void Channel::remove_client(const Client& client)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (_clients[i].fd == client.fd)
		{
			_clients[i] = Client();
			return;
		}
	}
}

bool Channel::has_client(const Client& client) const
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (_clients[i].fd == client.fd)
			return true;
	}
	return false;
}

void Channel::broadcast(const std::string& message, int exclude_fd) const
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (_clients[i].fd != -1 && _clients[i].fd != exclude_fd)
		{
			write(_clients[i].fd, message.c_str(), message.length());
		}
	}
}

std::string Channel::build_names_list() const
{
	std::string names;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (_clients[i].fd != -1 && !_clients[i].nick.empty())
		{
			if (!names.empty()) names += " ";
            if (_is_operator[i]) names += "@";
            names += _clients[i].nick;
		}
	}
	return names;
}

int Channel::get_user_count() const
{
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (_clients[i].fd != -1) count++;
    return count;
}

int Channel::find_client_index(const Client& client) const
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (_clients[i].fd == client.fd) return i;
    return -1;
}

bool Channel::isOperator(const Client& client) const
{
    int idx = find_client_index(client);
    return idx >= 0 ? _is_operator[idx] : false;
}

void Channel::setOperator(const Client& client, bool enabled)
{
    int idx = find_client_index(client);
    if (idx >= 0) _is_operator[idx] = enabled;
}

void Channel::inviteNick(const std::string& nick)
{
    _invited_nicks.push_back(nick);
}

bool Channel::isNickInvited(const std::string& nick) const
{
    for (size_t i = 0; i < _invited_nicks.size(); ++i)
        if (_invited_nicks[i] == nick) return true;
    return false;
}

void Channel::removeNickInvite(const std::string& nick)
{
    for (std::vector<std::string>::iterator it = _invited_nicks.begin(); it != _invited_nicks.end(); ++it) {
        if (*it == nick) { _invited_nicks.erase(it); break; }
    }
}
