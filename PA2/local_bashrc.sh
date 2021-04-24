alias rsync="rsync_all -m h -f ../PA2"
alias tmp="mpirun --oversubscribe -np 4 ./bfs_omp_mpi ../graph/9p8e.graph"
alias gdb="tmpi 4 gdb ./bfs_omp_mpi"
alias objdump="objdump -d -S -l ./bfs_omp_mpi | less"

srun -N 4 -n 100 ./bfs_omp_mpi /home/course/hpc/assignments/2021/PA2/graph/68m.graph 

