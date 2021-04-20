override CXXFLAGS += -std=c++17 -fopenmp -O2 -g3 -Wall -Wextra -pedantic

headers := $(wildcard *.h)
common_src := bfs_common.cpp graph.cpp

.PHONY: clean check_mpi

N := 4
n := 100
graph := 68m.graph

bfs_omp_mpi: main_omp_mpi.cpp bfs_omp_mpi.cpp $(common_src)
	mpicxx $(CXXFLAGS) -o $@ $^

bfs_omp: main_omp.cpp bfs_omp.cpp $(common_src)
	g++ $(CXXFLAGS) -o $@ $^

all: check_mpi bfs_omp bfs_omp_mpi

check_mpi:
ifeq ($(shell which mpicxx),)
	$(error No mpicxx found, please load OpenMPI first!)
endif

gen: gen_graph.cpp $(common_src)
	g++ -g -O0 -o $@ $^

test: test.cpp
	g++ -O0 -g $< -o $@

runm: bfs_omp_mpi
	bash ./run_bfs_mpi.sh
	# srun -N $(N) -n $(n) $< /home/course/hpc/assignments/2021/PA2/graph/$(graph)

runo: bfs_omp
	bash ./run_bfs_omp.sh

.DEFAULT_GOAL=bfs_omp_mpi

clean:
	rm -rf bfs_omp bfs_omp_mpi *~ *.*~
