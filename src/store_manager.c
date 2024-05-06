// SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdbool.h>

/*
pthread_mutex_t mutex;
pthread_cond_t not_full;
pthread_cond_t not_empty;
*/

int purchaseCosts[5] = {2, 5, 15, 25, 100};
int salesPrices[5] = {3, 10, 20, 40, 125};

typedef struct
{
  int *profits;
  int *product_stock;
  queue *buffer;
  struct element *operations;
  int num_operations;

  // other info to evenly distribute ops across producers

} associated_data;

typedef struct
{
  int active_producers;
  pthread_mutex_t lock;
  pthread_cond_t alldone;
} production_control;

production_control prod_ctrl = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

// produce function
void *producer(void *args)
{

  pthread_mutex_lock(&prod_ctrl.lock);
  prod_ctrl.active_producers++;
  pthread_mutex_unlock(&prod_ctrl.lock);

  associated_data *data = (associated_data *)args;
  struct element curOp;
  //  CURRENTLY ONLY FOR ONE PRODUCER. NEED TO CHANGE FOR MULTIPLE PRODUCERS
  for (int i = 0; i < data->num_operations; ++i)
  {
    // store operation
    curOp = data->operations[i];

    queue_put(data->buffer, &curOp);
  }

  pthread_mutex_lock(&prod_ctrl.lock);
  prod_ctrl.active_producers--;
  if (prod_ctrl.active_producers == 0)
  {
    pthread_cond_broadcast(&prod_ctrl.alldone);
  }
  pthread_mutex_unlock(&prod_ctrl.lock);

  pthread_exit(0);
}

// consume function
void *consumer(void *args)
{
  associated_data *data = (associated_data *)args;
  struct element curOp;

  while (true)
  {

    pthread_mutex_lock(&prod_ctrl.lock);
    printf("Consumer waiting: active_producers=%d, buffer_empty=%d\n", prod_ctrl.active_producers, queue_empty(data->buffer));

    while (prod_ctrl.active_producers > 0 && queue_empty(data->buffer))
    {
      pthread_cond_wait(&prod_ctrl.alldone, &prod_ctrl.lock);
      printf("Consumer waiting INSIDE COND: active_producers=%d, buffer_empty=%d\n", prod_ctrl.active_producers, queue_empty(data->buffer));
    }
    if (prod_ctrl.active_producers == 0 && queue_empty(data->buffer))
    {
      pthread_mutex_unlock(&prod_ctrl.lock);
      break;
    }
    pthread_mutex_unlock(&prod_ctrl.lock);
    // once there is an element we can extract it
    curOp = *(queue_get(data->buffer));

    printf("Consuming: Product ID=%d, Operation=%d, Units=%d\n", curOp.product_id, curOp.op, curOp.units);

    // compute profit and stock
    if (curOp.op == 1)
    {
      // PURCHASE
      *data->profits -= (purchaseCosts[curOp.product_id - 1] * curOp.units);
      data->product_stock[curOp.product_id - 1] += curOp.units;
    }
    else
    {
      // SALE
      *data->profits += (salesPrices[curOp.product_id - 1] * curOp.units);
      data->product_stock[curOp.product_id - 1] -= curOp.units;
    }
  }

  printf("Consumer exiting\n");
  pthread_exit(0);
}

