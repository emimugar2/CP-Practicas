#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    char **data;
    pthread_mutex_t *mutex;
    pthread_cond_t *full;
    pthread_cond_t *empty;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));
	pthread_mutex_t *mutex;
	
	//Miramos si hay memoria para el mutex, si no error
	if((mutex = malloc(sizeof(pthread_mutex_t)))==NULL){
		printf("Sin memoria para mutex\n");
		exit(1);
	}
	//Comprobamos si tenemos memoria para crear las variables de condicion
	pthread_cond_t *full;
	if((full = malloc(sizeof(pthread_cond_t)))==NULL){
		printf("Sin memoria para cond\n");
		exit(1);
	}
	
	pthread_cond_t *empty;
	if((empty = malloc(sizeof(pthread_cond_t)))==NULL){
		printf("Sin memoria para cond\n");
		exit(1);
	}
	
	q->mutex=mutex;
	q->full=full;
	q->empty=empty;
	//Inicializamos las variables de condicion y el mutex
	pthread_cond_init(q->full, NULL);
	pthread_cond_init(q->empty, NULL);
	pthread_mutex_init(q->mutex, NULL);
	
	q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size*sizeof(void *));
    return q;
}

int q_elements(queue q) {
    return q->used;
}

void q_print(queue q){
	int i;
	
	for (i = 0; i < q->used; ++i){
		printf("%s\n",q->data[i]);
	}
}
int q_insert(queue q, void *elem) {
    pthread_mutex_lock(q->mutex);
    while(q->size == q->used) {
		pthread_cond_wait(q->full, q->mutex);
	}
	
    q->data[(q->first+q->used) % q->size] = elem;    
    q->used++;
    
    if (q->used == 1){
		pthread_cond_broadcast(q->empty);
	}
    pthread_mutex_unlock(q->mutex);
    return 1;
}

void *q_remove(queue q) {
    void *res;
	pthread_mutex_lock(q->mutex);
    while(q->used==0) {
			pthread_cond_wait(q->empty,q->mutex);
	}
    
    res = q->data[q->first];
    
    q->first = (q->first+1) % q->size;
    q->used--;
    if(q->used == ((q->size)-1)){
			pthread_cond_broadcast(q->full);
	}
    pthread_mutex_unlock(q->mutex);

    return res;
}



void q_destroy(queue q) {
    pthread_cond_destroy(q->full);
		pthread_cond_destroy(q->empty);
    pthread_mutex_destroy(q->mutex);
    free(q->mutex);
    free(q->data);
    free(q);
}
