#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "queue.h"

struct thread_info {
	pthread_t       thread_id;        // id returned by pthread_create()
	int             thread_num;       // application defined thread #
};
struct args {
	int 		thread_num;       // application defined thread #
	int 	        delay;			  // delay between operations
	int		iterations;
};
struct options {
	int num_threads;
	int buffer_size;
	int iterations;
	int delay;
};

void *prueba(void *ptr){
	queue q;
	q =q_create(10);
	struct args *args =  ptr;
	q_insert(q, args);
	q_print(q);
}

void start_threads(struct options opt){
	int i;
	struct args *args;
	struct thread_info *threads;

	threads = malloc(sizeof(struct thread_info) * opt.num_threads);
	args = malloc(sizeof(struct args) * opt.num_threads);
		
		for (i = 0; i < opt.num_threads; i++) {
			threads[i].thread_num = i;		
			args[i].thread_num = i;
			args[i].delay      = opt.delay;
			args[i].iterations = opt.iterations;
		
		if ( 0 != pthread_create(&threads[i].thread_id, NULL, prueba, &args[i])) {
				printf("Could not create thread #%d", i);
				exit(1);
		}
	}
	free(args);
	free(threads);

}


int main(int arg, char **argv){
	struct options opt;
	// Default values for the options
	opt.num_threads = 10;
	opt.buffer_size = 10;
	opt.iterations  = 100;
	opt.delay       = 10;
	
	start_threads(opt);
	exit (0);
}

