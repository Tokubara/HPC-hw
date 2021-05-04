alias rsync="rsync_all -m h -f ../PA2"
alias tmp="mpirun --oversubscribe -np 2 ./bfs_omp_mpi ../graph/9p8e.graph"
alias gdb="tmpi r gdb ./bfs_omp_mpi"
alias objdump="objdump -d -S -l ./bfs_omp_mpi | less"
awk 'NR==1 {first=$4} 1<=NR && NR <=5 {one[NR]=$4} {printf "%s, speedup=%f, speedup_node=%f\n",$0,first/$4, one[NR%5==0?5:NR%5]/$4}' result_mpi_top_down_log.txt
alias out="rsync_all -m h -f ../PA2"
alias in="rsync_all -m h -r -f PA2 -t ../PA2"


# srun -N 4 -n 100 ./bfs_omp_mpi /home/course/hpc/assignments/2021/PA2/graph/68m.graph 

