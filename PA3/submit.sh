echo -n > result_mine_32_log.txt
# echo -n > result_ref_log.txt

for n in 1000 2500 5000 7500 10000; do
    srun -N 1 ./benchmark $n >> result_mine_32_log.txt
    # srun -N 1 ./benchmark_ref $n >> result_ref_log.txt
done