int main(int argc, const char *argv[])
{

  // ERROR CHECK FOR NEGATIVE VALUES

  int profits = 0;
  int product_stock[5] = {0};

  if (argc != 5)
  {
    printf("Error, too few arguments\n");
    return -1;
  }

  const char *file_name = argv[1];

  // ONLY 1 PRODUCER AND 1 CONSUMER FOR NOW
  int numProducers = atoi(argv[2]);
  int numConsumers = atoi(argv[3]);

  if (numProducers < 1 || numConsumers < 1)
  {
    fprintf(stderr, "Error, number of producers and consumers must be greater than 0\n");
    return -1;
  }

  pthread_t *producers = (pthread_t *)malloc(sizeof(pthread_t) * numProducers);
  pthread_t *consumers = (pthread_t *)malloc(sizeof(pthread_t) * numConsumers);

  if (!producers || !consumers)
  {
    fprintf(stderr, "Error, memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  associated_data *producerData = (associated_data *)malloc(sizeof(associated_data) * numProducers);
  associated_data *consumerData = (associated_data *)malloc(sizeof(associated_data) * numConsumers);
  if (!producerData || !consumerData)
  {
    fprintf(stderr, "Error, memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  queue *buffer = queue_init(atoi(argv[4]));

  FILE *file = fopen(file_name, "r");
  if (file == NULL)
  {
    perror("Couldn't open file");
    return -1;
  }

  // use scanf to load file

  // first value in file is number of operations
  int num_operations;
  fscanf(file, "%d", &num_operations);

  // use malloc to allocate enought memory for all of those operations
  struct element *operations = (struct element *)malloc(sizeof(struct element) * num_operations);
  // ERROR CHECK TO SEE IF MEMORY WAS ALLOCATED CORRECTLY?

  // loop through each operation, store the attributes in an element object, and store the element
  // in the operations array
  char op[10];
  for (int i = 0; i < num_operations; ++i)
  {
    fscanf(file, "%d %s %d", &operations[i].product_id, op, &operations[i].units);
    if (strcmp(op, "PURCHASE") == 0)
    {
      operations[i].op = 1;
    }
    else
    {
      operations[i].op = 2;
    }
  }

  // distribute operations among producer threads
  // pass parameters to threads in pthread_create
  // use a structure to pass parameters to threads
  // distribute evenly by splitting operations over n producers?
  // producer 0 goes from element 0 to element numOperations/N in operations array
  // producer 1 goes over the next numOperations/N elements... etc

  int operationsPerProducer = num_operations / numProducers;
  int remainingOperations = num_operations % numProducers;
  int start = 0;

  for (int i = 0; i < numProducers; ++i)
  {
    producerData[i] = (associated_data){
        .profits = &profits,
        .product_stock = product_stock,
        .buffer = buffer,
        .operations = operations + start,
        .num_operations = operationsPerProducer + (i < remainingOperations ? 1 : 0),

    };

    start += producerData[i].num_operations;
    pthread_create(&producers[i], NULL, producer, &producerData[i]);
  }

  for (int i = 0; i < numConsumers; ++i)
  {
    consumerData[i] = (associated_data){
        .profits = &profits,
        .product_stock = product_stock,
        .buffer = buffer,
        .num_operations = num_operations};

    pthread_create(&consumers[i], NULL, consumer, &consumerData[i]);
  }

  for (int i = 0; i < numProducers; ++i)
  {
    pthread_join(producers[i], NULL);
  }

  for (int i = 0; i < numConsumers; ++i)
  {
    pthread_join(consumers[i], NULL);
  }

  // free memory
  free(operations);
  free(producers);
  free(consumers);
  free(producerData);
  free(consumerData);

  // close file
  fclose(file);

  /*
  associated_data data;
  data.profits = &profits;
  data.product_stock = product_stock;
  data.buffer = buffer;
  data.operations = operations;
  data.num_operations = num_operations;

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&not_empty, NULL);
  pthread_cond_init(&not_full, NULL);

 // START WITH JUST ONE CONSUMER AND ONE PRODUCER
  pthread_t th1, th2;

  pthread_create(&th1,NULL,producer, &data);
  pthread_create(&th2,NULL,consumer, &data);

  pthread_join(th1,NULL);
  pthread_join(th2,NULL);

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&not_empty);
  pthread_cond_destroy(&not_full);

  // free memeory
  free(operations);

  // close file
  fclose(file);
  */

  // Output
  printf("Total: %d euros\n", profits);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  return 0;
}
