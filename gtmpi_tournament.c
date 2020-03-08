#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"

/*
   From the MCS Paper: A scalable, distributed tournament barrier with only local spinning
   type round_t = record
role : (winner, loser, bye, champion, dropout)
opponent : ^Boolean
flag : Boolean
shared rounds : array [0..P-1][0..LogP] of round_t
// row vpid of rounds is allocated in shared memory
// locally accessible to processor vpid
processor private sense : Boolean := true
processor private vpid : integer // a unique virtual processor index
//initially
//    rounds[i][k].flag = false for all i,k
//rounds[i][k].role = 
//    winner if k > 0, i mod 2^k = 0, i + 2^(k-1) < P , and 2^k < P
//    bye if k > 0, i mode 2^k = 0, and i + 2^(k-1) >= P
//    loser if k > 0 and i mode 2^k = 2^(k-1)
//    champion if k > 0, i = 0, and 2^k >= P
//    dropout if k = 0
//    unused otherwise; value immaterial
//rounds[i][k].opponent points to 
//    round[i-2^(k-1)][k].flag if rounds[i][k].role = loser
//    round[i+2^(k-1)][k].flag if rounds[i][k].role = winner or champion
//    unused otherwise; value immaterial
procedure tournament_barrier
round : integer := 1
loop   //arrival
case rounds[vpid][round].role of
loser:
rounds[vpid][round].opponent^ :=  sense
repeat until rounds[vpid][round].flag = sense
exit loop
winner:
repeat until rounds[vpid][round].flag = sense
bye:  //do nothing
champion:
repeat until rounds[vpid][round].flag = sense
rounds[vpid][round].opponent^ := sense
exit loop
dropout: // impossible
round := round + 1
loop  // wakeup
round := round - 1
case rounds[vpid][round].role of
loser: // impossible
winner:
rounds[vpid[round].opponent^ := sense
bye: // do nothing
champion: // impossible
dropout:
exit loop
sense := not sense
 */

#define WINNER (0)
#define LOSER (1)
#define BYE (2)
#define CHAMPION (3)
#define DROPOUT (4)
#define UNUSED (5)

typedef struct round {
        int opponent;
        int role;
} round_t;

round_t **rounds;
int P, num_rounds;

int nRounds(int count) {
        int i = 0;
        if (count <= 0) { return 0; }

        int p = count;
        while (p > 0) {
                p = p/2;
                i++;
        }
        return i;
}

void gtmpi_init(int num_threads) {
        P = num_threads;
        num_rounds = nRounds(P)+1;

        rounds = (round_t **) malloc(P * sizeof(round_t *));
        for (int i = 0; i < P; i++) {
                rounds[i] = (round_t *) malloc(num_rounds * sizeof(round_t));

                for (int j = 0; j < num_rounds; j++) {

                        if (j > 0) {
                                //    winner if k > 0, i mod 2^k = 0, i + 2^(k-1) < P , and 2^k < P
                                if ((i % (1 << j) == 0) && (i + (1 << (j-1)) < P) &&
                                                ((1 << j) < P)) {
                                        rounds[i][j].role = WINNER;
                                //    bye if k > 0, i mode 2^k = 0, and i + 2^(k-1) >= P
                                } else if ((i % (1 << j) == 0) && (i + (1 << (j-1)) >= P)) {
                                        rounds[i][j].role = BYE;
                                //    loser if k > 0 and i mode 2^k = 2^(k-1)
                                } else if ((i % (1 << j)) == (1 << (j-1))) {
                                        rounds[i][j].role = LOSER;
                                //    champion if k > 0, i = 0, and 2^k >= P
                                } else if ((i == 0) && ((1 << j) >= P)) {
                                        rounds[i][j].role = CHAMPION;
                                // unused otherwise; value immaterial
                                } else {
                                        rounds[i][j].role = UNUSED;
                                }
                        } else {
                                // dropout if k = 0
                                rounds[i][j].role = DROPOUT;
                        }

                        // now set the opponents
                        if (rounds[i][j].role == LOSER) {
                                rounds[i][j].opponent = i - (1 << (j-1));
                        } else if (rounds[i][j].role == WINNER || 
                                        rounds[i][j].role == CHAMPION) {
                                rounds[i][j].opponent = i + (1 << (j-1));
                        }
                }
        }
}

void gtmpi_barrier() {
        MPI_Status stat;
        int round = 1;
        int vpid;
        int exit_loop = 0;

        // MPI controls the IDs that we need to use
        MPI_Comm_rank(MPI_COMM_WORLD, &vpid);

        // Arrival loop
        while (!exit_loop) {
                switch (rounds[vpid][round].role) {
                        case LOSER:
                                // Blocking Send to the winner of this round:
                                MPI_Send(0, 0, MPI_CHAR, rounds[vpid][round].opponent, 1, MPI_COMM_WORLD);

                                // Blocking Recv here - receives the wake-up call when the opponent is done with rest of the rounds of the tournament
                                MPI_Recv(0, 0, MPI_CHAR, rounds[vpid][round].opponent, 1, MPI_COMM_WORLD, &stat);

                                // break out of loop
                                exit_loop = 1;
                                break;

                        case WINNER:
                                // MPI recv here: wait for the loser opponnents to inform when they have reached the barrier
                                MPI_Recv(0, 0, MPI_CHAR, rounds[vpid][round].opponent, 1, MPI_COMM_WORLD, &stat);
                                break;

                        case BYE:
                                // will get promoted to final round as loser to the champion
                                break;

                        case CHAMPION:
                                //MPI Recv here
                                MPI_Recv(0, 0, MPI_CHAR, rounds[vpid][round].opponent, 1, MPI_COMM_WORLD, &stat);

                                // MPI send here
                                MPI_Send(0, 0, MPI_CHAR, rounds[vpid][round].opponent, 1, MPI_COMM_WORLD);

                                // break out of loop
                                exit_loop = 1;
                                break;

                        case DROPOUT:
                                break;

                        default:
                                break;
                }

                if (exit_loop == 0) {
                        round++;
                }
        }

        exit_loop = 0;

        // Wake up losers
        while (exit_loop == 0) {
                round--;

                switch (rounds[vpid][round].role) {
                        case WINNER:
                                // Send message to loser brother
                                MPI_Send(0, 0, MPI_CHAR, rounds[vpid][round].opponent, 1, MPI_COMM_WORLD);
                                break;
                        case DROPOUT:
                                exit_loop = 1;
                                break;
                        default:
                                break;
                }
        }
}

void gtmpi_finalize() {
        for (int i = 0; i < P; i++) {
                free(rounds[i]);
        }
        free(rounds);
}
