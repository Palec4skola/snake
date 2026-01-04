#include <pthread.h>
#include "../game.h"
#define MAP_W 30
#define MAP_H 10
#define MAX_PLAYERS 4

typedef struct {
    int x, y;
} point_t;

typedef struct {
    point_t body[100];
    int length;
    char direction; // 'w','a','s','d'
    int alive;
} snake_t;

typedef struct {
    char map[MAP_H][MAP_W];
    snake_t snakes[MAX_PLAYERS];
    point_t fruit[MAX_PLAYERS];
    int active_players;
} game_state_t;

typedef struct {
  time_t last_player_left;
  pthread_mutex_t players_mutex;
  pthread_mutex_t clients_mutex;
  int isRunning;

  game_config_t *game;
  game_state_t game_state;
} SHARED_DATA;

typedef struct {
  int client_fd[MAX_PLAYERS];
  SHARED_DATA *shared;
} client_thread_arg_t ;

void init_game_state(SHARED_DATA* data);

void update_snakes(SHARED_DATA* data);

int is_occupied(SHARED_DATA* data, int x, int y);

void generate_fruit(SHARED_DATA* data);

int find_player_id(client_thread_arg_t* arg);

void add_player(game_state_t* game);

void detect_collisions(SHARED_DATA* data);
