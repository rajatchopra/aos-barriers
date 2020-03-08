#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <omp.h>
#include "gtmp.h"

/*
    From the MCS Paper: A scalable, distributed tree-based barrier with only local spinning.

    type treenode = record
        parentsense : Boolean
	parentpointer : ^Boolean
	childpointers : array [0..1] of ^Boolean
	havechild : array [0..3] of Boolean
	childnotready : array [0..3] of Boolean
	dummy : Boolean //pseudo-data

    shared nodes : array [0..P-1] of treenode
        // nodes[vpid] is allocated in shared memory
        // locally accessible to processor vpid
    processor private vpid : integer // a unique virtual processor index
    processor private sense : Boolean

    // on processor i, sense is initially true
    // in nodes[i]:
    //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
    //    parentpointer = &nodes[floor((i-1)/4].childnotready[(i-1) mod 4],
    //        or dummy if i = 0
    //    childpointers[0] = &nodes[2*i+1].parentsense, or &dummy if 2*i+1 >= P
    //    childpointers[1] = &nodes[2*i+2].parentsense, or &dummy if 2*i+2 >= P
    //    initially childnotready = havechild and parentsense = false
	
    procedure tree_barrier
        with nodes[vpid] do
	    repeat until childnotready = {false, false, false, false}
	    childnotready := havechild //prepare for next barrier
	    parentpointer^ := false //let parent know I'm ready
	    // if not root, wait until my parent signals wakeup
	    if vpid != 0
	        repeat until parentsense = sense
	    // signal children in wakeup tree
	    childpointers[0]^ := sense
	    childpointers[1]^ := sense
	    sense := not sense
*/

typedef struct {
        bool parentsense;
        bool *parentpointer;
        bool *childpointers[2];
        bool havechild[4];
        bool childnotready[4];
        bool dummy;
} treenode;

treenode *nodes;

int num_procs;
bool global_sense;


void gtmp_init(int num_threads) {
        num_procs = num_threads;
        global_sense = true;
        nodes = (treenode *) malloc(num_procs * sizeof(treenode));

        for (int i = 0; i < num_procs; i++) {
                for (int j = 0; j < 4; j++) {
                        //    havechild[j] = true if 4 * i + j + 1 < P; otherwise false
                        if (4 * i + j + 1 < num_procs) {
                                nodes[i].havechild[j] = true;
                        } else {
                                nodes[i].havechild[j] = false;
                        }

                        //    initially childnotready = havechild and parentsense = false
                        nodes[i].childnotready[j] = nodes[i].havechild[j];
                }

                //    parentpointer = &nodes[floor((i-1)/4].childnotready[(i-1) mod 4],
                if (i > 0) {
                        nodes[i].parentpointer = &(nodes[(i-1)/4].childnotready[(i-1)%4]);
                } else {
                        nodes[i].parentpointer = &(nodes[i].dummy);
                }

                //    childpointers[0] = &nodes[2*i+1].parentsense, or &dummy if 2*i+1 >= P
                if (2 * i + 1 < num_procs) {
                        nodes[i].childpointers[0] = &(nodes[2*i+1].parentsense);
                } else {
                        nodes[i].childpointers[0] = &(nodes[i].dummy);
                }

                //    childpointers[1] = &nodes[2*i+2].parentsense, or &dummy if 2*i+2 >= P
                if (2 * i + 2 < num_procs) {
                        nodes[i].childpointers[1] = &(nodes[2*i+2].parentsense);
                } else {
                        nodes[i].childpointers[1] = &(nodes[i].dummy);
                }

                //    initially childnotready = havechild and parentsense = false
                nodes[i].parentsense = false;
        }
}

void gtmp_barrier() {
        int vpid = omp_get_thread_num();
        bool sense = global_sense;

	//    repeat until childnotready = {false, false, false, false}
        while ((!nodes[vpid].childnotready[0] && !nodes[vpid].childnotready[1] &&
                                !nodes[vpid].childnotready[2] && !nodes[vpid].childnotready[3])) { }

	//    childnotready := havechild //prepare for next barrier
        for (int i = 0; i < 4; i++) {
                nodes[vpid].childnotready[i] = nodes[vpid].havechild[i];
        }

	//    parentpointer^ := false //let parent know I'm ready
        *(nodes[vpid].parentpointer) = 0;

        if (vpid != 0) {
                // spinozaaa
                // wait for sense to match
                while (nodes[vpid].parentsense != sense) { }
        }

        *nodes[vpid].childpointers[0] = sense;
        *nodes[vpid].childpointers[1] = sense;

        global_sense = !sense;
}

void gtmp_finalize(){
        free(nodes);
}
