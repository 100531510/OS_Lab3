// SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

// To create a queue
queue *queue_init(int size)
{
  queue *q = (queue *)malloc(sizeof(queue));
  q->qSize = 0;
  q->maxSize = size;
  q->arr = (struct element *)malloc(sizeof(struct element) * q->maxSize);
  q->head = 0;
  q->tail = 0;

  pthread_mutex_init(&q->lock, NULL);
  sem_init(&q->empty_slots, 0, size);
  sem_init(&q->filled_slots, 0, 0);

  return q;
}

// To Enqueue an element
int queue_put(queue *q, struct element *x)
{

  if (sem_wait(&q->empty_slots) != 0)
  {
    return -1;
  }

  pthread_mutex_lock(&q->lock);

  if (queue_full(q))
  {
    pthread_mutex_unlock(&q->lock);
    return -1;
  }

  // need to do a deep copy?
  q->arr[q->tail] = *x;
  q->tail = (q->tail + 1) % q->maxSize;
  q->qSize++;

  pthread_mutex_unlock(&q->lock);
  sem_post(&q->filled_slots);
  // return 0 if successfull?
  return 0;
}

// To Dequeue an element.
struct element *queue_get(queue *q)
{
  sem_wait(&q->filled_slots);
  pthread_mutex_lock(&q->lock);

  // use malloc
  struct element *element = (struct element *)malloc(sizeof(struct element));
  if (element == NULL)
  {
    return NULL;
  }

  // might be an issue here (do or don't dereference element?):
  *element = q->arr[q->head];

  q->head = (q->head + 1) % q->maxSize;
  q->qSize--;

  pthread_mutex_unlock(&q->lock);
  sem_post(&q->empty_slots);

  return element;
}

// To check queue state
int queue_empty(queue *q)
{

  return q->qSize == 0;
}

int queue_full(queue *q)
{

  return q->qSize == q->maxSize;
}

// To destroy the queue and free the resources
int queue_destroy(queue *q)
{
  sem_destroy(&q->empty_slots);
  sem_destroy(&q->filled_slots);
  free(q->arr);
  pthread_mutex_destroy(&q->lock);
  free(q);
  return 0;
}
