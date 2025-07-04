#include "ft_irc.hpp"

#include <cstdlib>
#include <cstdio>

void error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

int	main(int ac, char **av)
{
	int	sockfd;
	int	newsockfd;
	int	portno;
	int	clientfds[MAX_CLIENTS];
	int	ready;
	Client *clients[MAX_CLIENTS];
	socklen_t clilen;
	int opt = 1;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	
	struct pollfd *pfds;
	int nfds;

	// int	set_size = 0;
	// fd_set fdset;

	for (int i = 0; i < MAX_CLIENTS; i++)
		clientfds[i] = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("socket error");

	// override "TIME_WAIT state" behavior / allows port to be reused immediatly
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

	portno = DEFAULT_PORT;
	if (ac >= 2)
		portno = std::atoi(av[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("bind error");

	listen(sockfd, 5);

	clilen = sizeof(cli_addr);
	// FD_ZERO(&fdset);
	// FD_SET(sockfd, &fdset);
	while (true)
	{
		
		
		ready = poll(pfds, nfds, -1);
		if (ready < 0)
			error("poll error");
		
	}

	// close(newsockfd);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clientfds[i] != -1)
			close(clientfds[i]);
	}
	close(sockfd);
	return 0;
	//// idk what im doing btw

	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0)
	{
		perror("Error on accept");
		return 1;
	}
	n = read(newsockfd, buffer, BUFFER_SIZE - 1);
	//some sort of gnl on newsockfd might work better

	if (n < 0)
	{
		perror("Error reading from socket");
		return 1;
	}
	std::cout << "received message: " << buffer << std::endl;
	n = write(newsockfd, "message received", 17);
	if (n < 0)
	{
		perror("Error on writing to socket");
		return 1;
	}
	close(newsockfd);
	close(sockfd);
	return 0;
}