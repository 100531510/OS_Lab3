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
  pthread_cond_init(&q->not_empty, NULL);
  pthread_cond_init(&q->not_full, NULL);

  return q;
}

// To Enqueue an element
int queue_put(queue *q, struct element *x)
{

  pthread_mutex_lock(&q->lock);
  // wait if queue is full (needs to be implemented)
  while (q->qSize == q->maxSize)
  {
    pthread_cond_wait(&q->not_full, &q->lock);
  }

  // need to do a deep copy?
  q->arr[q->tail] = *x;
  q->tail = (q->tail + 1) % q->maxSize;
  q->qSize++;

  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);

  // return 0 if successfull?
  return 0;
}

// To Dequeue an element.
struct element *queue_get(queue *q)
{
  struct element *element = NULL;

  // wait if queue is empty (needs to be implemented)
  pthread_mutex_lock(&q->lock);
  while (q->qSize == 0)
  {
    pthread_cond_wait(&q->not_empty, &q->lock);
  }

  // return the element but also dequeue it?

  // use malloc
  element = (struct element *)malloc(sizeof(struct element));

  // might be an issue here (do or don't dereference element?):
  *element = q->arr[q->head];

  q->head = (q->head + 1) % q->maxSize;
  q->qSize--;

  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);

  return element;
}

// To check queue state
int queue_empty(queue *q)
{
  // return 1 if queue is empty
  if (q->qSize == 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int queue_full(queue *q)
{
  // return 1 if queue is full
  if (q->qSize == q->maxSize)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// To destroy the queue and free the resources
int queue_destroy(queue *q)
{
  free(q->arr);
  pthread_mutex_destroy(&q->lock);
  pthread_cond_destroy(&q->not_empty);
  pthread_cond_destroy(&q->not_full);
  free(q);
  return 0;
}
