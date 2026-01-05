#ifndef SHARED_H
#define SHARED_H

#include <time.h>
#include <pthread.h>
#define PORT 9997
#define MAP_W 30
#define MAP_H 10
#define MAX_PLAYERS 4

typedef enum {
  GAME_STANDARD = 1,
  GAME_TIMED = 2
} game_time_mode_t;

typedef struct {
  game_time_mode_t mode;
  int mode_obs;
  int time_limit; 
} game_config_t;

typedef struct {
  int x, y;
} point_t;

typedef enum {
  PLAYER_ACTIVE,
  PLAYER_PAUSED,
  PLAYER_LEFT
} player_state_t;

typedef struct {
  point_t body[100];
  int length;
  char direction; 
  int alive;
  player_state_t state;
  time_t time_start; 
  int points;
} snake_t;

typedef struct {
  char map[MAP_H][MAP_W];
  snake_t snakes[MAX_PLAYERS];
  point_t fruit[MAX_PLAYERS];
  int active_players;
  time_t last_player_left;
} game_state_t;

typedef struct {
  pthread_mutex_t players_mutex;
  pthread_mutex_t clients_mutex;
  int isRunning;

  game_config_t *game;
  game_state_t game_state;
} SHARED_DATA;

typedef struct {
  int server_fd;
  int client_fd[MAX_PLAYERS];
  SHARED_DATA *shared;
  time_t last_player_left;
} client_thread_arg_t ;

#endif 
