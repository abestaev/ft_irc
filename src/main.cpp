#include "ft_irc.hpp"

void error(std::string msg)
{
	perror(msg.c_str());
	exit(EXIT_FAILURE);
}

int	main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cout << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return (1);
	}

	int portno = std::atoi(av[1]);
	std::string pass = std::string(av[2]);

	// Validate port number
	if (portno <= 0 || portno > 65535)
	{
		std::cout << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
		return (1);
	}

	// Validate password
	if (pass.empty())
	{
		std::cout << "Error: Password cannot be empty." << std::endl;
		return (1);
	}

	try
	{
		Server server(portno, pass);
		server.init();
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}

	return (0);
}
