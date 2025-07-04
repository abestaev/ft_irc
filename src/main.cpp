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
	int	ready;
	Client *clients[MAX_CLIENTS];
	int opt = 1;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	int n;
	
	struct pollfd *pfds;//[MAX_CLIENTS];
	int nfds = 1;
	pfds[0].fd = sockfd;
	pfds[0].events = POLLIN;

	pfds = new struct pollfd[MAX_CLIENTS];
	for (int i = 1; i < MAX_CLIENTS; i++)
		pfds[i].fd = -1;
	
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
	while (true)
	{	
		ready = poll(pfds, nfds, -1);
		if (ready < 0)
			error("poll error");

		/*non blocking socket
			while (pfds[0].revents & POLLIN)
		*/
		if (pfds[0].revents & POLLIN)
		{
			newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
			if (newsockfd < 0)
			{
				/*non blocking socket
					if (errno == EWOULDBLOCK || errno == EAGAIN)
						break ;
					perror("accept error");
					break ;
				*/
				perror("accept error");
				continue ;
			}
		}
	}
	
	// close(newsockfd);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (pfds[i].fd != -1)
			close(pfds[i].fd);
	}
	close(sockfd);
	delete[] pfds;
	return 0;
	//// idk what im doing btw
	
	// clilen = sizeof(cli_addr);
	// newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	// if (newsockfd < 0)
	// {
	// 	perror("Error on accept");
	// 	return 1;
	// }
	// n = read(newsockfd, buffer, BUFFER_SIZE - 1);
	// //some sort of gnl on newsockfd might work better
	
	// if (n < 0)
	// {
	// 	perror("Error reading from socket");
	// 	return 1;
	// }
	// std::cout << "received message: " << buffer << std::endl;
	// n = write(newsockfd, "message received", 17);
	// if (n < 0)
	// {
	// 	perror("Error on writing to socket");
	// 	return 1;
	// }
	// close(newsockfd);
	// close(sockfd);
	// return 0;
}