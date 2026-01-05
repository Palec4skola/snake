#ifndef GAME_H
#define GAME_H

#include "shared.h"

void update_snakes(SHARED_DATA *data);

int is_occupied(SHARED_DATA *data, int x, int y);

void generate_fruit(SHARED_DATA * data);

void init_game_state(SHARED_DATA* data);

void add_player(game_state_t* game);

void detect_collisions(SHARED_DATA *data);

#endif

