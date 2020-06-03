#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"
#include <pthread.h>

#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 20

#define COMPRESS 1
#define DECOMPRESS 0

struct infoThread{
	pthread_t thread_id;
	int thread_num;
};
struct args {
	int 		thread_num;     
	queue 		*in;
	queue		*out;
	chunk     (*process) (chunk);
	pthread_mutex_t *mutex;
};

struct args_lectura{
	int fd;
	int size;
	queue *q_in;
	int chunks;
	
};

struct args_escritura{
	archive ar;
	queue *q_out;
	int chunks;
	
};

struct args_rChunck{
	queue *q_in;
	archive ar;
};

struct args_wChunck{
	queue *q_in;
	int fd;
	archive ar;
	queue *q_out;
};


// take chunks from queue in, run them through process (compress or decompress), send them to queue out
void *worker(void *ptr) {
struct args *args = ptr;
    chunk ch, res;
    while(q_elements(*args->in)>0) {
        ch = q_remove(*args->in);

        res = args->process(ch);
        free_chunk(ch);

        q_insert(*args->out, res);
    }
}

void *lectura(void *ptr){
	//Lee el fichero e inserta los chunks en la cola
	struct args_lectura *args_lect = ptr;
	chunk ch;
	int i, offset;

    for(i=0; i<args_lect->chunks; i++) {
        ch = alloc_chunk(args_lect->size);

        offset=lseek(args_lect->fd, 0, SEEK_CUR);

        ch->size   = read(args_lect->fd, ch->data,args_lect->size);
        ch->num    = i;
        ch->offset = offset;

        q_insert(*args_lect->q_in, ch);
    }	
}

void *escritura (void *ptr){
	//Escribe los ficheros de la cola de salida en el archivo
	struct args_escritura *args_esc = ptr;
	chunk ch;
	int i;
    for(i=0; i<args_esc->chunks; i++) {
        ch = q_remove(*args_esc->q_out);
        add_chunk(args_esc->ar, ch);
        free_chunk(ch);
    }
}

void *read_decomp(void *ptr){
	//Lee los chunks del archicvo 
	struct args_rChunck *args_r = ptr;
	chunk ch;
	int i;
	for(i=0; i<chunks(args_r->ar); i++) {
      ch = get_chunk(args_r->ar, i);
      q_insert(*args_r->q_in, ch);
    }
}

void *write_decomp(void *ptr){
	//Escribe en la cola de salida
	struct args_wChunck *args_write = ptr;
	chunk ch;
	int i;
	for(i=0; i<chunks(args_write->ar); i++) {
        ch=q_remove(*args_write->q_out);
        lseek(args_write->fd, ch->offset, SEEK_SET);
        write(args_write->fd, ch->data, ch->size);
        free_chunk(ch);
		}	
}
// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt) {
    int fd, chunks, i;
    struct stat st;
    char comp_file[256];
    archive ar;
    queue in, out;
    //Declaramos los threads y los structs a utilizar
	pthread_t lect;
	pthread_t escrit;
	struct args_lectura args_lect;
	struct args_escritura args_esc;
	struct infoThread *threads;
	struct args *args;
    if((fd=open(opt.file, O_RDONLY))==-1) {
        printf("Cannot open %s\n", opt.file);
        exit(0);
    }

    fstat(fd, &st);
    chunks = st.st_size/opt.size+(st.st_size % opt.size ? 1:0);
    if(opt.out_file) {
        strncpy(comp_file,opt.out_file,255);
    } else {
        strncpy(comp_file, opt.file, 255);
        strncat(comp_file, ".ch", 255);
    }

    ar = create_archive_file(comp_file);

    in  = q_create(opt.queue_size);
    out = q_create(opt.queue_size);

    //Se lee el archivo y se mandan los chunks a la cola
	args_lect.chunks = chunks;
	args_lect.fd = fd;
	args_lect.q_in = &in;
	args_lect.size = opt.size;
	//Ejecutamos la funcion de lectura 
	if ( 0 != pthread_create(&lect, NULL,
					 lectura, &args_lect)) {
			printf("Could not create thread");
			exit(1);
		}
	
	threads = malloc(sizeof(struct infoThread) * opt.num_threads);
	args = malloc(sizeof(struct args) * opt.num_threads);
	
	
	if (threads == NULL || args==NULL) {
		printf("Not enough memory\n");
		exit(1);
	}
	
    //Se crean los threads y se ejecuta worker(), pasando los
	//argumentos que necesita
   	for (i = 0; i < opt.num_threads; i++) {
		threads[i].thread_num = i;	
		args[i].thread_num = i;
		args[i].in = &in;
		args[i].out = &out;
		args[i].process = zcompress;

		if ( 0 != pthread_create(&threads[i].thread_id, NULL,
					 worker, &args[i])) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}
		
		
	//Esperamos a que terminen los threads
	for (i = 0; i < opt.num_threads; i++)
	pthread_join(threads[i].thread_id, NULL);
	pthread_join(lect, NULL);

	//Escribimos los chunks de la cola out en el archivo de salida
	args_esc.chunks = chunks;
	args_esc.ar = ar;
	args_esc.q_out = &out;
	
	if ( 0 != pthread_create(&escrit, NULL, escritura, &args_esc)) {
			printf("Could not create thread");
			exit(1);
		}

   
