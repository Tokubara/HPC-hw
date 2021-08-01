# {{{1 输出文件的位置
if ! [[ -d log ]]; then
	mkdir log
fi

result_file=log/omp_thread_num_log.txt
# echo -n > $result_file # 清空文件内容

# {{{1 运行参数控制
config=( 5 10 15 20 25 )
export OMP_PROC_BIND=true
export OMP_PLACES=cores
ass=/home/course/hpc/assignments/2021/PA2/
# ass=../
for i in ${config[@]};do
  export OMP_NUM_THREADS=$i
  # echo threads $OMP_NUM_THREADS
  # echo srun -n 1 ./bfs_omp $ass/graph/68m.graph
  # ./bfs_omp $ass/graph/500k.graph >> $result_file
  printf "thread_num:%d, time:" "$i"
  srun -n 1 ./bfs_omp $ass/graph/500m.graph
  # if (( $(echo "$result > 1500" | bc -l ) )); then
    # printf "thread_num:%d, time:%f\n" "$i" "$result" >> $result_file
  # fi
    # echo printf "thread_num:%d, time:%f\n" "$i" "$result"
done
echo done

