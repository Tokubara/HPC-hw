#ifndef WORKER_H
#define WORKER_H

#ifndef DEBUG
#define NDEBUG
#endif

#include <mpi.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>

/* #ifndef NDEBUG */
#define assertSuccess(err)                                                    \
    {                                                                         \
        if (err != MPI_SUCCESS) {                                             \
            char errStr[100];                                                 \
            int strLen;                                                       \
            MPI_Error_string(err, errStr, &strLen);                           \
            printf("Err 0x%X in line %d : %s\n", int(err), __LINE__, errStr); \
            abort();                                                          \
        }                                                                     \
    }
/* #else */
/* #define assertSuccess(err) */
/* #endif */
#define CHKERR(func)             \
    {                            \
        int _errCode = (func);   \
        assertSuccess(_errCode); \
    }
// 调用的例子为: CHKERR(MPI_File_open(MPI_COMM_WORLD, input_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &in_file)); 接受一个函数调用, 如果返回正常(==MPI_SUCCESS), 那就跟直接调用函数没区别, 如果不是, 就得到错误字符串, 并且打印在文件中的位置

//  这个函数的作用是, 返回ceilng(x/y)
template <typename T, typename P>
inline T ceiling(T x, P y) {
    return (x + y - 1) / y;
}

class Worker {
    // you may use the following variables
   private:
    int nprocs, rank;
    size_t n, block_len; // data数组的长度
    float *data;
    bool last_rank, out_of_range;

    // you need to implement this function
   public:
    void sort();

    // don't touch the following variables & functions
   private:
    size_t IO_offset;
    MPI_File in_file, out_file;

   public:
    Worker(size_t _n, int _nprocs, int _rank) : nprocs(_nprocs), rank(_rank), n(_n) {
        size_t block_size = ceiling(n, nprocs);
        IO_offset = block_size * rank; // 负责的数组的起始区域
        out_of_range = IO_offset >= n; // 可能出现这种情况?
        last_rank = IO_offset + block_size >= n;
        if (!out_of_range) {
            block_len = std::min(block_size, n - IO_offset); // 对数组的最后一段长度不到block_size的处理
            data = new float[block_len]; //? 为什么是float类型?
        } else {
            block_len = 0; //? 为什么如此处理? 为什么不是直接null?
            data = nullptr;
        }
    }

    ~Worker() {
        if (!out_of_range) {
            delete[] data;
        }
    }

    void input(const char *input_name);
    int check();
};

#endif  // WORKER_H
