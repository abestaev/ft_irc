#include "Message.hpp"
#include <algorithm>
#include <sstream>

Message::Message() {}

Message::Message(const std::string& raw_message)
{
	parse(raw_message);
}

Message::~Message() {}

void Message::parse(const std::string& raw_message)
{
	clear();
	
	if (raw_message.empty())
		return;
	
	std::string message = raw_message;
	
	// Remove trailing \r\n if present
	if (message.length() >= 2 && message.substr(message.length() - 2) == "\r\n")
		message = message.substr(0, message.length() - 2);
	
	// Parse prefix (optional)
	if (message[0] == ':')
	{
		size_t prefix_end = message.find(' ');
		if (prefix_end != std::string::npos)
		{
			_prefix = message.substr(1, prefix_end - 1);
			message = message.substr(prefix_end + 1);
		}
		else
		{
			_prefix = message.substr(1);
			return; // Only prefix, no command
		}
	}
	
	// Parse command (required)
	size_t cmd_end = message.find(' ');
	if (cmd_end != std::string::npos)
	{
		_command = message.substr(0, cmd_end);
		message = message.substr(cmd_end + 1);
	}
	else
	{
		_command = message;
		return; // Only command, no parameters
	}
	
	// Parse parameters and trailing
	if (!message.empty())
	{
		// Check for trailing parameter (starts with :)
		size_t trailing_pos = message.find(" :");
		if (trailing_pos != std::string::npos)
		{
			// Extract trailing
			_trailing = message.substr(trailing_pos + 2);
			message = message.substr(0, trailing_pos);
		}
		
		// Parse remaining parameters
		if (!message.empty())
		{
			std::istringstream iss(message);
			std::string param;
			while (iss >> param)
			{
				_params.push_back(param);
			}
		}
	}
	
	// Convert command to uppercase (IRC standard)
	std::transform(_command.begin(), _command.end(), _command.begin(), ::toupper);
}

void Message::clear()
{
	_prefix.clear();
	_command.clear();
	_params.clear();
	_trailing.clear();
}
