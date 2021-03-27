#!/bin/bash
# set -e

source /home/spack/spack/share/spack/setup-env.sh

spack load openmpi

make -j 4

job_name=./odd_even_sort

data_file=data/$1.dat

# run mpi_pow
srun -N 1 -n 1   --cpu-bind sockets $job_name $1 $data_file
srun -N 1 -n 2   --cpu-bind sockets $job_name $1 $data_file
srun -N 1 -n 4  --cpu-bind sockets $job_name $1 $data_file
srun -N 1 -n 16  --cpu-bind sockets $job_name $1 $data_file
srun -N 2 -n 32  --cpu-bind sockets $job_name $1 $data_file

echo All done!

