#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

// IRC Message structure according to RFC 1459
class Message
{
private:
	std::string _prefix;      // Optional prefix (nick!user@host)
	std::string _command;     // Command name (NICK, USER, etc.)
	std::vector<std::string> _params; // Command parameters
	std::string _trailing;    // Trailing parameter (after :)

public:
	Message();
	Message(const std::string& raw_message);
	~Message();
	
	// Getters
	const std::string& getPrefix() const { return _prefix; }
	const std::string& getCommand() const { return _command; }
	const std::vector<std::string>& getParams() const { return _params; }
	const std::string& getTrailing() const { return _trailing; }
	
	// Utility methods
	bool hasPrefix() const { return !_prefix.empty(); }
	bool hasTrailing() const { return !_trailing.empty(); }
	size_t getParamCount() const { return _params.size(); }
	
	// Parse raw IRC message
	void parse(const std::string& raw_message);
	
	// Clear message
	void clear();
};

#endif
