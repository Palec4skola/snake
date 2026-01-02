#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "server.h"

#define MAP_W 20
#define MAP_H 10
#define MAX_PLAYERS 4
#define PORT 9998
#define BUF_SIZE 256

void * client_loop(void* arg){
  client_thread_arg_t* data = (client_thread_arg_t*)arg;
 // SHARED_DATA *shared = data->shared;
    //printf("arg CLIENT: %p\n", data);
 
  int client_fd = data->client_fd;

  pthread_mutex_lock(&data->shared->players_mutex);
  data->shared->active_players++;
  printf("Klient pripojeny (fd=%d)\n",client_fd);
  pthread_mutex_unlock(&data->shared->players_mutex);
  while (1) {
    game_state_t snapshot;
    pthread_mutex_lock(&data->shared->players_mutex);
    snapshot = data->shared->game_state;
    pthread_mutex_unlock(&data->shared->players_mutex);
    //send(client_fd, &snapshot, sizeof(game_state_t),0); 
    //char input;
    //printf("caka na input\n");
    //recv(client_fd, &input,1,0);
    //printf("input %s\n",&input);
    //handle_input(player_id, input);
    sleep(1);
  }
  return NULL;
}

void * game_loop(void* arg) {
  SHARED_DATA* data = (SHARED_DATA*)arg;
  time_t game_start = time(NULL);

  while(data->isRunning) {
    pthread_mutex_lock(&data->players_mutex);
    int players = data->active_players;
    time_t last_left = data->last_player_left;
    game_mode_t mode = data->game->mode;
    int limit = data->game->time_limit;
    pthread_mutex_unlock(&data->players_mutex);

    if (mode == GAME_TIMED) {
      if (time(NULL) - game_start >= limit) {
        printf("Cas vyprsal, koncim hru\n");
        exit(0);
      }
    }
    if (mode == GAME_STANDARD) {
      if (players == 0 && last_left != 0 && time(NULL) - last_left >= 10) {
        printf("[GAME] Nikto sa nepripojil 10s, hra konci\n");
        exit(0);
      }
    }

    update_snakes(data);
    generate_fruit(data);
    sleep(2);
  }
  data->isRunning = 0;
  return NULL;
}

int main(int argc, char * argv[]) {
  int server_fd;
  struct sockaddr_in addr;
  //socklen_t addrlen = sizeof(addr);
  struct sockaddr_in newAddr;

  socklen_t addrSize;
  char buf[BUF_SIZE];
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

  SHARED_DATA *data;
  data = malloc(sizeof(SHARED_DATA));
  data->active_players = 0;
  data->last_player_left = 0;
  data->isRunning =1;
  pthread_mutex_init(&data->players_mutex, NULL);
  data->game = &game;
  init_game_state(data);
  pthread_create(&game_thread, NULL, game_loop, data);
  pthread_detach(game_thread);  

  printf("Server spusteny na porte %d\n", PORT);
  printf("Herny rezim: %s\n",
      game.mode == GAME_STANDARD ? "STANDARDNY" : "CASOVY");

  if (game.mode == GAME_TIMED) {
      printf("Casovy limit: %d sekund\n", game.time_limit);
  }

  
  while (data->isRunning) {
    sleep(1);
    pthread_t client_thread;
    
  int newSocket;
    newSocket = accept(server_fd, (struct sockaddr*)&newAddr,&addrSize);

    client_thread_arg_t *arg;
    arg = malloc(sizeof(client_thread_arg_t));
   
    if (!arg) {
      perror("malloc");
      continue;
    }
    arg->client_fd = newSocket;
    strcpy(buf, "sprava od servera\n");
    send(newSocket, buf, strlen(buf), 0);
    arg->shared = data;
    
    if(arg->client_fd < 0) {
      perror("accept");
      free(arg);
      continue;
    }
    pthread_create(&client_thread, NULL, client_loop, arg);
    pthread_detach(client_thread);
    //free(arg);
  }
  pthread_mutex_destroy(&data->players_mutex);
  printf("Koniec\n");
  free(data);
}

void update_snakes(SHARED_DATA *data) {
  printf("Update snake\n");
  for (int i = 0; i < data->active_players ; i++) {
    snake_t *snake = &data->game_state.snakes[i];

    if (!snake->alive || snake->length == 0) {
      continue;
    }
    for (int j = snake->length; j > 0; j--) {
      snake->body[j] = snake->body[j-1];
    }
    point_t *head = &snake->body[0];

    switch (snake->direction) {
      case 'w':
        head->y--;
        break;
      case 's':
        head->y++;
        break;
      case 'a':
        head->x--;
        break;
      case 'd':
        head->x++;
        break;
    }
    if (head->x < 0) {
      head->x = MAP_W -1;
    }
    if (head->x >= MAP_W) {
      head->x = 0;
    }
    if (head->y < 0) {
      head->y = MAP_H -1;
    }
    if (head->y >= MAP_H) {
      head->y = 0;
    }
  }
}

int is_occupied(SHARED_DATA *data, int x, int y) {
  for (int i = 0; i < data->active_players; i++) {
    snake_t *snake = &data->game_state.snakes[i];
    if (snake->alive) {
      continue;
    }

    for (int j = 0; j < snake->length; j++) {
      if (snake->body[j].x == x && snake->body[j].y == y) {
      return 1;
      }
    }
  }
  return 0;
}

void generate_fruit(SHARED_DATA * data) {
  for (int i = 0; i < data->active_players; i++) {
    point_t * fruit = &data->game_state.fruit[i];

    if (fruit->x != -1) {
      continue; //uz existuje
    }
    int x,y;
    do {
      x = rand() % MAP_W;
      y = rand() % MAP_H;
    } while (is_occupied(data, x, y));

    fruit->x = x;
    fruit->y = y;
  }
    printf("Ovocko\n");
}

void init_game_state(SHARED_DATA* data) {
  game_state_t * game = &data->game_state;

  memset(game->map, ' ', sizeof(game->map));

  for (int i = 0; i < data->active_players; i++) {
    snake_t * snake = &game->snakes[i];

    snake->length = 3;
    snake->alive = 0;
    snake->direction = 'd';

    for (int j = 0; j < 100; j++) {
      snake->body[j].x = -1;
      snake->body[j].y = -1;
    }
    game->fruit->x = -1;
    game->fruit->y = -1;
  }
}

