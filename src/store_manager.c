//SSOO-P3 23/24

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

pthread_mutex_t mutex;
pthread_cond_t not_full;
pthread_cond_t not_empty;

int purchaseCosts[5] = {2,5,15,25,100};
int salesPrices[5] = {3,10,20,40,125};

typedef struct {
  int* profits;
  int* product_stock;
  queue* buffer;
  struct element* operations;
  int num_operations;
  int num_producers;
  int num_consumers;
  int ops_per_producer;
  int ops_per_consumer;
  int op;
  int extra_consumers;
  int extra_producers;
  // other info to evenly distribute ops across producers

} associated_data;


// produce function
void* producer(void* args) {
  associated_data* data = (associated_data*)args;
  struct element curOp;
  char finalMsg[2028] = "";

  //  Compute the quantity of operations that need to be managed.
  /* 
  Each producer is producing one more operation so that no operations are left out in the remainder.
  Every time a producer does an operation it'll check that the num_operations is not exceeded, that check 
  needs to be done with a mutex so that no other producer can do the same operation at the same time.
  */

  for (int i = 0; i < (data -> ops_per_producer); ++i) {
    char tempMsg[1024]; // Temporal message to store the sprintf
    sprintf(tempMsg,"Producer entering loop for the %d time \n",i);
    strcat(finalMsg,tempMsg);
    // lock buffer mutex
    pthread_mutex_lock(&mutex);
    //Save the current operation in a variable
    curOp = data->operations[data->op];
    sprintf(tempMsg,"curOp: %d %d %d\n", curOp.product_id, curOp.op, curOp.units);
    strcat(finalMsg,tempMsg);
    while(queue_full(data->buffer)){
      // wait if buffer is full (no space for us to add an operation)
      pthread_cond_wait(&not_full, &mutex);
    }

    // re-acquired mutex and checked condition again
    queue_put(data->buffer,&curOp);
    data->op++;
    // signal that the buffer is not empty
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);

  }
  
  pthread_mutex_lock(&mutex);
  if(data->extra_producers > 0){

    for(int i = 0; i < data->extra_producers; i++)
    {
      char tempMsg[1024]; // Temporal message to store the sprintf
      sprintf(tempMsg,"Producer entering extra for the extra %d time \n",data->extra_producers);
      strcat(finalMsg,tempMsg);
      //Save the current operation in a variable
      curOp = data->operations[data->op];
      sprintf(tempMsg,"curOp: %d %d %d\n", curOp.product_id, curOp.op, curOp.units);
      strcat(finalMsg,tempMsg);
      while(queue_full(data->buffer)){
        // wait if buffer is full (no space for us to add an operation)
        pthread_cond_wait(&not_full, &mutex);
      }

      // re-acquired mutex and checked condition again
      queue_put(data->buffer,&curOp);
      data->op++;
      // signal that the buffer is not empty
      pthread_cond_signal(&not_empty);
      
    }
    

  }
  pthread_mutex_unlock(&mutex);

  
  strcat(finalMsg,"Producer exiting loop\n");
  strcat(finalMsg,"Producer exiting thread\n");
  printf("%s",finalMsg);
  pthread_exit(0);
}

// consume function
void* consumer(void* args) {
  associated_data* data = (associated_data*)args;
  struct element curOp;

  for (int i = 0; i < (data->ops_per_consumer); ++i) {
    printf("Consumer entering loop\n");
    // lock the mutex on the buffer
    pthread_mutex_lock(&mutex);
    while(queue_empty(data->buffer)) {
      // while the queue is empty wait (we cant retrieve any operatiosn)
      pthread_cond_wait(&not_empty, &mutex);
    }
    // once there is an element we can extract it
    curOp = *(queue_get(data->buffer));

    // now signal that the queue is not full
    pthread_cond_signal(&not_full);
    

    // compute profit and stock
    if (curOp.op == 1) {
      // PURCHASE
      *data->profits -= (purchaseCosts[curOp.product_id - 1] * curOp.units);
      data->product_stock[curOp.product_id - 1] += curOp.units; 
    } else {
      // SALE
      *data->profits += (salesPrices[curOp.product_id - 1] * curOp.units);
      data->product_stock[curOp.product_id - 1] -= curOp.units; 
      
    }
    pthread_mutex_unlock(&mutex);
  }

  pthread_mutex_lock(&mutex);
  if(data->extra_consumers > 0){
    for(int i = 0; i < data->extra_consumers; i++){
    printf("Consumer entering EXTRA LOOP\n");
    while(queue_empty(data->buffer)) {
      // while the queue is empty wait (we cant retrieve any operatiosn)
      pthread_cond_wait(&not_empty, &mutex);
    }
    // once there is an element we can extract it
    curOp = *(queue_get(data->buffer));
    // now signal that the queue is not full
    pthread_cond_signal(&not_full);

    // compute profit and stock
    if (curOp.op == 1) {
      // PURCHASE
      *data->profits -= (purchaseCosts[curOp.product_id - 1] * curOp.units);
      data->product_stock[curOp.product_id - 1] += curOp.units; 
    } else {
      // SALE
      *data->profits += (salesPrices[curOp.product_id - 1] * curOp.units);
      data->product_stock[curOp.product_id - 1] -= curOp.units; 
    }
    }
  }
  pthread_mutex_unlock(&mutex);
  
  pthread_exit(0);
}