pthread_join(escrit, NULL);
    
    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);
    free(args);
	free(threads);
}


// Decompress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the decompressed file

void decomp(struct options opt) {
    int fd, i;
    char uncomp_file[256];
    archive ar;
    queue in, out;
   
   //Creamos los threads de lectura y escritura y los structs de los argumentos
	pthread_t read_thread;
	pthread_t write_thread;
	struct infoThread *threads;
	struct args *args;
	struct args_rChunck read;
	struct args_wChunck escribe;

    if((ar=open_archive_file(opt.file))==NULL) {
        printf("Cannot open archive file\n");
        exit(0);
    };
    
    if(opt.out_file) {
        strncpy(uncomp_file, opt.out_file, 255);
    } else {
        strncpy(uncomp_file, opt.file, strlen(opt.file) -3);
        uncomp_file[strlen(opt.file)-3] = '\0';
    }

    if((fd=open(uncomp_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))== -1) {
        printf("Cannot create %s: %s\n", uncomp_file, strerror(errno));
        exit(0);
    }

    in  = q_create(opt.queue_size);
    out = q_create(opt.queue_size);

    // read chunks with compressed data
	read.ar = ar;
	read.q_in = &in;
	if ( 0 != pthread_create(&read_thread, NULL,
					 read_decomp, &read)) {
			printf("Could not create thread");
			exit(1);
		}
    //Se reserva memoria para la info de los threads y los
	//argumentos de la funcion workers
	threads = malloc(sizeof(struct infoThread) * opt.num_threads);
	args = malloc(sizeof(struct args) * opt.num_threads);
	
    if (threads == NULL || args==NULL) {
		printf("Not enough memory\n");
		exit(1);
	}
    
    //Se crean los threads y se ejecuta la funcion worker
    for (i = 0; i < opt.num_threads; i++) {
		threads[i].thread_num = i;	
		args[i].thread_num = i;
		args[i].in = &in;
		args[i].out = &out;
		args[i].process = zdecompress;
	
		if ( 0 != pthread_create(&threads[i].thread_id, NULL,
					 worker, &args[i])) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}
		//Esperamos a que los threads terminen
		for (i = 0; i < opt.num_threads; i++)
		pthread_join(threads[i].thread_id, NULL);
			pthread_join(read_thread, NULL);
	
	escribe.q_out = &out;
	escribe.q_in = &in;
	escribe.fd = fd;
	escribe.ar = ar;
	if ( 0 != pthread_create(&write_thread, NULL,
					 write_decomp, &escribe)) {
			printf("Could not create thread");
			exit(1);
		}
	

	pthread_join(write_thread, NULL);
	
    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);
    free(args);
	free(threads);
}

int main(int argc, char *argv[]) {
    struct options opt;

    opt.compress    = COMPRESS;
    opt.num_threads = 3;
    opt.size        = CHUNK_SIZE;
    opt.queue_size  = QUEUE_SIZE;
    opt.out_file    = NULL;

    read_options(argc, argv, &opt);

    if(opt.compress == COMPRESS) comp(opt);
    else decomp(opt);
}
