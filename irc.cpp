#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6667
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket, client_socket[MAX_CLIENTS], max_sd, activity, valread, sd;
    int opt = 1;
    struct sockaddr_in address;
	socklen_t addrlen;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    // Initialise tous les clients à 0 (non connectés)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Créer socket serveur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Permet la réutilisation du port
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    // Configurer adresse
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "Serveur IRC démarré sur le port " << PORT << std::endl;

    // Écouter
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Boucle principale
    while (true) {
        // Réinitialise le set de sockets
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Ajoute les clients au set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        // Attendre activité
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0)) {
            perror("select error");
        }

        // Nouvelle connexion
        if (FD_ISSET(server_fd, &readfds)) {
            int addrlen = sizeof(address);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            std::cout << "Nouvelle connexion : socket fd " << new_socket << ", IP : " << inet_ntoa(address.sin_addr) << ", port : " << ntohs(address.sin_port) << std::endl;

            // Ajouter dans liste des clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        // Messages des clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // Déconnexion
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    std::cout << "Client déconnecté, IP : " << inet_ntoa(address.sin_addr) << ", port : " << ntohs(address.sin_port) << std::endl;
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    // Broadcast à tous sauf serveur et émetteur
                    buffer[valread] = '\0';
                    std::cout << "Message reçu : " << buffer;

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_socket[j] != 0 && client_socket[j] != sd) {
                            send(client_socket[j], buffer, strlen(buffer), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
