#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"

struct buffer {
	int *data;
	int size;
    pthread_mutex_t *mutex;
};


struct thread_info {
	pthread_t       thread_id;        // id returned by pthread_create()
	int             thread_num;       // application defined thread #
};

struct args {
	int 		thread_num;       // application defined thread #
	int 	        delay;			  // delay between operations
    int *iterations;
    pthread_mutex_t *m_array;
    pthread_mutex_t m_iter;
	struct buffer   *buffer;		  // Shared buffer

};

void *swap(void *ptr)
{
    int iteraciones_completadas = 0;/*Variable usada para la condición del while
	 mediante la cual comprobamos el número total de iteraciones*/
    struct args *args = ptr;

    while (!iteraciones_completadas) {
        if (pthread_mutex_lock((&args->m_iter))) {
            perror("ERROR lock");
        }

        if (!*args->iterations) { //Comprobamos si quedan iteraciones por realizar
            iteraciones_completadas = 1;
            break;
        } else {
            (*args->iterations)--;  //Si no, se reduce el número de iteraciones
        }
        if (pthread_mutex_unlock((&args->m_iter))) {
            perror("ERROR lock");
        }
		
		int i,j, tmp;
		
		i=rand() % args->buffer->size;
		j=rand() % args->buffer->size;
        while (i == j) { /*Nos aseguramos de que i y j no sean iguales para
		 evitar problemas*/
            j = rand() % args->buffer->size;
        }

        if (i < j) { /*Usamos la técnica de Reserva Ordenada asegurándonos de que
		 que los valores son distintos*/
            if (pthread_mutex_lock((&args->m_array[i]))) {
                perror("ERROR lock");
            }

            if (pthread_mutex_lock(&args->m_array[j])) {
                perror("ERROR lock");

            }
        } else {
            if (pthread_mutex_lock((&args->m_array[j]))) {
                perror("ERROR lock");
            }

            if (pthread_mutex_lock(&args->m_array[i])) {
                perror("ERROR lock");

            }
        }

        printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
			args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);

		tmp = args->buffer->data[i];
		
		if(args->delay) usleep(args->delay); // Force a context switch
		
		args->buffer->data[i] = args->buffer->data[j];
		if(args->delay) usleep(args->delay);
		
		args->buffer->data[j] = tmp;
		if(args->delay) usleep(args->delay);

        if (pthread_mutex_unlock((&args->m_array[j]))) {
            perror("ERROR unlock");
            break;
        }
        if (pthread_mutex_unlock((&args->m_array[i]))) {
            perror("ERROR unlock");
            break;
        }

	}

    if (pthread_mutex_unlock((&args->m_iter))) {
        perror("ERROR lock");
    }
	return NULL;
}

void print_buffer(struct buffer buffer) {
	int i;
	
	for (i = 0; i < buffer.size; i++)
		printf("%i ", buffer.data[i]);
	printf("\n");
}

void start_threads(struct options opt)
{
	int i, cont_mutex; // Variable contador del mutex
	struct thread_info *threads;
	struct args *args;
	struct buffer buffer;
    pthread_mutex_t *mutex_a; //Array de buffer.size mutex
    pthread_mutex_t mutex_it;
    int iterations; //Variable global usada por todos los threads

	srand(time(NULL));

	if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
		printf("Out of memory\n");
		exit(1);
	}
	buffer.size = opt.buffer_size;

	for(i=0; i<buffer.size; i++)
        buffer.data[i] = i;

	printf("creating %d threads\n", opt.num_threads);
	threads = malloc(sizeof(struct thread_info) * opt.num_threads);
	args = malloc(sizeof(struct args) * opt.num_threads);

	if (threads == NULL || args==NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	printf("Buffer before: ");
	print_buffer(buffer);

    mutex_a = malloc(buffer.size * sizeof(pthread_mutex_t));
    for (cont_mutex = buffer.size - 1; cont_mutex >= 0; cont_mutex--) {
        pthread_mutex_init(&(mutex_a[cont_mutex]), NULL);
    } //Inicializamos el array de mutex (1 por posición)

    pthread_mutex_init(&mutex_it, NULL);
    iterations = opt.iterations;

	// Create num_thread threads running swap() 
	for (i = 0; i < opt.num_threads; i++) {
		threads[i].thread_num = i;

        args[i].m_array = mutex_a;

		args[i].thread_num = i;
		args[i].buffer     = &buffer;
		args[i].delay      = opt.delay;
		args[i].iterations = &iterations;/*La nueva variable global igualada
		 al número de operaciones que queremos*/
		
		if ( 0 != pthread_create(&threads[i].thread_id, NULL,
					 swap, &args[i])) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}
	
	// Wait for the threads to finish
	for (i = 0; i < opt.num_threads; i++)
		pthread_join(threads[i].thread_id, NULL);

	// Print the buffer
	printf("Buffer after:  ");
	print_buffer(buffer);

	free(args);
	free(threads);
	free(buffer.data);

    for (cont_mutex = buffer.size - 1; cont_mutex >= 0; cont_mutex--) {
        pthread_mutex_destroy(&(mutex_a[cont_mutex]));
    }
    free(mutex_a); //Liberamos el array de mutex
    pthread_mutex_destroy(&mutex_it);
	pthread_exit(NULL);
}

int main (int argc, char **argv)
{
	struct options opt;
	// Default values for the options
	opt.num_threads = 10;
	opt.buffer_size = 10;
	opt.iterations  = 100;
	opt.delay       = 10;
	
	read_options(argc, argv, &opt);
	start_threads(opt);
	exit (0);
}
