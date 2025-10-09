# 🚀 ft_irc - IRC Server Implementation

<div align="center">

**A fully compliant, non-blocking IRC server built in C++98**

[![42 Project](https://img.shields.io/badge/42-Project-blue)](https://42.fr)
[![Language](https://img.shields.io/badge/C%2B%2B-98-green)](https://en.wikipedia.org/wiki/C%2B%2B98)
[![Protocol](https://img.shields.io/badge/RFC-1459%20%7C%202812-orange)](https://datatracker.ietf.org/doc/html/rfc2812)

</div>

---

## 📋 Table of Contents

- [Features](#-features)
- [Requirements](#-requirements)
- [Installation](#-installation)
- [Usage](#-usage)
- [Architecture](#-architecture)
- [Supported Commands](#-supported-commands)
- [Testing](#-testing)
- [Technical Details](#-technical-details)
- [Project Structure](#-project-structure)

---

## ✨ Features

### Core IRC Functionality
- ✅ **Full Authentication** - Password-protected server with PASS/NICK/USER registration
- ✅ **Channel Management** - Create, join, part channels with operator privileges
- ✅ **Private Messaging** - User-to-user and channel messaging (PRIVMSG/NOTICE)
- ✅ **Channel Modes** - Invite-only (+i), topic protection (+t), password (+k), user limit (+l), operator (+o)
- ✅ **User Modes** - Invisible (+i), wallops (+w), restricted (+r), operator (+o/O)
- ✅ **Moderation Tools** - KICK, INVITE, OPER, KILL commands
- ✅ **Discovery** - WHO, WHOIS, LIST, NAMES commands

### Technical Excellence
- ✅ **Non-blocking I/O** - Single `poll()` call for all operations
- ✅ **RFC Compliant** - Follows IRC protocol specifications (RFC 1459/2812)
- ✅ **Robust Parsing** - Handles partial data, multiple line endings (CRLF/LF)
- ✅ **Output Buffering** - Queued sends with POLLOUT management
- ✅ **Signal Handling** - Graceful shutdown on SIGINT/SIGTERM
- ✅ **Colored Logging** - ANSI-colored server logs for debugging

---

## 🔧 Requirements

- **Compiler**: `g++` or `clang++` with C++98 support
- **OS**: Linux (tested on Ubuntu/Debian)
- **Build tool**: `make`
- **Client**: Any IRC client (WeeChat, irssi, HexChat, nc)

---

## 📦 Installation

```bash
# Clone the repository
git clone <your-repo-url> ft_irc
cd ft_irc

# Build the project
make

# The binary 'ircserv' will be created in the root directory
```

### Compilation Flags
```makefile
-Wall -Wextra -Werror -std=c++98 -g3
```

---

## 🚀 Usage

### Starting the Server

```bash
./ircserv <port> <password>
```

**Example:**
```bash
./ircserv 6667 mySecurePass123
```

**Output:**
```
[INFO] Starting IRC server on port 6667
[INFO] Password required for connection: mySecurePass123
[INFO] Listening on 0.0.0.0:6667
Press Ctrl+C to stop the server
```

### Connecting with a Client

#### Using netcat (quick test)
```bash
nc 127.0.0.1 6667
PASS mySecurePass123
NICK alice
USER alice 0 * :Alice Wonderland
JOIN #general
PRIVMSG #general :Hello everyone!
QUIT :Goodbye
```

#### Using WeeChat
```bash
weechat
/server add local 127.0.0.1/6667 -password=mySecurePass123
/connect local
/nick alice
/join #general
```

#### Using irssi
```bash
irssi
/connect 127.0.0.1 6667 mySecurePass123 alice
/join #general
```

---

## 🏗️ Architecture

### Single Poll() Loop
```
┌──────────────────────────────────────────┐
│     Main Loop (Server::run)              │
│                                          │
│  while (!signal) {                       │
│      poll(_pfds, _nfds, 100ms);          │ ← ONE poll() call
│                                          │
│      if (POLLIN on listening socket)     │
│          → accept_new_clients()          │ (1 accept/cycle)
│                                          │
│      if (POLLIN on client socket)        │
│          → process_client_messages()     │ (1 read/client)
│                                          │
│      if (POLLOUT on client socket)       │
│          → flush output buffers          │ (1 send/client)
│  }                                       │
└──────────────────────────────────────────┘
```

### Data Flow
```
Client → [recv] → inbuf → [parse] → Command → [process] → outbuf → [send] → Client
```

### Key Design Decisions

1. **Non-blocking I/O**: All sockets use `fcntl(fd, F_SETFL, O_NONBLOCK)`
2. **Input Buffering**: Accumulates partial data until complete line (`\r\n` or `\n`)
3. **Output Buffering**: Queues messages, sends on `POLLOUT` readiness
4. **Single Poll**: Exactly one `poll()` call per event loop iteration
5. **No errno loops**: No `EAGAIN` checking to trigger re-reads (subject requirement)

---

## 📡 Supported Commands

### Authentication & Registration
| Command | Description | Example |
|---------|-------------|---------|
| `PASS` | Set connection password | `PASS mySecurePass123` |
| `NICK` | Set/change nickname | `NICK alice` |
| `USER` | Set username and realname | `USER alice 0 * :Alice W.` |
| `CAP` | Capability negotiation | `CAP LS` |

### Channel Operations
| Command | Description | Example |
|---------|-------------|---------|
| `JOIN` | Join channel(s) | `JOIN #general,#random` |
| `PART` | Leave channel(s) | `PART #general :Goodbye` |
| `TOPIC` | View/set channel topic | `TOPIC #general :New topic` |
| `LIST` | List all channels | `LIST` |
| `NAMES` | List channel members | `NAMES #general` |

### Messaging
| Command | Description | Example |
|---------|-------------|---------|
| `PRIVMSG` | Send message | `PRIVMSG #general :Hello!` |
| `NOTICE` | Send notice (no auto-reply) | `NOTICE alice :Info` |

### Moderation & Admin
| Command | Description | Example |
|---------|-------------|---------|
| `KICK` | Remove user from channel | `KICK #general bob :Bad behavior` |
| `INVITE` | Invite user to channel | `INVITE alice #private` |
| `MODE` | Change channel/user modes | `MODE #general +it` |
| `OPER` | Gain operator privileges | `OPER admin pass` |
| `KILL` | Disconnect user (oper only) | `KILL bob :Abuse` |

### Information
| Command | Description | Example |
|---------|-------------|---------|
| `WHO` | Get user info | `WHO #general` |
| `WHOIS` | Detailed user info | `WHOIS alice` |
| `PING` | Keep-alive check | `PING token` |
| `PONG` | Response to PING | *(automatic)* |

### Other
| Command | Description | Example |
|---------|-------------|---------|
| `QUIT` | Disconnect | `QUIT :Bye` |
| `ERROR` | Server error message | *(server-sent)* |

---

## 🧪 Testing

### Manual Testing Scenarios

#### 1. Basic Connection & Authentication
```bash
echo -e "PASS wrong\r\nQUIT\r\n" | nc 127.0.0.1 6667
# Expected: ERROR :Password incorrect
```

#### 2. Multi-Client Chat
```bash
# Terminal 1
nc 127.0.0.1 6667
PASS mySecurePass123
NICK alice
USER alice 0 * :Alice
JOIN #test

# Terminal 2
nc 127.0.0.1 6667
PASS mySecurePass123
NICK bob
USER bob 0 * :Bob
JOIN #test
PRIVMSG #test :Hello alice!
```

#### 3. Channel Modes
```bash
MODE #test +i          # Invite-only
MODE #test +t          # Topic protection
MODE #test +k secret   # Set password
MODE #test +l 10       # Max 10 users
MODE #test +o alice    # Give alice operator
```

#### 4. Partial Data (Subject Requirement)
```bash
# Send command in fragments (simulating slow connection)
(printf "PA"; sleep 0.1; printf "SS pass"; sleep 0.1; printf "123\r\n") | nc 127.0.0.1 6667
# Expected: Server correctly assembles and processes "PASS pass123"
```

---

## 🔍 Technical Details

### Compliance with Subject Requirements

#### ✅ Non-blocking I/O
- All sockets use `O_NONBLOCK` via `fcntl(fd, F_SETFL, O_NONBLOCK)`
- No other `fcntl()` usage (F_GETFL forbidden by subject)

#### ✅ Single poll()
- Exactly **1** `poll()` call in entire codebase (`src/Server.cpp:110`)
- No `select()`, `epoll()`, or `kqueue()`
- Verify with: `grep -r "poll(" src/`

#### ✅ poll() before I/O
- Every `accept()` preceded by `POLLIN` check
- Every `read()` preceded by `POLLIN` check  
- Every `send()` preceded by `POLLOUT` check
- **No errno checking** for `EAGAIN` to trigger actions (forbidden by subject)

#### ✅ Partial Data Handling
- Input buffer accumulates data until complete line
- Supports both `\r\n` (CRLF) and `\n` (LF) line endings
- Tested with fragmented input (see `test_partial_data.sh`)

### Error Handling

- **Socket errors**: Logged and client disconnected
- **Unknown commands**: `421 ERR_UNKNOWNCOMMAND`
- **Missing parameters**: `461 ERR_NEEDMOREPARAMS`
- **Privilege errors**: `481/482 ERR_NOPRIVILEGES`
- **Channel errors**: `403/471/473/475` (No such channel, full, invite-only, bad key)

### Logging System

Color-coded server logs for debugging:
- 🔵 `[INFO]` - Server startup/status (blue)
- 🟢 `[AUTH]` - Authentication events (green)
- 🟡 `[CHANNEL]` - Channel operations (yellow)
- 🔴 `[ERROR]` - Errors (red)
- 🟣 `[CMD]` - Commands received (magenta)
- 🔵 `[STATE]` - Server state (blue)
- 🟠 `[CONNECT]` - New connections (cyan)
- 🟡 `[DISCONNECT]` - Client disconnections (yellow)

---

## 📁 Project Structure

```
ft_irc/
├── inc/                          # Header files
│   ├── Channel.hpp              # Channel management
│   ├── Client.hpp               # Client structure
│   ├── Commands.hpp             # Command handler class
│   ├── Message.hpp              # IRC message parser
│   ├── Server.hpp               # Main server class
│   ├── config.hpp               # Constants (MAX_CLIENTS, BUFFER_SIZE)
│   ├── ft_irc.hpp               # Common includes
│   └── utils.hpp                # Utility functions
│
├── src/                          # Source files
│   ├── main.cpp                 # Entry point
│   ├── Server.cpp               # Server implementation (poll loop)
│   ├── Client.cpp               # Client methods
│   ├── Channel.cpp              # Channel methods
│   ├── Message.cpp              # IRC message parsing
│   ├── Commands.cpp             # Command dispatcher
│   ├── utils.cpp                # Helper functions
│   │
│   └── commands/                # Individual command handlers
│       ├── CAP.cpp              # Capability negotiation
│       ├── PASS.cpp             # Password authentication
│       ├── NICK.cpp             # Nickname management
│       ├── USER.cpp             # User registration
│       ├── JOIN.cpp             # Channel joining (multi-channel support)
│       ├── PART.cpp             # Channel leaving (multi-channel support)
│       ├── TOPIC.cpp            # Topic management
│       ├── MODE.cpp             # Mode changes (channel & user)
│       ├── PRIVMSG_NOTICE.cpp   # Messaging
│       ├── INVITE_KICK.cpp      # Moderation
│       ├── OPER.cpp             # Operator authentication
│       ├── KILL.cpp             # Force disconnect
│       ├── WHO_WHOIS.cpp        # User information
│       ├── LIST_NAMES.cpp       # Channel listing
│       ├── PING_PONG.cpp        # Keep-alive
│       ├── QUIT.cpp             # Disconnect
│       └── ERROR.cpp            # Error handling
│
├── Makefile                      # Build configuration
```

### Key Classes

- **`Server`**: Main event loop, socket management, poll() handling
- **`Client`**: Client state (fd, nick, user, modes, buffers)
- **`Channel`**: Channel state (name, topic, modes, users, operators)
- **`Message`**: IRC message parser (command, params, trailing)
- **`Commands`**: Command dispatcher and handlers

---

## 📊 Configuration

Edit `inc/config.hpp` to customize:

```cpp
#define MAX_CLIENTS 100          // Maximum simultaneous clients
#define BUFFER_SIZE 4096         // Read buffer size
```

---

## 🐛 Known Limitations

### Intentional (Subject Constraints)
- Single server (no server-to-server linking)
- No SSL/TLS support
- No services (NickServ, ChanServ)
- Minimal capability negotiation (CAP)


## 🎯 Subject Compliance Checklist

### Mandatory Part
- [x] `./ircserv <port> <password>` usage
- [x] Non-blocking I/O on all operations
- [x] One single `poll()` (or equivalent)
- [x] `poll()` called before each `accept/read/recv/write/send`
- [x] No other `fcntl()` usage
- [x] No `errno` checks after I/O operations
- [x] TCP/IP (v4) sockets
- [x] Multiple clients without hanging
- [x] No forking
- [x] Partial data handling
- [x] Low bandwidth client support
- [x] Client-to-client communication
- [x] Authentication (password, nickname, username)
- [x] Channel operations (join, send/receive messages)
- [x] Forward messages to all channel clients
- [x] Distinguish operators from regular users
- [x] Operator-only commands implementation

### Commands Implemented
- [x] `PASS` - Password authentication
- [x] `NICK` - Set nickname
- [x] `USER` - Set username
- [x] `JOIN` - Join channels
- [x] `PART` - Leave channels
- [x] `PRIVMSG` - Send messages
- [x] `NOTICE` - Send notices
- [x] `KICK` - Kick user (operator)
- [x] `INVITE` - Invite to channel (operator)
- [x] `TOPIC` - View/set topic (operator if +t)
- [x] `MODE` - Channel/user modes (operator)
- [x] `PING/PONG` - Keep-alive

### Channel Modes
- [x] `+i` - Invite-only channel
- [x] `+t` - Topic restriction (operators only)
- [x] `+k` - Channel password
- [x] `+o` - Operator privileges
- [x] `+l` - User limit

---

## 📚 Resources

- [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459) - Original IRC Protocol
- [RFC 2812](https://datatracker.ietf.org/doc/html/rfc2812) - IRC Client Protocol
- [RFC 2813](https://datatracker.ietf.org/doc/html/rfc2813) - IRC Server Protocol
- [Modern IRC Documentation](https://modern.ircdocs.horse/)
- [Numeric Replies Reference](https://www.alien.net.au/irc/irc2numerics.html)

---

