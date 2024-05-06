// SSOO-P3 23/24
#include <pthread.h>
#include <semaphore.h>

#ifndef HEADER_FILE
#define HEADER_FILE

struct element
{
  int product_id; // Product identifier
  int op;         // Operation (1 = PURCHASE, 2 = SALE)
  int units;      // Product units
};

typedef struct queue
{
  // Define the struct yourself

  // queue has a current size
  int qSize;

  // queue has a max size
  int maxSize;

  // queue holds an array of elements
  struct element *arr;

  // queue has a head pointer
  int head;

  // queue has tail pointer
  int tail;

  sem_t empty_slots;
  sem_t filled_slots;
  pthread_mutex_t lock;

} queue;

queue *queue_init(int size);
int queue_destroy(queue *q);
int queue_put(queue *q, struct element *elem);
struct element *queue_get(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);

#endif
