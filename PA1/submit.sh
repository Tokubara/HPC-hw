#!/bin/bash
set -e

source /home/spack/spack/share/spack/setup-env.sh

spack load openmpi

make -j 4

job_name=./odd_even_sort

num=100000000
data_file=data/$num.dat
if ! [[ -d log ]]; then
	mkdir log
fi
result_file=log/result_$num_log.txt

rm -f result_log.txt && touch $result_file

declare -a srun_config
srun_config=(1 1 1 2 1 4 1 16 2 32)
for((i=0;i<${#srun_config[@]};i+=2)) do
	Nparam=${srun_config[$i]}
	nparam=${srun_config[$(($i+1))]}
	printf "%d %d " $Nparam $nparam >> $result_file
	srun -N $Nparam -n $nparam --cpu-bind sockets $job_name $num $data_file >> $result_file # 因为main中去掉了printf的\n
	echo -e "\n" >> $result_file
done

# {{{1 处理log结果
awk -f awk.sh $result_file

# run mpi_pow
# srun -N 1 -n 1   --cpu-bind sockets $job_name $1 $data_file
# srun -N 1 -n 2   --cpu-bind sockets $job_name $1 $data_file
# srun -N 1 -n 4  --cpu-bind sockets $job_name $1 $data_file
# srun -N 1 -n 16  --cpu-bind sockets $job_name $1 $data_file
# srun -N 2 -n 32  --cpu-bind sockets $job_name $1 $data_file

echo All done!

