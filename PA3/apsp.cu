// PLEASE MODIFY THIS FILE TO IMPLEMENT YOUR SOLUTION

// Brute Force APSP Implementation:

#include "apsp.h"
#define BLOCK_SIZE 32
#define MAX_DISTANCE (1<<30-1)

namespace { // 为什么要namespace?

// __global__ void kernel(int n, int k, int *graph) {
//     auto i = blockIdx.y * blockDim.y + threadIdx.y;
//     auto j = blockIdx.x * blockDim.x + threadIdx.x;
//     if (i < n && j < n) {
//         graph[i * n + j] = min(graph[i * n + j], graph[i * n + k] + graph[k * n + j]);
//     }
// }

}

__global__ void blocked_fw_phase1(const int block_id, int n, int* const graph) {
    __shared__ int cache_diag[BLOCK_SIZE][BLOCK_SIZE]; // 对角块的shared存储, 即使是右下角块, 空间也是开够了的
    const int idx = threadIdx.x; // 块内的列号 // TODO 我担心这样是不是太浪费寄存器了
    const int idy = threadIdx.y;

    const int global_row_id = BLOCK_SIZE * block_id + idy; // 整个graph矩阵的行号
    const int global_col_id = BLOCK_SIZE * block_id + idx;

    int new_len;

    const int global_id = global_row_id * n + global_col_id; // 在graph中对应的对角块的索引
    cache_diag[idy][idx] = (global_row_id < n && global_col_id < n)?graph[global_id]:MAX_DISTANCE; // 处理右下角的对角块太小的问题, 避免越界
    #pragma unroll
    for (int k = 0; k < BLOCK_SIZE; ++k) { // FW的外层循环
        __syncthreads(); // 下一次循环前得同步
        new_len = cache_diag[idy][k] + cache_diag[k][idx]; // new_len省掉能不能节省一点寄存器?

        if (new_len < cache_diag[idy][idx]) {
            cache_diag[idy][idx] = new_len;
        }
    }

    if (global_row_id < n && global_col_id < n) {
        graph[global_id] = cache_diag[idy][idx]; // 写回graph
    }
}

__global__ void blocked_fw_phase2(const int block_id, const int n, int* const graph) {
    if (blockIdx.x == block_id) return; // x的范围是0到2G-1, 交叉就是对角块, 是block_id

    const int idx = threadIdx.x;
    const int idy = threadIdx.y;

    int global_row_id = BLOCK_SIZE * block_id + idy; // global_row_id存的是对角块对应的位置
    int global_col_id = BLOCK_SIZE * block_id + idx;

    __shared__ int cache_diag[BLOCK_SIZE][BLOCK_SIZE];
    // Load base block for graph and predecessors
    int global_id = global_row_id * n + global_col_id;
    cache_diag[idy][idx] = (global_row_id < n && global_col_id < n)?graph[global_id]:MAX_DISTANCE;

    // blockIdx.y==0对应的是行块, blockIdx.y==1对应的是列块
    if (blockIdx.y == 0) {
        global_col_id = BLOCK_SIZE * blockIdx.x + idx; // 行块需要修改列号, 列块需要修改行号
    } else {
        global_row_id = BLOCK_SIZE * blockIdx.x + idy;
    }
    global_id = global_row_id * n + global_col_id;

    __shared__ int cache_self[BLOCK_SIZE][BLOCK_SIZE]; // 把负责的graph块拷贝到了cache_self
    int cur_len = (global_row_id < n && global_col_id < n)?graph[global_id]:MAX_DISTANCE;
    cache_self[idy][idx] = cur_len;  // 载入共享内存

    __syncthreads();

    int new_len;

    if (blockIdx.y == 0) { // 处理行块
        #pragma unroll
        for (int k = 0; k < BLOCK_SIZE; ++k) {
            new_len = cache_diag[idy][k] + cache_self[k][idx];

            if (new_len < cur_len) {
                cur_len = new_len;
            }

            cache_self[idy][idx] = cur_len;
            __syncthreads();
        }
    } else {
        #pragma unroll
        for (int k = 0; k < BLOCK_SIZE; ++k) {
            new_len = cache_self[idy][k] + cache_diag[k][idx];

            if (new_len < cur_len) {
                cur_len = new_len;
            }

            cache_self[idy][idx] = cur_len;
            __syncthreads();
        }
    }

    if (global_row_id < n && global_col_id < n) {
        graph[global_id] = cur_len;
    }
}

__global__ void blocked_fw_phase3(const int block_id, const int n, int* const graph) {
    if (blockIdx.x == block_id || blockIdx.y == block_id) return; // 前两个阶段的块

    const int idx = threadIdx.x;
    const int idy = threadIdx.y;

    const int global_row_id = blockDim.y * blockIdx.y + idy; // 在graph矩阵中负责的位置
    const int global_col_id = blockDim.x * blockIdx.x + idx;
    const int cache_row_id = BLOCK_SIZE * block_id + idy;
    const int cache_col_id = BLOCK_SIZE * block_id + idx;

    __shared__ int cache_row[BLOCK_SIZE][BLOCK_SIZE]; // 对应的行块
    __shared__ int cache_col[BLOCK_SIZE][BLOCK_SIZE]; // 对应的列块

    cache_row[idy][idx] = (cache_row_id < n && global_col_id < n)?graph[cache_row_id * n + global_col_id]:MAX_DISTANCE; // 写对应的行块
    cache_col[idy][idx] = (global_row_id  < n && cache_col_id < n)?graph[global_row_id * n + cache_col_id]:MAX_DISTANCE; // 写对应的列块

   __syncthreads();
   
   if (global_row_id  < n && global_col_id < n) {
       int global_id = global_row_id * n + global_col_id;
       int cur_len = graph[global_id];

       int new_len;

        #pragma unroll
       for (int k = 0; k < BLOCK_SIZE; ++k) {
           new_len = cache_col[idy][k] + cache_row[k][idx];
           if (cur_len > new_len) {
               cur_len = new_len;
           } // 不需要同步, 因为用不上
       }
       graph[global_id] = cur_len;
    } 
}

void apsp(int n, /* device */ int *graph) { // graph是device内存上的
    int num_block = (n - 1) / BLOCK_SIZE + 1;
    dim3 grid_phase1(1 ,1, 1);
    dim3 grid_phase2(num_block, 2 , 1);
    dim3 grid_phase3(num_block, num_block, 1);
    dim3 block_size(BLOCK_SIZE, BLOCK_SIZE, 1);

    for (int blockID = 0; blockID < num_block; ++blockID) {
         blocked_fw_phase1<<<grid_phase1, block_size>>>(blockID, n, graph);
         blocked_fw_phase2<<<grid_phase2, block_size>>>(blockID, n, graph);
         blocked_fw_phase3<<<grid_phase3, block_size>>>(blockID, n, graph);
    }
}

