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

int num_procs, num_rounds;

int nRounds(int count) {
        int i = 0;
        if (count <= 0) { return 0; }

        int p = count;
        while (p > 1) {
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
  num_rounds = nRounds(num_threads);
  //printf("Numrounds: %d\n", num_rounds);
  num_procs = num_threads;
}

void gtmpi_barrier(){
  int vpid; 
  MPI_Status stat;

  // MPI controls the IDs that we need to use
  MPI_Comm_rank(MPI_COMM_WORLD, &vpid);

  for (int i = 0; i < num_rounds; i++) {
    // send message to partner
    int partner = (vpid + (1<<i))%num_procs;
    // tag is deterministic for the sender and receiver in a round, 
    // so its a function of the round number, but offset by a fixed integer so that
    // test harness doesn't mess with it
    MPI_Send(0, 0, MPI_CHAR, partner, (i + 4), MPI_COMM_WORLD);

    // do MPI recv here
    partner = ((vpid - (1<<i))+num_procs)%num_procs;
    MPI_Recv(0, 0, MPI_CHAR, partner, (i + 4), MPI_COMM_WORLD, &stat);
  }
}

void gtmpi_finalize(){
}
