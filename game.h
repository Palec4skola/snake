#ifndef GAME_H
#define GAME_H

typedef enum {
    GAME_STANDARD = 1,
    GAME_TIMED = 2
} game_mode_t;

typedef struct {
    game_mode_t mode;
    int time_limit;   // len pre GAME_TIMED (v sekundách)
} game_config_t;

#endif

