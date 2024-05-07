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
pthread_mutex_t Cmutex;
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
  int ops_per_producer;
  int ops_per_consumer;
  int extra_consumers;
  int extra_producers;
  int opNum;
} associated_data;

int CurrOpNum = 0;
// produce function
void produce(void* args, char* tempMsg, char* finalMsg){
  struct element curOp;
  //Get the data from the struct: 
  associated_data* data = (associated_data*)args;

  CurrOpNum += 1;
  // Whait for the conditional variable
  while(queue_full(data->buffer)){
    // wait if buffer is full (no space for us to add an operation)
    pthread_cond_wait(&not_full, &mutex);
  }

  //Save the current operation.
  curOp = data->operations[data->opNum];
  queue_put(data->buffer,&curOp);
  //DEBBUGING
  sprintf(tempMsg,"Current op %d\n",data->opNum);
  strcat(finalMsg,tempMsg);
  sprintf(tempMsg,"Current op2 %d\n",CurrOpNum);
  strcat(finalMsg,tempMsg);
  //DEBUGGING 
  
  //DEBBUGING
  sprintf(tempMsg,"Current produced operation: curOp:  ID: %d OP: %d UNITS:%d\n", curOp.product_id, curOp.op, curOp.units);
  strcat(finalMsg,tempMsg);
  //DEBUGGING

  // Change the op variable as one operation has been done.
  pthread_mutex_lock(&Cmutex);
  data -> opNum += 1;
  pthread_mutex_unlock(&Cmutex);
  //DEBBUGING
  sprintf(tempMsg,"Added one to the opNum variable\n");
  strcat(finalMsg,tempMsg);
  //DEBUGGING
  

  // signal that the buffer is not empty
  pthread_cond_signal(&not_empty);
}


void* producer(void* args) {
  associated_data* data = (associated_data*)args;
  
  //DEBUGGING
  char finalMsg[6048];
  //DEBUGGING

  // Each producer has a set number of operations.
  for (int i = 0; i < (data -> ops_per_producer); ++i) {

    //DEBUGGING
    char tempMsg[1024]; // Temporal message to store the sprintf
    //sprintf(tempMsg,"Producer entering loop for the %d time \n",i);
    //strcat(finalMsg,tempMsg);
    // DEBUGGING

    // lock buffer mutex
    pthread_mutex_lock(&mutex);
    // Critical section, produce
    produce(data, tempMsg, finalMsg);
    // Unlock the buffer mutex
    pthread_mutex_unlock(&mutex);
  }
  
  // Lock because the while needs to be protected.
  pthread_mutex_lock(&mutex);
  // Produce all the extra one after the other without anyone blocking the resource
  while(data-> extra_producers > 0)
  {
    //DEBBUGING
    char tempMsg[1024]; // Temporal message to store the sprintf
    //sprintf(tempMsg,"Producer entering extra for the extra %d time \n",data->extra_producers);
    //strcat(finalMsg,tempMsg);
    // DEBBUGING

    // Produce
    produce(data,tempMsg,finalMsg);
    // Lower the extra producers needed
    data-> extra_producers --;
  }
  // Unlock once the while is done.
  pthread_mutex_unlock(&mutex);

  // DEBBUGING
  //strcat(finalMsg,"Producer exiting thread\n");
  printf("%s",finalMsg);
  // DEBBUGING

  pthread_exit(0);
}

