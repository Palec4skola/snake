#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

#define SERVER_PORT 9997
void show_menu() {
  printf("\n=== SNAKE GAME ===\n");
  printf("1. Nova hra\n");
  printf("2. Pripojit sa k hre\n");
  printf("3. Koniec\n");
  printf("Vyber: ");
}

void start_new_game() {
  game_config_t config;
  int mode_time;
  int mode_obs;
  printf("Zvol herny rezim:\n");
  printf("1 - Standardny\n");
  printf("2 - Casovy:\n");
  printf("> ");
  scanf("%d", &mode_time);
  printf("\nZvol rezim s/bez prekazok\n");
  printf("1 - S prekazkami\n");
  printf("2 - Bez prekazok\n");
  printf("> ");
  scanf("%d",&mode_obs);
  config.mode_obs = mode_obs;
  config.mode = mode_time;

  if (config.mode == GAME_TIMED) {
    printf("Zadaj cas hry (sekundy): ");
    scanf("%d",&config.time_limit);
  }else {
    config.time_limit = 0;
  }
  char mode_str[10];
  char time_str[10];
  char mode_obs_str[10];
  sprintf(mode_str, "%d", config.mode);
  sprintf(time_str, "%d", config.time_limit);
  sprintf(mode_obs_str, "%d", config.mode_obs);
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
    execl("./server_out", "server_out", mode_str, time_str, mode_obs_str, NULL);
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
  sleep(1); 
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
  system("clear");
  for (int i = 0; i < MAP_W + 2; i++){
    printf("-");
  }
  printf("\n");
  for (int i = 0; i < MAP_H; i++) {
    printf("|");
    for (int j = 0;j < MAP_W; j++) {
      char c = ' ';

      for (int k = 0; k < state->active_players;k++) {
        for (int l = 0;l < state->snakes[k].length;l++) {
          if (state->snakes[k].body[l].x==j && state->snakes[k].body[l].y == i) {
         c = (l==0)?'O':'o'; 
          }
        }
      }


      if (state->fruit[0].x == j && state->fruit[0].y ==i) {
        c = '*';
      }
      printf("%c",c);
    }
    printf("|\n");
  }
  for (int i = 0; i < MAP_W + 2; i++) {
    printf("-");
  }
  printf("\n");
  printf("Points: %d\n",state->snakes[0].points);
  printf("Cas: %ld\n", time(NULL) - state->snakes[0].time_start);
}
void* recv_loop(void* arg) {
  client_t* client = (client_t*)arg;
  pthread_mutex_lock(&client->mutex);
  int client_fd = client->client_fd;
  pthread_mutex_unlock(&client->mutex);
  game_state_t state;
  while (1) {
    int bytes = recv(client_fd, &state, sizeof(game_state_t), 0);

    if (bytes <= 0) {
      printf("Server ukončil spojenie\n");
      break;
    }

    if (state.active_players == 0) {
      printf("Nie je zivi ziadny hradik, hra konci\n");
      break;
    } else {
      draw_map(&state);
    }
  }
  pthread_mutex_lock(&client->mutex);
  client->game_running = 0;
  pthread_mutex_unlock(&client->mutex);
  return NULL;
}

void run_game(int client_fd){
  client_t *client = malloc(sizeof(client_t));
  client->client_fd = client_fd;
  client->game_running = 1;

  pthread_mutex_init(&client->mutex,NULL);
  pthread_t recv_thread;
  pthread_create(&recv_thread, NULL, recv_loop, client);
  pthread_detach(recv_thread);

  while (1) {
    pthread_mutex_lock(&client->mutex);
    int running = client->game_running;
    pthread_mutex_unlock(&client->mutex);
    
    usleep(20000);
    if (running == 0) {
      break;
    }
    char key = getchar();
    send(client_fd,&key,sizeof(key),0);
      
  }
  printf("Koniec klienta\n");
  pthread_mutex_destroy(&client->mutex);
  free(client);
  close(client_fd);
  exit(0);
}

int main() {
  int choice;
  int client;
  while (1) {
    show_menu();
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n');
      continue;
    }

    switch (choice) {
      case 1:
        printf("Nova hra\n");
        start_new_game();
        sleep(1);
        client = join_game();
        if (client < 0) {
          return 1;
        }
        run_game(client);
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



