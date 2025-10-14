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
	{
		return;
	}

	std::string message = raw_message;

	// Remove trailing \r\n if present
	if (message.length() >= 2 && message.substr(message.length() - 2) == "\r\n")
		message = message.substr(0, message.length() - 2);

	//maybe needs to be refused when checking for specific commands or during parsing i dont know yet
	
	// Parse prefix (optional)
	if (message[0] == ':')
	{
		size_t prefix_end = message.find(' ');
		if (prefix_end != std::string::npos) {
			_prefix = message.substr(1, prefix_end - 1);
			message = message.substr(prefix_end + 1);
		} else {
			_prefix = message.substr(1);
			return; // only prefix, no command
		}
	}

	// parse command (required)
	size_t cmd_end = message.find(' ');
	if (cmd_end != std::string::npos) {
		_command = message.substr(0, cmd_end);
		message = message.substr(cmd_end + 1);
	} else {
		_command = message;
		return ; // only command, no parameters
	}

	// Parse parameters (and trailing !!! trailing is just a regular parameter)
	if (!message.empty())
	{
		if (!message.empty())
		{
			std::istringstream iss(message);
			std::string param;
			while (std::getline(iss, param, ' '))
			{
				if (param[0] == ':'){
					_has_trailing = true;
					std::string remaining;
					std::getline(iss, remaining, '\0');
					param += remaining;
					param = param.substr(1, param.size() - 1);
					iss.clear();//dsssssssssssssssssssssssssssvbggggggggggg -Moulinette
					iss.str(std::string());
				}
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
	_has_trailing = false;
	// _trailing.clear();
}