void computeProfitAndStock(void* args, struct element curOp)
{
  associated_data* data = (associated_data*)args;
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


// consume function
void consume(void* args, char* tempMsg, char* finalMsg)
{
  associated_data* data = (associated_data*)args;
  struct element curOp;

  // Conditional variable
  while(queue_empty(data->buffer)) {
      // while the queue is empty wait (we cant retrieve any operatiosn)
      pthread_cond_wait(&not_empty, &mutex);
    }

  // once there is an element we can extract it
  curOp = *(queue_get(data->buffer));
  
  //DEBBUGING
  //sprintf(tempMsg,"curOp: %d %d %d\n", curOp.product_id, curOp.op, curOp.units);
  //strcat(finalMsg,tempMsg);
  //DEBUGGING

  
  //DEBBUGING
  //sprintf(tempMsg,"Computing profit and stock \n");
  //strcat(finalMsg,tempMsg);
  //DEBUGGING

  // Compute the changes in profit and stock
  computeProfitAndStock(data,curOp);

  // now signal that the queue is not full
  pthread_cond_signal(&not_full);
}

void* consumer(void* args) {
  associated_data* data = (associated_data*)args;

  //DEBUGGING
  char finalMsg[3048];
  //DEBUGGING

  for (int i = 0; i < (data->ops_per_consumer); ++i) {
    
    //DEBUGGING
    char tempMsg[1024]; // Temporal message to store the sprintf
    //sprintf(tempMsg,"Consumer entering loop for the %d time \n",i);
    //strcat(finalMsg,tempMsg);
    // DEBUGGING
    
    // lock the mutex on the buffer
    pthread_mutex_lock(&mutex);
    //Consume
    consume(data,tempMsg,finalMsg);
    //Unlock the mutex on the buffer
    pthread_mutex_unlock(&mutex);

  }
  // Lock because the while needs to be protected
  pthread_mutex_lock(&mutex);
  while(data-> extra_consumers >0)
  {
    //DEBBUGING
    char tempMsg[1024]; // Temporal message to store the sprintf
    //sprintf(tempMsg,"Consumer entering extra for the extra %d time \n",data->extra_consumers);
    //strcat(finalMsg,tempMsg);
    // DEBBUGING

    //Consume
    consume(data,tempMsg,finalMsg);
    //Lower the extra consumers needed
    data -> extra_consumers --;
  }
  //Unlock once the while is done
  pthread_mutex_unlock(&mutex);
  
  //DEBBUGING
  //strcat(finalMsg,"Consumer exiting thread\n");
  //printf("%s",finalMsg);
  //DEBBUGING

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

  // GETTING DATA:

  // Number of consumers and producers
  int numProducers = atoi(argv[2]);
  int numConsumers = atoi(argv[3]);
  // Inizialization of the queue with the correct longitude
  queue* buffer = queue_init(atoi(argv[4]));
  // Open the file given in the input path
  FILE* file = fopen(file_name, "r");
  //Check for errors while opening the file
  if (file == NULL) {
    perror("Couldn't open file");
    return -1;
  }


  // LOADING THE FILE:

  // first value in file is number of operations
  int num_operations;
  fscanf(file, "%d", &num_operations);

  // use malloc to allocate enought memory for all of those operations
  struct element* operations = (struct element*)malloc(sizeof(struct element)*num_operations);
  if (operations == NULL) {
    fprintf(stderr, "Failed to allocate memory for operations.\n");
    exit(1);
  }
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

  // GET DATA INTO STRUCTURE:

  associated_data data;
  data.profits = &profits;
  data.product_stock = product_stock;
  data.buffer = buffer;
  data.operations = operations;
  data.num_operations = num_operations;
  data.ops_per_producer = (num_operations / numProducers) ;
  data.ops_per_consumer = (num_operations / numConsumers) ;
  data.opNum = 0;
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

  // Inizialization of the mutex and conditional variables
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&Cmutex,NULL);
  pthread_cond_init(&not_empty, NULL);
  pthread_cond_init(&not_full, NULL);

  // Create the needed threads based on numConsumers and numProducers.
  // Create an array of threads to initizalise them all at one.
  pthread_t threads[numConsumers + numProducers];
  
  // Create producers:
  for(int i = 0; i < numProducers; i++){

    // Check that the producer was created
    if (pthread_create(&threads[i], NULL, producer, &data) != 0) {
      perror("Error creating producer thread");
      return -1;
    }
    //DEBUGGING
    //printf("Created producer %d\n", i);
    //DEBUGGING
  }

  //Create consumers:
  for(int i = numProducers; i < numConsumers + numProducers; i++){
    //Check that the consumer was created
    if(pthread_create(&threads[i], NULL, consumer, &data) != 0){
      perror("Error creating consumer thread");
      return -1;
    }
    //DEBUGGING
    //printf("Created consumer %d\n", i-numProducers);
    //DEBUGGING
  } 

  // Join all threads:
  for(int i = 0; i < numConsumers + numProducers; i++){
    pthread_join(threads[i], NULL);
  }
  
  // Destroy mutex and condition variables
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&Cmutex);
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
