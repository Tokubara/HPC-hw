#!/bin/bash

# run on 1 machine * 2 process * 14 threads with process binding, feel free to change it!

export OMP_NUM_THREADS=28
export OMP_PROC_BIND=true
export OMP_PLACES=cores

srun -N 4 -n 4 ./bfs_omp_mpi /home/course/hpc/assignments/2021/PA2/graph/500m.graph 

