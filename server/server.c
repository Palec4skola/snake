#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../game.h"

#define PORT 9999
#define BUF_SIZE 256
typedef struct {
  int active_players;
  time_t last_player_left;
  pthread_mutex_t* players_mutex;
  int isRunning;

  game_config_t *game;
} SHARED_DATA;

typedef struct {
  int client_fd;
  SHARED_DATA *shared;
} client_thread_arg_t ;

void* client_loop(void* arg){
  client_thread_arg_t *data= (client_thread_arg_t*) arg;
  printf("Vlakno klienta\n");
  int client_fd = data->client_fd;
  printf("Dalej vo vlakne\n");
  pthread_mutex_lock(data->shared->players_mutex);
  printf("Haloooo???\n"); 
  data->shared->active_players++;
  printf("Klient pripojeny (fd=%d)\n",client_fd);
  pthread_mutex_unlock(data->shared->players_mutex);
  //char buf[BUF_SIZE];
  //int n;

 // while ((n = read(client_fd, buf, sizeof(buf))) > 0) {
 //   write(client_fd,buf, n);
 // }
  printf("Ahoj\n");
  close(client_fd);

  pthread_mutex_lock(data->shared->players_mutex);
  data->shared->active_players--;
  printf("Klient odpojeny, zostava aktivnych hracov: %d\n", data->shared->active_players);
  pthread_mutex_unlock(data->shared->players_mutex);
  return NULL;
}

void * game_loop(void* arg) {
  SHARED_DATA* data = (SHARED_DATA*)arg;
  time_t game_start = time(NULL);

  while(1) {
    sleep(2);
    pthread_mutex_lock(data->players_mutex);
    int players = data->active_players;
    time_t last_left = data->last_player_left;
    game_mode_t mode = data->game->mode;
    int limit = data->game->time_limit;
    pthread_mutex_unlock(data->players_mutex);
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
    printf("isRunning: %d\n", data->isRunning);
  }
  printf("isRunning: %d\n", data->isRunning);
  data->isRunning = 0;
  return NULL;
}

int main(int argc, char * argv[]) {
  int server_fd;
  struct sockaddr_in addr;
  //socklen_t addrlen = sizeof(addr);
  int newSocket;
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
  //strcpy(buf, "sprava od servera\n");
  //send(newSocket, buf, strlen(buf), 0);
  //printf("Server bezi, cakam na porte %d\n",PORT);

  pthread_t game_thread;
  game_config_t game;

  game.mode = atoi(argv[1]);
  printf("%d\n",game.mode);
  game.time_limit = atoi(argv[2]);
  printf("%d\n",game.time_limit);
  SHARED_DATA data;
  data.active_players = 0;
  data.last_player_left = 0;
  data.isRunning =1;

  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);
  data.players_mutex = &mutex;
  data.game = &game;
  pthread_create(&game_thread, NULL, game_loop, &data);
  pthread_detach(game_thread);  
  printf("Server spusteny na porte %d\n", PORT);
  printf("Herny rezim: %s\n",
      game.mode == GAME_STANDARD ? "STANDARDNY" : "CASOVY");

  if (game.mode == GAME_TIMED) {
      printf("Casovy limit: %d sekund\n", game.time_limit);
  }

  
  while (data.isRunning) {
    sleep(1);
    printf("cakam na klienta\n");
    client_thread_arg_t *arg = malloc(sizeof(client_thread_arg_t));
   
    if (!arg) {
      perror("malloc");
      continue;
    }
    printf("Is Running\n");
    newSocket = accept(server_fd, (struct sockaddr*)&newAddr,&addrSize);
    arg->client_fd = newSocket;
    strcpy(buf, "sprava od servera\n");
    send(newSocket, buf, strlen(buf), 0);
    printf("Neviem\n");
    arg->shared = &data;
    
    if(arg->client_fd < 0) {
      perror("accept");
      free(arg);
      continue;
    }

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, client_loop, &arg);
    pthread_detach(client_thread);
    free(arg);
  }
  pthread_mutex_destroy(&mutex);
  printf("Koniec\n");
}

