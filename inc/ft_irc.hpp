#ifndef FT_IRC_HPP
#define FT_IRC_HPP

// Standard includes
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>

// System includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>

// Project includes
#include "Message.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Commands.hpp"

// Constants
#define DEFAULT_PORT 6667
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Global functions
void error(std::string msg);

#endif