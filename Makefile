#OpenMP Flags Etc.
OMPFLAGS = -g -Wall -fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`
OMPLIBS = -lgomp
CC = gcc

#MPI Flags Etc (may need to customize)
#MPICH = /usr/lib64/openmpi/1.4-gcc
MPIFLAGS = -g -Wall #-I$(MPICH)/include
MPILIBS =
#MPICC = /opt/openmpi-1.4.3-gcc44/bin/mpicc
MPICC = mpicc

all: mpi_tournament mpi_dissemination mpi_counter mp_counter mp_mcs

mp_counter: gtmp_counter.c mp_main.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

mp_mcs: gtmp_mcs.c mp_main.c
	$(CC) $(OMPFLAGS) -o $@ $^ $(OMPLIBS)

mpi_counter: gtmpi_counter.c mpi_main.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

mpi_tournament: gtmpi_tournament.c mpi_main.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

mpi_dissemination: gtmpi_dissemination.c mpi_main.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

clean:
	rm -rf *.o hello_openmp hello_mpi
