#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

class IRCBot {
private:
    int _sockfd;
    std::string _buffer;
    std::set<std::string> _banwords;
    std::set<std::string> _joined_channels;
    std::string _nick;
    std::string _password;
    time_t _last_list_check;
    
    void send_raw(const std::string& msg) {
        std::string full = msg + "\r\n";
        send(_sockfd, full.c_str(), full.length(), 0);
    }
    
    void process_line(const std::string& line) {
        // PING/PONG
        if (line.find("PING") == 0) {
            send_raw("PONG " + line.substr(5));
            return;
        }
        
        // Check for OPER confirmation
        if (line.find("381") != std::string::npos || line.find("IRC operator") != std::string::npos) {
            std::cout << "\033[32m[✓]\033[0m Bot is now an IRC operator" << std::endl;
            // Request list of all channels to auto-join them
            send_raw("LIST");
        }
        
        // Check for error on KICK
        if (line.find("482") != std::string::npos) {
            std::cout << "\033[31m[✗]\033[0m Permission denied - OPER failed" << std::endl;
        }
        
        // Parse LIST response (322 = channel list item)
        if (line.find(" 322 ") != std::string::npos) {
            // Format: :server 322 nick #channel users :topic
            size_t chan_start = line.find("#");
            if (chan_start != std::string::npos) {
                std::string channel = line.substr(chan_start);
                channel = channel.substr(0, channel.find(" "));
                if (_joined_channels.find(channel) == _joined_channels.end()) {
                    _joined_channels.insert(channel);
                    send_raw("JOIN " + channel);
                    std::cout << "\033[36m[→]\033[0m Joining existing channel " << channel << std::endl;
                }
            }
        }
        
        // JOIN notification - auto-join new channels
        if (line.find("JOIN") != std::string::npos) {
            size_t pos = line.find("#");
            if (pos != std::string::npos) {
                std::string channel = line.substr(pos);
                channel = channel.substr(0, channel.find(" "));
                channel = channel.substr(0, channel.find("\r"));
                
                // Check if it's our own JOIN confirmation
                if (line.find(":" + _nick + "!") == 0 || line.find(":" + _nick + " ") == 0) {
                    std::cout << "\033[36m[→]\033[0m Joined " << channel << std::endl;
                }
                // Auto-join if it's someone else creating a new channel
                else if (_joined_channels.find(channel) == _joined_channels.end()) {
                    _joined_channels.insert(channel);
                    std::cout << "\033[36m[→]\033[0m Auto-joining " << channel << std::endl;
                    send_raw("JOIN " + channel);
                }
            }
        }
        
        // PRIVMSG - check for ban words and commands
        if (line.find("PRIVMSG") != std::string::npos) {
            // Parse: :nick!user@host PRIVMSG #channel :message
            // OR:    :nick PRIVMSG #channel :message
            
            if (line[0] != ':') return; // Must start with :
            
            size_t nick_end = line.find('!');
            if (nick_end == std::string::npos) {
                // No !, try to find space (format: :nick PRIVMSG)
                nick_end = line.find(' ');
            }
            
            size_t chan_start = line.find("#");
            size_t msg_start = line.find(" :", chan_start);
            
            if (nick_end != std::string::npos && chan_start != std::string::npos && msg_start != std::string::npos) {
                std::string nick = line.substr(1, nick_end - 1);
                std::string channel = line.substr(chan_start);
                channel = channel.substr(0, channel.find(" "));
                std::string message = line.substr(msg_start + 2);
                
                // Clean message from \r\n
                size_t end = message.find("\r");
                if (end != std::string::npos) message = message.substr(0, end);
                end = message.find("\n");
                if (end != std::string::npos) message = message.substr(0, end);
                
                // Ignore our own messages
                if (nick == _nick) {
                    return;
                }
                
                // Command: !addban <word>
                if (message.find("!addban ") == 0) {
                    std::string word = message.substr(8);
                    word = word.substr(0, word.find(" "));
                    word = word.substr(0, word.find("\r"));
                    word = word.substr(0, word.find("\n"));
                    _banwords.insert(word);
                    send_raw("PRIVMSG " + channel + " :Ban word added: " + word);
                    std::cout << "\033[33m[+]\033[0m Ban word added: " << word << std::endl;
                    return;
                }
                
                // Command: !listbans
                if (message.find("!listbans") == 0) {
                    std::string list = "Ban words: ";
                    for (std::set<std::string>::iterator it = _banwords.begin(); it != _banwords.end(); ++it) {
                        list += *it + " ";
                    }
                    send_raw("PRIVMSG " + channel + " :" + list);
                    return;
                }
                
                // Check for ban words
                for (std::set<std::string>::iterator it = _banwords.begin(); it != _banwords.end(); ++it) {
                    if (message.find(*it) != std::string::npos) {
                        std::cout << "\033[31m[!]\033[0m Kicked " << nick << " from " << channel << " (reason: " << *it << ")" << std::endl;
                        send_raw("KICK " + channel + " " + nick + " :Ban word detected: " + *it);
                        return;
                    }
                }
            }
        }
    }
    
public:
    IRCBot(const std::string& nick) : _sockfd(-1), _nick(nick), _last_list_check(0) {
        // Ban words par défaut
        _banwords.insert("badword");
        _banwords.insert("spam");
    }
    
    ~IRCBot() {
        if (_sockfd != -1)
            close(_sockfd);
    }
    
    bool connect(const std::string& host, int port, const std::string& password) {
        _password = password;
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);
        
        if (::connect(_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error connecting to server" << std::endl;
            return false;
        }
        
        // Register and become operator
        send_raw("CAP LS");
        send_raw("PASS " + password);
        send_raw("NICK " + _nick);
        send_raw("USER " + _nick + " 0 * :IRC Bot");
        send_raw("CAP END");
        send_raw("OPER admin admin123");
        
        return true;
    }
    
    void join_channel(const std::string& channel) {
        _joined_channels.insert(channel);
        send_raw("JOIN " + channel);
    }
    
    void run() {
        struct pollfd pfd;
        pfd.fd = _sockfd;
        pfd.events = POLLIN;
        
        char buf[4096];
        std::cout << "\033[32m=== BotGuard Active ===\033[0m" << std::endl;
        std::cout << "Ban words: ";
        for (std::set<std::string>::iterator it = _banwords.begin(); it != _banwords.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
        
        while (true) {
            // Check for new channels every 10 seconds
            time_t now = time(NULL);
            if (now - _last_list_check >= 2) {
                send_raw("LIST");
                _last_list_check = now;
            }
            
            int ret = poll(&pfd, 1, 100);
            if (ret > 0 && (pfd.revents & POLLIN)) {
                int n = recv(_sockfd, buf, sizeof(buf) - 1, 0);
                if (n <= 0) {
                    std::cerr << "Connection closed" << std::endl;
                    break;
                }
                
                buf[n] = '\0';
                _buffer += buf;
                
                // Process complete lines
                size_t pos;
                while ((pos = _buffer.find("\r\n")) != std::string::npos) {
                    std::string line = _buffer.substr(0, pos);
                    _buffer = _buffer.substr(pos + 2);
                    if (!line.empty())
                        process_line(line);
                }
            }
        }
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }
    
    int port = atoi(argv[1]);
    std::string password = argv[2];
    
    IRCBot bot("BotGuard");
    
    if (!bot.connect("127.0.0.1", port, password)) {
        return 1;
    }
    
    bot.run();
    return 0;
}

