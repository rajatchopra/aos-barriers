#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include "gtmpi.h"

/*
    From the MCS Paper: A sense-reversing centralized barrier

    shared count : integer := P
    shared sense : Boolean := true
    processor private local_sense : Boolean := true

    procedure central_barrier
        local_sense := not local_sense // each processor toggles its own sense
	if fetch_and_decrement (&count) = 1
	    count := P
	    sense := local_sense // last processor toggles global sense
        else
           repeat until sense = local_sense
*/


// rchopra33: no need for status array, each message can have a static one
// static MPI_Status* status_array;
static int P;

void gtmpi_init(int num_threads){
  P = num_threads;
  // status_array = (MPI_Status*) malloc((P - 1) * sizeof(MPI_Status));
}

void gtmpi_barrier(){
  int vpid, i;
  MPI_Status status;

  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);
  
  // rchopra33: instead of thread sending it to everybody, all can send to 0
  // and process 0 can be the gatekeeper of the barrier
  if (vpid != 0) {
    MPI_Send(NULL, 0, MPI_INT, 0, 1, MPI_COMM_WORLD);
  } else {
        // and process 0 can receive from everyone
          for(i = 1; i < P; i++)
            MPI_Recv(NULL, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
   }
}

void gtmpi_finalize(){
  //
  //if(status_array != NULL){
  //  free(status_array);
  //}
}

