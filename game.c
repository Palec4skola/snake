#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

void add_player(game_state_t* game) {
  game->active_players++;
  game->fruit[game->active_players-1].x = rand() % MAP_W;
  game->fruit[game->active_players-1].y = rand() % MAP_H;
  
  snake_t * snake = &game->snakes[game->active_players-1];
  snake->time_start = time(NULL);
  snake->points = 0;
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

    point_t* head = &s->body[0];
    // kolizia so stenou
    if(data->game->mode_obs == 1) {
      if (head->x >= MAP_W||head->x < 0||head->y >= MAP_H||head->y < 0) {
        s->alive = 0;
        printf("Hrac narazil do steny\n");
        data->game_state.active_players--;
        data->game_state.last_player_left = time(NULL);
      }

    } else if (data->game->mode_obs == 2) {
        if (head->x < 0) {
          head->x = MAP_W - 1;
        }
        if (head->x >= MAP_W) {
          head->x = 0;
        }
        if (head->y < 0) {
          head->y = MAP_H - 1;
        }
        if (head->y >= MAP_H) {
          head->y = 0;
        }
    }
        // kolízia so sebou 
    for (int j = 1; j < s->length; j++) {
      if (head->x == s->body[j].x &&
        head->y == s->body[j].y) {
        s->alive = 0;
        printf("Hrac narazil do seba\n");
        data->game_state.active_players--;
        data->game_state.last_player_left = time(NULL);
      }
    }

    // kolízia s iným hadom
    for (int k = 0; k < data->game_state.active_players; k++) {
      if (k == i) continue;

      snake_t *other = &data->game_state.snakes[k];
      if (!other->alive) continue;

      for (int j = 0; j < other->length; j++) {
        if (head->x == other->body[j].x &&
            head->y == other->body[j].y) {
            s->alive = 0;
            printf("Hrac narazil do hraca\n");
            data->game_state.active_players--;
            data->game_state.last_player_left = time(NULL); 
        }
      }
    }

    // kolízia s ovocím
    point_t *f = &data->game_state.fruit[i];
    if (head->x == f->x && head->y == f->y) {
        s->length++;
        f->x = -1; // znovu vygeneruj
        s->points++;
        printf("Hrac zjedol ovocie\n");
    }
  }
}
