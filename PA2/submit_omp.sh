# {{{1 输出文件的位置
if ! [[ -d log ]]; then
	mkdir log
fi

result_file=log/result_opm_log.txt
echo -n > $result_file # 清空文件内容

# {{{1 运行参数控制
config=( 1 7 14 28 )
export OMP_PLACES=cores
ass=/home/course/hpc/assignments/2021/PA2/
for i in ${config[@]};do
  export OMP_NUM_THREADS=$i
  # echo threads $OMP_NUM_THREADS
  # echo srun -n 1 ./bfs_omp $ass/graph/68m.graph
  srun -n 1 ./bfs_omp $ass/graph/68m.graph >> $result_file
done
echo done
