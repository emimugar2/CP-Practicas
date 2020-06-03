// Emilio Martínez Varela 79344591L

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>

#define DEBUG 0

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/


#define M  1000 // Number of sequences
#define N  200000  // Number of bases per sequence

// The distance between two bases
int base_distance(int base1, int base2) {

    if ((base1 == 4) || (base2 == 4)) {
        return 3;
    }

    if (base1 == base2) {
        return 0;
    }

    if ((base1 == 0) && (base2 == 3)) {
        return 1;
    }

    if ((base2 == 0) && (base1 == 3)) {
        return 1;
    }

    if ((base1 == 1) && (base2 == 2)) {
        return 1;
    }

    if ((base2 == 2) && (base1 == 1)) {
        return 1;
    }

    return 2;
}

int main(int argc, char *argv[]) {

    int *result, *issue_piece;
    int i, j, mpi_size, mpi_rank, end, communication_time, computing_time, block_size;
    int *data1, *data2, *recv_data1, *recv_data2, *communication_times, *computing_times;
    struct timeval tv1, tv2;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    block_size = ceil(1.0 * M / mpi_size);

    issue_piece = (int *) malloc((int) block_size * sizeof(int));
    recv_data1 = (int *) malloc((int) block_size * N * sizeof(int));
    recv_data2 = (int *) malloc((int) block_size * N * sizeof(int));


    if (mpi_rank == 0) {
        data1 = (int *) malloc(block_size * mpi_size * N * sizeof(int));
        data2 = (int *) malloc(block_size * mpi_size * N * sizeof(int));
        result = (int *) malloc(block_size * mpi_size * sizeof(int));

        computing_times = (int *) malloc(mpi_size * sizeof(int));
        communication_times = (int *) malloc(mpi_size * sizeof(int));

        for(i=0;i<M;i++) {
            for(j=0;j<N;j++) {
                data1[i*N+j] = (i+j)%5;
                data2[i*N+j] = ((i-j)*(i-j))%5;
            }
        }
    }


    gettimeofday(&tv1, NULL);

    //data entrada | numero de elementos por cada uno n*m/numero procesos | los trozos que recibe | numero total que recibe
    MPI_Scatter(data1, N * block_size, MPI_INT, recv_data1, N * block_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(data2, N * block_size, MPI_INT, recv_data2, N * block_size, MPI_INT, 0, MPI_COMM_WORLD);

    gettimeofday(&tv2, NULL);

    communication_time = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

    gettimeofday(&tv1, NULL);

    end = block_size;
    if (mpi_rank == (mpi_size - 1)) {
        end = M - block_size * (mpi_size - 1);
    }
    //Se realiza el calculo hasta el tamaño de los cachos que te corresponde
    for (i = 0; i < end; i++) {
        //inicializar issue_piece con el mismo tamaño de secuencias y se va guardando los resultados
        issue_piece[i] = 0;
        for (j = 0; j < N; j++) {
            issue_piece[i] += base_distance(recv_data1[i * N + j], recv_data2[i * N + j]);
        }
    }

    gettimeofday(&tv2, NULL);

    computing_time = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

    gettimeofday(&tv1, NULL);

    /*Coge los trozos de los resultados y lo pega en un array mas grande y como resultado obtiene
    * todo el array de M | Es lo contrario al Scatter */
    MPI_Gather(issue_piece, block_size, MPI_INT, result, block_size, MPI_INT, 0, MPI_COMM_WORLD);

    gettimeofday(&tv2, NULL);

    communication_time = +(tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);


    MPI_Gather(&computing_time, 1, MPI_INT, computing_times, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&communication_time, 1, MPI_INT, communication_times, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //Se imprimen los tiempos de computo y comunicación por cada uno de los procesos
    if (mpi_rank == 0) {
        if (DEBUG) {
            for (i = 0; i < M; i++) {
                printf(" %d \t ", result[i]);
            }
        } else {
            for (i = 0; i < mpi_size; i++) {
                printf("PROCESO %d \nTiempo de computo: %lf segundos \nTiempo de comunicación: %lf segundos\n\n", i,
                       (double) computing_times[i] / 1E6, (double) communication_times[i] / 1E6);

            }
        }

        free(data1);
        free(data2);
        free(computing_times);
        free(communication_times);
        free(result);

    }
    free(recv_data1);
    free(recv_data2);
    free(issue_piece);

    MPI_Finalize();

    return 0;
}