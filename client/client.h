#include <pthread.h>
#include "../server/server.h"
typedef struct {
  int client_fd;
  int game_running;
  pthread_mutex_t mutex;
} client_t ;

typedef struct {
    msg_type_t type;
    char key; 
} client_msg_t;

void show_menu();
