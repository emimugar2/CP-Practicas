// Emilio Martínez Varela 79344591L
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

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
        MPI_Bcast(&n, 1, MPI_INT, root, MPI_COMM_WORLD);
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
        MPI_Reduce(&count, &sum, 1, MPI_INT,MPI_SUM,root,MPI_COMM_WORLD);
        if(rank == root)
            printf("The number of primes lower than %d is %d\n", n, sum);
    }
    MPI_Finalize();
}
