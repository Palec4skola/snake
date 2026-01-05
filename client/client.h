#include <pthread.h>
typedef struct {
  int client_fd;
  int game_running;
  pthread_mutex_t mutex;
} client_t ;
