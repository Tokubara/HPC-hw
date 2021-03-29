// {{{1 headers
#include "worker.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <mpi.h>
#include <stdint.h>

// {{{1 为了使用MPI的size_t
#if SIZE_MAX == UCHAR_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
#error "what is happening here?"
#endif

void Worker::sort() {
  // {{{1 设置odd_rank,even_rank, 也就是partner序号
  MPI_Status status;
  float * temp;
  int odd_rank, even_rank;

  std::sort(data, data + block_len);

  if (rank % 2 == 0) {
    odd_rank = rank - 1;
    even_rank = rank + 1;
  } else {
    odd_rank = rank + 1;
    even_rank = rank - 1;
  }
  if (odd_rank == -1 || odd_rank == nprocs)
    odd_rank = MPI_PROC_NULL;
  if (even_rank == -1 || even_rank == nprocs)
    even_rank = MPI_PROC_NULL;

  // {{{1 获得parter的block_len
  size_t odd_len, even_len; // 分别是odd/even partner的长度
  int LEN_TAG = 2;
  MPI_Sendrecv( & block_len, 1, my_MPI_SIZE_T, odd_rank, 2, & odd_len, 1, my_MPI_SIZE_T, odd_rank, LEN_TAG, MPI_COMM_WORLD, & status);
  MPI_Sendrecv( & block_len, 1, my_MPI_SIZE_T, even_rank, 2, & even_len, 1, my_MPI_SIZE_T, even_rank, LEN_TAG, MPI_COMM_WORLD, & status);
  // odd_len与even_len可能是无效的
  // 开辟buffer, 用于存储buffer
  float * partner_buffer = new float[ceiling(n, nprocs)]; // 这里可以用堆,也可以用栈,但是怀疑栈有上限,堆没有上限,那就用堆?
  temp = new float[block_len];
  // {{{1 迭代归并
  for (int p = 0; p < nprocs - 1; p++) {
    // {{{2 收发数据, 存储data到temp, 为归并做准备
    size_t partner_len = (p % 2 == 1) ? odd_len : even_len;
    int partner_rank = (p % 2 == 1) ? odd_rank : even_rank;
    MPI_Sendrecv(data, block_len, MPI_FLOAT, partner_rank, 1, partner_buffer,
      partner_len, MPI_FLOAT, partner_rank, 1, MPI_COMM_WORLD, & status);

    for (size_t i = 0; i < block_len; i++) {
      temp[i] = data[i];
    }
    if (status.MPI_SOURCE == MPI_PROC_NULL)
      continue;
    else if (rank < status.MPI_SOURCE) {
      // {{{2 如果是自己rank较小的, 归并得到小的部分
      size_t i, j, k;
      for (i = j = k = 0; k < block_len; k++) {
        data[k] = (j == partner_len || (i < block_len && temp[i] < partner_buffer[j])) ? temp[i++] : partner_buffer[j++];
      }
    } else {
      // {{{2 如果是自己rank较大的, 归并得到大的部分
      long i, j, k; // 按理说也该用size_t, 但是需要-1
      for (i = k = block_len - 1, j = partner_len - 1; k >= 0; k--) {
        data[k] = (j == -1 || (i >= 0 && temp[i] >= partner_buffer[j])) ? temp[i--] : partner_buffer[j--];
      }
    }
  }
  // {{{1 释放buffer
  delete[] partner_buffer;
  delete[] temp;
}
