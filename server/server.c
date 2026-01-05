#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "server.h"

int main(int argc, char * argv[]) {
  srand(time(NULL));
  int server_fd;
  struct sockaddr_in addr;
  //socklen_t addrlen = sizeof(addr);
  struct sockaddr_in newAddr;

  socklen_t addrSize;
  // 1. vytvor socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
      perror("socket");
      exit(1);
  }

  // 2. nastav adresu
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(PORT);

  // 3. bind
  if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("bind");
      exit(1);
  }

  // 4. listen
  listen(server_fd, 5);//mnozstvo klientov, ktori sa mozu pripojit
  addrSize = sizeof(newAddr);

  pthread_t game_thread;
  game_config_t game;

  game.mode = atoi(argv[1]);
  game.time_limit = atoi(argv[2]);
  game.mode_obs = atoi(argv[3]);
  printf("obs_mode: %d\n",game.mode_obs);

  SHARED_DATA *data;
  data = malloc(sizeof(SHARED_DATA)); 
  client_thread_arg_t *arg; 
  arg = malloc(sizeof(client_thread_arg_t));

  data->game_state.active_players = 0;
  data->game_state.last_player_left = 0;
  data->isRunning =1;
  arg->shared = data;
  arg->server_fd = server_fd;
  pthread_mutex_init(&data->players_mutex, NULL);
  pthread_mutex_init(&data->clients_mutex, NULL);
  data->game = &game;
  pthread_create(&game_thread, NULL, game_loop, arg);
  pthread_detach(game_thread);  

  printf("Server spusteny na porte %d\n", PORT);
  printf("Herny rezim: %s\n",
      game.mode == GAME_STANDARD ? "STANDARDNY" : "CASOVY");

  if (game.mode == GAME_TIMED) {
      printf("Casovy limit: %d sekund\n", game.time_limit);
  }

  int game_runs;
  pthread_mutex_lock(&data->players_mutex);
  game_runs = data->isRunning;
  pthread_mutex_unlock(&data->players_mutex);
  
  while (game_runs) {
    pthread_t client_thread;
    
    int newSocket;//caka, kym sa pripoji klient
    printf("Caka na klienta: %d\n",server_fd);
    newSocket = accept(server_fd, (struct sockaddr*)&newAddr,&addrSize);
    printf("%d\n",newSocket);
    if (newSocket < 0) {
      break;
    }
      
    if (!arg) {
      perror("malloc");
      continue;
    }
    pthread_mutex_lock(&data->players_mutex);
    arg->client_fd[data->game_state.active_players] = newSocket;
   
    add_player(&arg->shared->game_state);
    pthread_mutex_unlock(&data->players_mutex);
    pthread_create(&client_thread, NULL, client_loop, arg);
    pthread_detach(client_thread);
    //free(arg);
    pthread_mutex_lock(&data->players_mutex);
    game_runs = data->isRunning;
    pthread_mutex_unlock(&data->players_mutex);
  }
  pthread_mutex_destroy(&data->players_mutex);
  pthread_mutex_destroy(&data->clients_mutex);
  printf("Koniec\n");
  free(data);
}

void * client_loop(void* arg){
  client_thread_arg_t* data = (client_thread_arg_t*)arg;
 // SHARED_DATA *shared = data->shared;
    //printf("arg CLIENT: %p\n", data);
  int player_id = data->shared->game_state.active_players-1;
  int client_fd = data->client_fd[player_id];
  char key;
  pthread_mutex_lock(&data->shared->players_mutex);
  int isAlive = data->shared->game_state.snakes[player_id].alive;
  int game_runs = data->shared->isRunning;
  pthread_mutex_unlock(&data->shared->players_mutex);
  while(isAlive == 1 && game_runs == 1) {
    int n;
    n = recv(client_fd, &key, 1, 0);
    if ( n > 0) {
      if (key == 27) {
        printf("Pauza\n");
      } else if (key != 'w' && key != 'a' && key != 's' && key != 'd'){
        continue;
      }      pthread_mutex_lock(&data->shared->players_mutex);
      data->shared->game_state.snakes[player_id].direction = key;
      pthread_mutex_unlock(&data->shared->players_mutex);

    }
    usleep(200000);
    pthread_mutex_lock(&data->shared->players_mutex);
    isAlive = data->shared->game_state.snakes[player_id].alive;
    game_runs = data->shared->isRunning;
    pthread_mutex_unlock(&data->shared->players_mutex);

    }

  return NULL;
}

void * game_loop(void* arg) {
  client_thread_arg_t* data = (client_thread_arg_t*)arg;
  time_t game_start = time(NULL);

  while(data->shared->isRunning) {
    pthread_mutex_lock(&data->shared->players_mutex);
    int players = data->shared->game_state.active_players;
    time_t last_left = data->shared->game_state.last_player_left;
    game_time_mode_t mode = data->shared->game->mode;
    int limit = data->shared->game->time_limit;
    pthread_mutex_unlock(&data->shared->players_mutex);

    if (mode == GAME_TIMED) {
      if (time(NULL) - game_start >= limit) {
        printf("Cas vyprsal, koncim hru\n");
        break;
      }
    }
    if (mode == GAME_STANDARD) {
      if (players == 0 && last_left != 0 && time(NULL) - last_left >= 10) {
        printf("[GAME] Nikto sa nepripojil 10s, hra konci\n");
        break;
      }
    }
    pthread_mutex_lock(&data->shared->players_mutex);
    pthread_mutex_lock(&data->shared->clients_mutex);
    detect_collisions(data->shared);
    if (data->shared->game_state.snakes->state == PLAYER_ACTIVE) {
      update_snakes(data->shared);
    } 
    generate_fruit(data->shared);
    for (int i = 0; i < data->shared->game_state.active_players; i++) {
      if (data->client_fd[i] > 0 && data->shared->game_state.active_players > 0) {
        send(data->client_fd[i], &data->shared->game_state, sizeof(game_state_t), 0); 
      }

    }
    pthread_mutex_unlock(&data->shared->clients_mutex);
    pthread_mutex_unlock(&data->shared->players_mutex);

    usleep(200000);
  }
  printf("Hra sa skoncila\n");
  pthread_mutex_lock(&data->shared->players_mutex);
  data->shared->isRunning=0;
  pthread_mutex_unlock(&data->shared->players_mutex);
  shutdown(data->server_fd, SHUT_RDWR);
  close(data->server_fd);
  return NULL;
}
