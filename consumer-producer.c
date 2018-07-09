#include <pthread.h> 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_CONS_THREADS 3 
#define NUM_TOT_THREADS 4

#define BUF_LEN 1024

#define TRUE 1
#define FALSE 0 

typedef int bool;

// data structure for inputted and consumed messages
struct data {
	char message[BUF_LEN];
	bool isConsumed;
	bool isPrinted;
	int consumerid;
};

// variables accessed by the input and toPrint mutexes
struct data input;
struct data toPrint;

// mutex lock for consumed message
pthread_mutex_t cons_lock;

// mutex lock for printed message
pthread_mutex_t print_lock;

// conditional variable for consumer
pthread_cond_t cons_cond = PTHREAD_COND_INITIALIZER;

// conditional variable for printer
pthread_cond_t print_cond = PTHREAD_COND_INITIALIZER;

// consuming function
void *consume(void *consumerno) { 
	int consumerid = *((int *) consumerno);
    free(consumerno);
	while(TRUE) {
		// lock consumer mutex
		pthread_mutex_lock(&cons_lock);

		if(input.isConsumed == FALSE) {
	 		printf("message consumed\n");
	 		input.isConsumed = TRUE;
	 		input.isPrinted = FALSE;
	 		input.consumerid = consumerid;

	 		// lock printer mutex
			pthread_mutex_lock(&print_lock);
			strncpy(toPrint.message, input.message, BUF_LEN);
			toPrint.isConsumed = TRUE;
			toPrint.isPrinted = FALSE;
			toPrint.consumerid = consumerid;

			// send printer conditional signal
			pthread_cond_signal(&print_cond);

			// unlock printer mutex
			pthread_mutex_unlock(&print_lock);

			input.isPrinted = TRUE;
		}

		// unlock consumer mutex
		pthread_mutex_unlock(&cons_lock);
		// send consumer conditional signal
		pthread_cond_signal(&cons_cond);
	}
	pthread_exit(NULL); 
} 

// printing function
void *print() {
	// lock printer mutex
	pthread_mutex_lock(&print_lock);

	while(TRUE) {
		// wait for printer condition call
		pthread_cond_wait(&print_cond, &print_lock);
		if(toPrint.isConsumed == TRUE && toPrint.isPrinted == FALSE) {
			printf("%d %s\n", toPrint.consumerid, toPrint.message);
	 		toPrint.isPrinted = TRUE;
		}
	}

	// unlock printer mutex
	pthread_mutex_unlock(&print_lock);
	pthread_exit(NULL);
} 

int main (int argc, const char * argv[]) {
	//set initial input values
	input.isConsumed = TRUE;
	input.isPrinted = TRUE;
	toPrint.isConsumed = TRUE;
	toPrint.isPrinted = TRUE;

	pthread_t threads[NUM_TOT_THREADS]; 
	int rc,t; 

	// init consumers
	for (t=0;t<NUM_CONS_THREADS;t++) { 
		printf("Creating consumer %d\n",t+1); 
		int *consumerno = malloc(sizeof(int));
		*consumerno = t+1;
		rc = pthread_create(&threads[t],NULL,consume,consumerno); 
		if (rc) { 
			printf("ERROR return code from pthread_create(): %d\n",rc); 
			exit(-1); 
		} 
	}

	// init printer
	printf("creating printer\n");
	rc = pthread_create(&threads[3],NULL,print,NULL); 
		if (rc) { 
			printf("ERROR return code from pthread_create(): %d\n",rc); 
			exit(-1); 
		} 

	// init consumer mutex
	printf("creating consumer mutex\n");
	if (pthread_mutex_init(&cons_lock, NULL) != 0) {
		printf("error initialising mutex\n");
		exit(1);
	}

	// init printer mutex
	printf("creating printer mutex\n");
	if (pthread_mutex_init(&print_lock, NULL) != 0) {
		printf("error initialising mutex\n");
		exit(1);
	}

	printf("\n");

	// lock consumer mutex
	pthread_mutex_lock(&cons_lock);

	// init producer - waits for string from console
	while(TRUE) {
		if(input.isConsumed == TRUE && input.isPrinted == TRUE) {
			printf("input your message: ");
			fgets(input.message, BUF_LEN, stdin);
			printf("message received\n");
			input.isConsumed = FALSE;
			input.isPrinted = FALSE;

			// wait for consumer conditional call
			pthread_cond_wait(&cons_cond, &cons_lock);

			// give time for prining thread to print message
			sleep(0.5);
		}
	}

	// unlock consumer mutex
	pthread_mutex_unlock(&cons_lock);

	// wait for threads to exit 
	for(t=0;t<NUM_TOT_THREADS;t++) { 
		pthread_join(threads[t], NULL); 
	} 
	return 0;
}