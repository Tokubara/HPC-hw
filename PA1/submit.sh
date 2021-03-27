#!/bin/bash
# {{{1 加载环境
set -e

source /home/spack/spack/share/spack/setup-env.sh

spack load openmpi

make -j 4

# {{{1 输入
job_name=./odd_even_sort
num=100000000
data_file=data/$num.dat

# {{{1 输出文件的位置
if ! [[ -d log ]]; then
	mkdir log
fi

result_file=log/result_${num}_log.txt
rm -f $result_file && touch $result_file

# {{{1 运行
declare -a srun_config
srun_config=(1 1 1 2 1 4 1 16 2 32)
for((i=0;i<${#srun_config[@]};i+=2)) do
	Nparam=${srun_config[$i]}
	nparam=${srun_config[$(($i+1))]}
	printf "%d %d " $Nparam $nparam >> $result_file
	srun -N $Nparam -n $nparam --cpu-bind sockets $job_name $num $data_file >> $result_file # 因为main中去掉了printf的\n
	echo >> $result_file
done

# {{{1 处理log结果
awk -f awk.sh $result_file

echo All done!

