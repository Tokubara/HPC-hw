#!/bin/bash
set -e

# source /home/spack/spack/share/spack/setup-env.sh

# spack load openmpi

# make -j 4

job_name=./odd_even_sort

num=100000000
data_file=data/$num.dat

declare -a srun_config
srun_config=(1 1 1 2 1 4 1 16 2 32)
for((i=0;i<${#srun_config[@]};i+=2)) do
	echo srun -N ${srun_config[$i]} -n ${srun_config[$(($i+1))]} --cpu-bind sockets $job_name $num $data_file
done

# run mpi_pow
# srun -N 1 -n 1   --cpu-bind sockets $job_name $1 $data_file
# srun -N 1 -n 2   --cpu-bind sockets $job_name $1 $data_file
# srun -N 1 -n 4  --cpu-bind sockets $job_name $1 $data_file
# srun -N 1 -n 16  --cpu-bind sockets $job_name $1 $data_file
# srun -N 2 -n 32  --cpu-bind sockets $job_name $1 $data_file

echo All done!

