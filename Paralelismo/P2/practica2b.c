// Emilio Martínez Varela 79344591L
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>


int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root ,MPI_Comm comm) {
    int rank, numprocs;
    int        partner;
    unsigned   bitmask = 1;
    int        participate = bitmask << 1;
	
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   while (bitmask < numprocs) {
      if (rank < participate) {
         partner = rank ^ bitmask;
         if (rank < partner) {
            if (partner < numprocs )
               MPI_Send(buf, 1, datatype, partner, 0, comm);
         } else {
            MPI_Recv(buf, 1, datatype, partner, 0, comm, 
                  MPI_STATUS_IGNORE); 
         }
      }
      bitmask <<= 1;
      participate <<= 1;
   }
   return 1;
}



int MPI_FlattreeColectiva(void *buff,void *buffM, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
	MPI_Status status;
	int mild, numprocs,c;

	if (MPI_ERR_COMM == MPI_Comm_rank(comm, &mild)) {
		printf("4");
		return -4;
	} //Obtenemos el nº del proceso
	if (MPI_ERR_COMM == MPI_Comm_size(comm, &numprocs)) {
		printf("3");
		return -3;
	} //Obtenemos el nº de procesos

    if (mild != root) {
        if (MPI_ERR_COMM == MPI_Send(buff, count, datatype, root, 0, comm)) {
            perror("send");
            return -1;
        }
    } else {
        for(c = 0; c < numprocs; c++){
            if(c != root){
                 MPI_Recv(buffM, count, datatype, c, 0, comm, &status);
                *(int*)buff = *(int*)buff + *(int*)buffM;
            }
        }
    }
    return 1;
}

int main(int argc, char *argv[]){
    int i, j, prime, done = 0, n, count;
    int numprocs, rank, c, sum, root;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    root = 0;
    while (!done){
		if (n == 0) break;
		if(rank == root){
		    printf("Enter the maximum number to check for primes: (0 quits) \n");
            scanf("%d",&n);
        }
        MPI_BinomialBcast(&n, 1, MPI_INT, root, MPI_COMM_WORLD);
        count = 0;
        for (i = 2+rank; i < n; i+=numprocs) {
            prime = 1;

            // Check if any number lower than i is multiple
            for (j = 2; j < i; j++) {
                if((i%j) == 0) {
                   prime = 0;
                   break;
                }
            }
            count += prime;
        }
        MPI_FlattreeColectiva(&count, &sum, 1, MPI_INT, root, MPI_COMM_WORLD);
        if (rank == root)
            printf("The number of primes lower than %d is %d\n", n, count);
    }
    
    MPI_Finalize();
}
