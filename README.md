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

## Supported commands (minimal)
- PASS, NICK, USER: registration flow; welcome when complete
- PING/PONG
- JOIN <#chan>: create/join channel
- PART <#chan>
- NAMES <#chan>: lists nicks (basic)
- PRIVMSG <#chan> :text: broadcast to channel (basic)
- TOPIC <#chan> [ :topic ]: get/set topic
- QUIT: close link

## Notes
- Non-blocking sockets with poll(2)
- Per-client input buffering; messages parsed on CRLF
- Constants in `inc/config.hpp`

## Known limitations
- No user-to-user PRIVMSG
- No MODE/KICK/INVITE/OPER logic
- Channel/user limits & modes are placeholders
