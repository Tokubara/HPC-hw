#include <mpi.h>
#include <sys/time.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "worker.h"

void Worker::input(const char *input_name) {
#ifndef NDEBUG
    if (!out_of_range) {
        printf("Process %d handles [%lu, %lu)\n", rank, IO_offset, IO_offset + block_len);
    } else {
        printf("Process %d is out of range, skipping...\n", rank);
    }
#endif
    // read 0 bytes is fine
    CHKERR(MPI_File_open(MPI_COMM_WORLD, input_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &in_file));
    CHKERR(MPI_File_read_at_all(in_file, IO_offset * sizeof(float), (void *)data, block_len, MPI_FLOAT,
                                MPI_STATUS_IGNORE)); //? 为什么这里用的是MPI_File_read_at_all, 而不是MPI_File_read_at?
    CHKERR(MPI_File_close(&in_file));
}

/**
 * 判断sort排序结果的正误
 * */
int Worker::check() {
    // directly skip if out of range
    if (out_of_range) return 1;

    int ret_val = 1;
    /** Check intra-process data */
    for (size_t i = 0; i < block_len - 1; i++) {
        if (data[i] > data[i + 1]) {
            ret_val = -1;
#ifndef NDEBUG
            printf("Intra-process wrong at rank %d: data[%lu] = %f > data[%lu] = %f\n", rank, i, data[i], i + 1,
                   data[i + 1]);
#endif
            break;
        }
    }
    /** Check inter-process data */
    float L = -1; // 用于接收的buffer
    if (nprocs > 1) { // i把排序的最后一段送到i+1的进程, 由于0只发, 最后一个只收, 因此需要单独处理
        if (rank == 0) {
            MPI_Send(&data[block_len - 1], 1, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD); // 只发送最后一个, 发送到1
        } else if (this->last_rank) {
            MPI_Recv(&L, 1, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, NULL); // 从相邻的1个接收, 也就是倒数第二个
            if (L - data[0] > 1e-15) {
                ret_val = -1;
#ifndef NDEBUG
                printf("Inter-process wrong at rank %d: %f > %f\n", rank, data[0], L);
#endif
            }
        } else {
            MPI_Recv(&L, 1, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, NULL);
            if (L - data[0] > 1e-15) {
                ret_val = -1;
#ifndef NDEBUG
                printf("Inter-process wrong at rank %d: %f > %f\n", rank, data[0], L);
#endif
            }
            MPI_Send(&data[block_len - 1], 1, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD);
        }
    }
    return ret_val;
}
