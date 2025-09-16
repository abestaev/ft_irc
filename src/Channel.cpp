#include "Channel.hpp"
#include <algorithm>
#include <unistd.h>

Channel::Channel(const std::string& name): 
	_name(name), 
	_topic(""), 
	_password(""), 
	_user_limit(0), 
	_invite_only(false), 
	_topic_restricted(false), 
	_moderated(false)
{
	// Initialize client arrays
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		_clients[i] = Client();
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
			names += _clients[i].nick;
		}
	}
	return names;
}
