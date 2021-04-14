CXXFLAGS ?= -std=c++17 -fopenmp -O2 -g3 -Wall -Wextra -pedantic

headers := $(wildcard *.h)
common_src := bfs_common.cpp graph.cpp

.PHONY: clean check_mpi

bfs_omp: main_omp.cpp bfs_omp.cpp $(common_src)
	g++ $(CXXFLAGS) -o $@ $^
bfs_omp_mpi: main_omp_mpi.cpp bfs_omp_mpi.cpp $(common_src)
	mpicxx $(CXXFLAGS) -o $@ $^

all: check_mpi bfs_omp bfs_omp_mpi

check_mpi:
ifeq ($(shell which mpicxx),)
	$(error No mpicxx found, please load OpenMPI first!)
endif

test: test.c
	@gcc -O0 -g test.c -o test

run: bfs_omp
	bash run_bfs_omp.sh

clean:
	rm -rf bfs_omp bfs_omp_mpi *~ *.*~
