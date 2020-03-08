#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/*
    From the MCS Paper: The scalable, distributed dissemination barrier with only local spinning.
    type flags = record
        myflags : array [0..1] of array [0..LogP - 1] of Boolean
	partnerflags : array [0..1] of array [0..LogP - 1] of ^Boolean
	
    processor private parity : integer := 0
    processor private sense : Boolean := true
    processor private localflags : ^flags
    shared allnodes : array [0..P-1] of flags
        //allnodes[i] is allocated in shared memory
	//locally accessible to processor i
    //on processor i, localflags points to allnodes[i]
    //initially allnodes[i].myflags[r][k] is false for all i, r, k
    //if j = (i+2^k) mod P, then for r = 0 , 1:
    //    allnodes[i].partnerflags[r][k] points to allnodes[j].myflags[r][k]
    procedure dissemination_barrier
        for instance : integer :0 to LogP-1
	    localflags^.partnerflags[parity][instance]^ := sense
	    repeat until localflags^.myflags[parity][instance] = sense
	if parity = 1
	    sense := not sense
	parity := 1 - parity
*/

static int num_procs, num_rounds, cur_bar;
static char sense_init, parity_init;

int nRounds(int count) {
        int i = 0;
        if (count <= 0) { return 0; }

        int p = count;
        while (p > 0) {
                p = p/2;
                i++;
        }
        // if count is exact power of 2, then good, or increment i
        if ((1 << i) != count) {
                i++;
        }
        return i;
}

void gtmpi_init(int num_threads){
  num_procs = num_threads;
  num_rounds = nRounds(num_procs);
  printf("Numrounds: %d\n", num_rounds);
  sense_init = 1;
  parity_init = 0;
  cur_bar = 4;
}

void gtmpi_barrier(){
  int parity = parity_init;
  char sense = sense_init;
  int bar = cur_bar;
  int vpid; 
  int i, src, dst;
  MPI_Status stat;

  // MPI controls the IDs that we need to use
  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);

  for (i = 0; i < num_rounds; i++) {
    // do MPI send here
    dst = (vpid + (1<<i)) % num_procs;
    MPI_Send(&sense, 1, MPI_CHAR, dst, (i + (bar * num_rounds)), MPI_COMM_WORLD);

    // do MPI recv here
    src = ((vpid - (1<<i)) + num_procs) % num_procs;
    MPI_Recv(&sense, 1, MPI_CHAR, src, (i + (bar * num_rounds)), MPI_COMM_WORLD, &stat);
  }

  if (parity == 1) {
    sense_init = !sense_init;
  }

  parity_init = (parity + 1) % 2;
  cur_bar++;
}

void gtmpi_finalize(){
}