int main (int argc, const char * argv[])
{

  // ERROR CHECK FOR NEGATIVE VALUES

  int profits = 0;
  int product_stock [5] = {0};

  if (argc != 5) {
    printf("Error, too few arguments\n");
    return -1;
  }

  const char* file_name = argv[1];

  // Specify the number of producer and consumer threads
  int numProducers = atoi(argv[2]);
  int numConsumers = atoi(argv[3]);
  queue* buffer = queue_init(atoi(argv[4]));

  FILE* file = fopen(file_name, "r");
  if (file == NULL) {
    perror("Couldn't open file");
    return -1;
  }

  // use scanf to load file

  // first value in file is number of operations
  int num_operations;
  fscanf(file, "%d", &num_operations);

  // use malloc to allocate enought memory for all of those operations
  struct element* operations = (struct element*)malloc(sizeof(struct element)*num_operations);
  // ERROR CHECK TO SEE IF MEMORY WAS ALLOCATED CORRECTLY?

  // loop through each operation, store the attributes in an element object, and store the element
  // in the operations array
  char op[10];
  for (int i = 0; i < num_operations; ++i) {
    fscanf(file, "%d %s %d", &operations[i].product_id, op, &operations[i].units);
    if (strcmp(op, "PURCHASE") == 0) {
      operations[i].op = 1;
    } else {
      operations[i].op = 2;
    }
  }



  // distribute operations among producer threads
    // pass parameters to threads in pthread_create
    // use a structure to pass parameters to threads
    // distribute evenly by splitting operations over n producers?
      // producer 0 goes from element 0 to element numOperations/N in operations array
      // producer 1 goes over the next numOperations/N elements... etc


  associated_data data;
  data.profits = &profits;
  data.product_stock = product_stock;
  data.buffer = buffer;
  data.operations = operations;
  data.num_operations = num_operations;
  data.num_consumers = numConsumers;
  data.num_producers = numProducers;
  data.ops_per_producer = (num_operations / numProducers) ;
  data.ops_per_consumer = (num_operations / numConsumers) ;
  data.op = 0;
  if (num_operations % numConsumers != 0) {
    data.extra_consumers = num_operations % numConsumers;
  } else {
    data.extra_consumers = 0;
  }
  if (num_operations % numProducers != 0) {
    data.extra_producers = num_operations % numProducers;
  } else {
    data.extra_producers = 0;
  }


  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&not_empty, NULL);
  pthread_cond_init(&not_full, NULL);

 // Create the needed threads based on numConsumers and numProducers.
 // Create an array of threads to initizalise them all at one.
  pthread_t threads[numConsumers+ numProducers];
  
  // Create producers:
  for(int i = 0; i < numProducers; i++){
    if (pthread_create(&threads[i], NULL, producer, &data) != 0) {
      perror("Error creating producer thread");
      return -1;
    }
    // Check that the producer was created

    printf("Created producer %d\n", i);
  }
  //Create consumers:
  for(int i = numProducers; i < numConsumers + numProducers; i++){
    if(pthread_create(&threads[i], NULL, consumer, &data) != 0){
      perror("Error creating consumer thread");
      return -1;
    }
    printf("Created consumer %d\n", i-numProducers);
  } 

  // Join all threads:
  for(int i = 0; i < numConsumers + numProducers; i++){
    pthread_join(threads[i], NULL);
  }
  
  // Destroy mutex and condition variables
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&not_empty);
  pthread_cond_destroy(&not_full);

  // free memeory
  free(operations);
  
  // close file
  fclose(file);


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
