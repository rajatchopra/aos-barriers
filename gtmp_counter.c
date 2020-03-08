#include <omp.h>
#include <stdbool.h>
#include "gtmp.h"

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


volatile unsigned int count;
bool sense;

int num_procs;

void gtmp_init(int num_threads){
    count = num_threads;
    num_procs = num_threads;
    sense = true;
}

void gtmp_barrier(){

    bool local_sense = !sense;

    if( __sync_fetch_and_sub(&count, 1) == 1 ) {
        // count is now 1, which means I am the last thread
        count = num_procs;
        sense = local_sense;
    } else {
        while(sense != local_sense) {}
    }
}

void gtmp_finalize(){
}
