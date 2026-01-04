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

#define MAX_PLAYERS 4
#define PORT 9998
#define BUF_SIZE 256

void * client_loop(void* arg){
  client_thread_arg_t* data = (client_thread_arg_t*)arg;
 // SHARED_DATA *shared = data->shared;
    //printf("arg CLIENT: %p\n", data);
  int player_id = data->shared->game_state.active_players-1;
  int client_fd = data->client_fd[player_id];
  char key;
 while (data->shared->game_state.snakes[player_id].alive) {
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
    }

  return NULL;
}

void * game_loop(void* arg) {
  client_thread_arg_t* data = (client_thread_arg_t*)arg;
  time_t game_start = time(NULL);

  while(data->shared->isRunning) {
    pthread_mutex_lock(&data->shared->players_mutex);
    int players = data->shared->game_state.active_players;
    time_t last_left = data->shared->last_player_left;
    game_mode_t mode = data->shared->game->mode;
    int limit = data->shared->game->time_limit;
    pthread_mutex_unlock(&data->shared->players_mutex);

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
    pthread_mutex_lock(&data->shared->players_mutex);
    pthread_mutex_lock(&data->shared->clients_mutex);
    update_snakes(data->shared);
    generate_fruit(data->shared);
    detect_collisions(data->shared);
    for (int i = 0; i < data->shared->game_state.active_players; i++) {
      if (data->client_fd[i] > 0 && data->shared->game_state.active_players > 0) {
        send(data->client_fd[i], &data->shared->game_state, sizeof(game_state_t), 0); 
      }

    }
    pthread_mutex_unlock(&data->shared->clients_mutex);
    pthread_mutex_unlock(&data->shared->players_mutex);

    usleep(200000);
  }
  data->shared->isRunning = 0;
  return NULL;
}

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

  SHARED_DATA *data;
  data = malloc(sizeof(SHARED_DATA)); 
  client_thread_arg_t *arg; 
  arg = malloc(sizeof(client_thread_arg_t));

  data->game_state.active_players = 0;
  data->last_player_left = 0;
  data->isRunning =1;
  arg->shared = data;
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

  
  while (data->isRunning) {
    printf("Caka na klienta\n");
    usleep(200000);
    pthread_t client_thread;
    
  int newSocket;//caka, kym sa pripoji klient
    newSocket = accept(server_fd, (struct sockaddr*)&newAddr,&addrSize);
    printf("%d\n",newSocket);
      
    if (!arg) {
      perror("malloc");
      continue;
    }
    pthread_mutex_lock(&data->players_mutex);
    arg->client_fd[data->game_state.active_players] = newSocket;
    if(arg->client_fd[data->game_state.active_players] < 0) {
      perror("accept");
      free(arg);
      continue;
    }
    add_player(&arg->shared->game_state);
    pthread_mutex_unlock(&data->players_mutex);
    pthread_create(&client_thread, NULL, client_loop, arg);
    pthread_detach(client_thread);
    //free(arg);
  }
  pthread_mutex_destroy(&data->players_mutex);
  pthread_mutex_destroy(&data->clients_mutex);
  printf("Koniec\n");
  free(data);
}

void update_snakes(SHARED_DATA *data) {
  
  for (int i = 0; i < data->game_state.active_players ; i++) {
    snake_t *snake = &data->game_state.snakes[i];

    if (!snake->alive || snake->length == 0) {
      continue;
    }
    for (int j = snake->length; j > 0; j--) {
      snake->body[j] = snake->body[j-1];
    }
    point_t *head = &snake->body[0];
    switch (snake[0].direction) {
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
  }
}

int is_occupied(SHARED_DATA *data, int x, int y) {
  for (int i = 0; i < data->game_state.active_players; i++) {
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
  for (int i = 0; i < data->game_state.active_players; i++) {
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
}

void init_game_state(SHARED_DATA* data) {
  game_state_t * game = &data->game_state;

  memset(game->map, ' ', sizeof(game->map));
}

void add_player(game_state_t* game) {
  game->active_players++;
  game->fruit[game->active_players-1].x = rand() % MAP_W;
  game->fruit[game->active_players-1].y = rand() % MAP_H;
  
  snake_t * snake = &game->snakes[game->active_players-1];
  snake->alive = 1;
  snake->length = 3;
  snake->direction = 'd';

  for (int j = 0; j < 100; j++) {
      snake->body[j].x = -1;
      snake->body[j].y = -1;
    }

  snake->body[0].x = 3;
  snake->body[0].y = 3;
  snake->body[1].x = 2;
  snake->body[1].y = 3;
  snake->body[2].x = 1;
  snake->body[2].y = 3;
}

void detect_collisions(SHARED_DATA *data) {
    for (int i = 0; i < data->game_state.active_players; i++) {
        snake_t *s = &data->game_state.snakes[i];
        if (!s->alive) continue;

        point_t head = s->body[0];
        // kolizia so stenou
        
         if (head.x >= MAP_W) {
          s->alive = 0;
          printf("Hrac %d narazil do steny\n",i);
          data->game_state.active_players--;
        }
        if (head.x < 0) {
          s->alive = 0;
          printf("Hrac %d narazil do steny\n",i);
          data->game_state.active_players--;
        }
        if (head.y >= MAP_H) {
          s->alive = 0;
          printf("Hrac %d narazil do steny\n",i);
          data->game_state.active_players--;
        }
        if (head.y < 0) {
          s->alive = 0;
          printf("Hrac %d narazil do steny\n",i);
          data->game_state.active_players--;
        }
        // kolízia so sebou 
        for (int j = 1; j < s->length; j++) {
            if (head.x == s->body[j].x &&
                head.y == s->body[j].y) {
                s->alive = 0;
                printf("Hrac %d narazil do seba\n", i);
                data->game_state.active_players--;
            }
        }

        // kolízia s iným hadom
        for (int k = 0; k < data->game_state.active_players; k++) {
            if (k == i) continue;

            snake_t *other = &data->game_state.snakes[k];
            if (!other->alive) continue;

            for (int j = 0; j < other->length; j++) {
                if (head.x == other->body[j].x &&
                    head.y == other->body[j].y) {
                    s->alive = 0;
                    printf("Hrac %d narazil do hraca %d\n", i, k);
                    data->game_state.active_players--;
                    
                }
            }
        }

        // kolízia s ovocím
        point_t *f = &data->game_state.fruit[i];
        if (head.x == f->x && head.y == f->y) {
            s->length++;
            f->x = -1; // znovu vygeneruj
            printf("Hrac %d zjedol ovocie\n", i);
        }
    }
}
