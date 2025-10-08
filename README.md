# ft_irc (minimal IRC server)

## Build

```bash
make
```

## Run

```bash
./ircserv <port> <password>
# example
./ircserv 6667 secret
```

## Quick test with netcat

```bash
printf "PASS secret\r\nNICK alice\r\nUSER u 0 * :Real\r\nPING 42\r\n" | nc -v 127.0.0.1 6667 -w 5
```

Expected: password ack, welcome, then `PONG :42`.

## Supported commands
- PASS, NICK, USER, CAP, PING/PONG, QUIT
- Channels: JOIN, PART, TOPIC (+t), LIST, NAMES
- Messaging: PRIVMSG, NOTICE (channel and user)
- Moderation: INVITE, KICK, MODE (channel and user modes), OPER, KILL

## Notes
- Non-blocking sockets with poll(2)
- Per-client input buffering; messages parsed on CRLF (LF accepted)
- Constants in `inc/config.hpp`
- Basic numerics (001..), minimal MOTD

## Known limitations
- Some RFC numerics (005, 251–255, LUSERS, etc.) are minimal or omitted
- Channel mode enforcement is partial (e.g. +m moderated and +n external filtering may need refinement)
- Capability negotiation (CAP) is minimal (no real capabilities)
