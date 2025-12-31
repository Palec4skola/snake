#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client.h"
#include "../game.h"

#define SERVER_PORT 9998
void show_menu() {
    printf("\n=== SNAKE GAME ===\n");
    printf("1. Nova hra\n");
    printf("2. Pripojit sa k hre\n");
    printf("3. Koniec\n");
    printf("Vyber: ");
}

void start_new_game() {
    game_config_t config;
    int mode;
    printf("Zvol herny rezim:\n");
    printf("1 - Standardny\n");
    printf("2 - Casovy:\n");
    printf("> ");
    scanf("%d", &mode);
    config.mode = mode;

    if (config.mode == GAME_TIMED) {
      printf("Zadaj cas hry (sekundy): ");
      scanf("%d",&config.time_limit);
    }else {
      config.time_limit = 0;
    }
    char mode_str[10];
    char time_str[10];
    sprintf(mode_str, "%d", config.mode);
    sprintf(time_str, "%d", config.time_limit);

    pid_t pid = fork();
    printf("pid: %d\n",pid);

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        if (setsid() < 0) {
            perror("setsid");
        }
        printf("Server sa spusta.\n");
        execl("./server_app", "server_app", mode_str, time_str, NULL);
        perror("execl");
        exit(1);    
    } 
    else {
        printf("Server spusteny (PID=%d) client\n", pid);
    }
}

void join_game() {
    int clientSocket;
    struct sockaddr_in server_addr;
    char buffer[256];
    int n;
    printf("priprajanie klienta\n");

    // 1. vytvor socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("socket");
        return;
    }
    // 2. nastav adresu servera
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // 3. pripoj sa na server
    if (connect(clientSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(clientSocket);
        return;
    }
    printf("pripojeny na server\n");
    printf("Pripojeny k hre na porte %d\n", SERVER_PORT);

    // 4. posli spravu serveru
    //strcpy(buffer, "HELLO FROM CLIENT\n");
    //write(clientSocket, buffer, strlen(buffer));
    //printf("poslal spravu serveru\n");
    // 5. precitaj odpoved
    n = recv(clientSocket, buffer, sizeof(buffer) - 1,0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Odpoved od servera: %s\n", buffer);
    }

    close(clientSocket);
}

int main() {
    int choice;

    while (1) {
        show_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // flush
            continue;
        }

        switch (choice) {
            case 1:
                printf("Nova hra\n");
                start_new_game();
                break;
            case 2:
                printf("Pripojenie k hre\n");
                join_game();
                break;
            case 3:
                printf("Koniec\n");
                return 0;
            default:
                printf("Neplatna volba\n");
        }
    }
}

