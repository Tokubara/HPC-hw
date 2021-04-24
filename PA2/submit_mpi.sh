# {{{1 输出文件的位置
if ! [[ -d log ]]; then
	mkdir log
fi

result_file=log/result_mpi_log.txt
echo -n > $result_file # 清空文件内容

# {{{1 运行参数控制
export OMP_PLACES=cores
ass=/home/course/hpc/assignments/2021/PA2/


declare -a config
config=(1 1 1 2 1 4 1 14 1 28 2 1 2 2 2 4 2 14 2 28 4 1 4 2 4 4 4 14 4 28)
for((i=0;i<${#config[@]};i+=2)) do
	N=${config[$i]}
	P=${config[$(($i+1))]}
  n=$((N*P))
  T=$((28/P))
  export OMP_NUM_THREADS=$T
	# echo printf "%d %d %d " $N $P $T
	# echo srun -N $N -n $n --cpu-bind sockets ./bfs_omp_mpi $ass/graph/68m.graph # 因为main中去掉了printf的\n
	printf "%d %d %d " $N $P $T >> $result_file
	srun -N $N -n $n --cpu-bind sockets ./bfs_omp_mpi $ass/graph/68m.graph >> $result_file # 因为main中去掉了printf的\n
	# echo >> $result_file # 好似是因为前面没有输出\n, 我觉得这里是多余的
done
echo done
