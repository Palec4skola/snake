#include <pthread.h>
#include "../shared.h"

typedef enum {
  MSG_MOVE,
  MSG_PAUSE,
  MSG_RESUME,
  MSG_QUIT
} msg_type_t;

void init_game_state(SHARED_DATA* data);

void update_snakes(SHARED_DATA* data);

int is_occupied(SHARED_DATA* data, int x, int y);

void generate_fruit(SHARED_DATA* data);

int find_player_id(client_thread_arg_t* arg);

void add_player(game_state_t* game);

void detect_collisions(SHARED_DATA* data);

void* client_loop(void* arg);

void* game_loop(void* arg);
