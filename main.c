#include <stdio.h>
#include <sys/utsname.h>
#include "mpi.h"
#include "gtmpi.h"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

bool disableBarrier = false;

void printMessages(int my_id, int num_processes, int num_barriers);

int main(int argc, char **argv)
{
        int my_id, num_processes;
        struct utsname ugnm;
        MPI_Request req;

        MPI_Init(&argc, &argv);

        MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
        gtmpi_init(num_processes);

        MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

        uname(&ugnm);

        char msg[256];
        sprintf(msg,"Hello World from thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);
        // send to 0
        MPI_Isend(msg, 256, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &req);
        //sleep(my_id + 1);
        if (!disableBarrier) {
                gtmpi_barrier();
        }

        sprintf(msg,"Goodbye cruel world from thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);
        MPI_Isend(msg, 256, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &req);
        //sleep(my_id + 1);
        if (!disableBarrier) {
                gtmpi_barrier();
        }

        sprintf(msg,"Last test from thread %d of %d, running on %s.\n", my_id, num_processes, ugnm.nodename);
        MPI_Isend(msg, 256, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &req);
        //sleep(my_id + 1);
        if (!disableBarrier) {
                gtmpi_barrier();
        }

        printMessages(my_id, num_processes, 3);

        MPI_Finalize();
        gtmpi_finalize();
        return 0;
}

void printMessages(int my_id, int num_processes, int num_barriers) {
        if (my_id == 0) {
                for (int i =0; i < num_processes*num_barriers; i++) {
                        MPI_Status stat;
                        char buffer[256];
                        MPI_Recv(buffer, 256, MPI_CHAR, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &stat);
                        printf("%s", buffer);
                        fflush(stdout);
                }
        }
}
