#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client.h"
#include "../game.h"
#include "../server/server.h"

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

int join_game() {
    int clientSocket;
    struct sockaddr_in server_addr;
    printf("priprajanie klienta\n");

    // 1. vytvor socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("socket");
        return -1;
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
        return -1;
    }
    printf("pripojeny na server\n");
    printf("Pripojeny k hre na porte %d\n", SERVER_PORT);
    return clientSocket;
}

void draw_map(game_state_t* state) {
  //system("clear");
  printf("Kreslenie mapy aktivny hraci: %d\n",state->active_players);
  printf("%d:%d\n",state->snakes[0].body->x, state->snakes[0].body->y);
  for (int i = 0; i < MAP_H; i++) {
    printf("");
    for (int j = 0;j < MAP_W; j++) {
      char c = ' ';

      for (int k = 0; k < state->active_players;i++) {
        for (int l = 0;l < state->snakes[k].length;l++) {
          if (state->snakes[k].body->x==j && state->snakes[k].body->y == i) {
         c = (i==0)?'O':'o'; 
          }
        }
      }


      if (state->fruit->x == j && state->fruit->y ==1) {
        c = '*';
      }
      printf("%c",c);
    }
    printf("\n");
  }
  printf("%d:%d\n",state->snakes[0].body->x, state->snakes[0].body->y);
}
void* recv_loop(void* arg) {
  int fd = *(int*)arg;
  game_state_t state;
  while (1) {
    if (recv(fd,&state,sizeof(game_state_t),0) <= 0 ) {
      printf("Chyba pri recv\n");
      break;
    }
    printf("Hlava hada: %d:%d\n",state.snakes[0].body[0].x,state.snakes[0].body[0].x);
    draw_map(&state);
  }
  free(arg);
  return NULL;
}

void run_game(int client_fd){
  int *fd = malloc(sizeof(int));
  *fd = client_fd;
  pthread_t recv_thread;
  pthread_create(&recv_thread, NULL, recv_loop, fd);

  while (1) {
    sleep(1);
    char key = getchar();
    send(client_fd,&key,1,0);
  }
}

int main() {
    int choice;
    int client;
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
                client = join_game();
                if (client < 0) {
                  return 1;
                }
                run_game(client);
                break;
            case 3:
                printf("Koniec\n");
                return 0;
            default:
                printf("Neplatna volba\n");
        }
    close(client);
    }
}

