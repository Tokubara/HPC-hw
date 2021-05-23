// PLEASE MODIFY THIS FILE TO IMPLEMENT YOUR SOLUTION

// Brute Force APSP Implementation:

#include "apsp.h"
#define BLOCK_SIZE 16
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

__global__ void blocked_fw_phase1(const int blockId, int n, int* const graph) {
    __shared__ int cacheGraph[BLOCK_SIZE][BLOCK_SIZE]; // 对角块的shared存储, 即使是右下角块, 空间也是开够了的
    const int idx = threadIdx.x; // 块内的列号 // TODO 我担心这样是不是太浪费寄存器了
    const int idy = threadIdx.y;

    const int v1 = BLOCK_SIZE * blockId + idy; // 整个graph矩阵的行号
    const int v2 = BLOCK_SIZE * blockId + idx;

    int newPath;

    const int cellId = v1 * n + v2; // 在graph中对应的对角块的索引
    cacheGraph[idy][idx] = (v1 < n && v2 < n)?graph[cellId]:MAX_DISTANCE; // 处理右下角的对角块太小的问题, 避免越界
    #pragma unroll
    for (int u = 0; u < BLOCK_SIZE; ++u) { // FW的外层循环
        __syncthreads(); // 下一次循环前得同步
        newPath = cacheGraph[idy][u] + cacheGraph[u][idx]; // newPath省掉能不能节省一点寄存器?

        if (newPath < cacheGraph[idy][idx]) {
            cacheGraph[idy][idx] = newPath;
        }
    }

    if (v1 < n && v2 < n) {
        graph[cellId] = cacheGraph[idy][idx]; // 写回graph
    }
}

__global__ void blocked_fw_phase2(const int blockId, const int n, int* const graph) {
    if (blockIdx.x == blockId) return; // x的范围是0到2G-1, 交叉就是对角块, 是blockId

    const int idx = threadIdx.x;
    const int idy = threadIdx.y;

    int v1 = BLOCK_SIZE * blockId + idy; // v1存的是对角块对应的位置
    int v2 = BLOCK_SIZE * blockId + idx;

    __shared__ int cacheGraphBase[BLOCK_SIZE][BLOCK_SIZE];

    // Load base block for graph and predecessors
    int cellId = v1 * n + v2;

    cacheGraphBase[idy][idx] = (v1 < n && v2 < n)?graph[cellId]:MAX_DISTANCE;

    // blockIdx.y==0对应的是行块, blockIdx.y==1对应的是列块
    if (blockIdx.y == 0) {
        v2 = BLOCK_SIZE * blockIdx.x + idx; // 行块需要修改列号, 列块需要修改行号
    } else {
        v1 = BLOCK_SIZE * blockIdx.x + idy;
    }

    __shared__ int cacheGraph[BLOCK_SIZE][BLOCK_SIZE]; // 把负责的graph块拷贝到了cacheGraph

    int currentPath;

    cellId = v1 * n + v2;
    currentPath = (v1 < n && v2 < n)?graph[cellId]:MAX_DISTANCE;

    cacheGraph[idy][idx] = currentPath; 

    __syncthreads();

    int newPath;

    if (blockIdx.y == 0) { // 处理行块
        #pragma unroll
        for (int u = 0; u < BLOCK_SIZE; ++u) {
            newPath = cacheGraphBase[idy][u] + cacheGraph[u][idx];

            if (newPath < currentPath) {
                cacheGraph[idy][idx] = newPath;
            }

            __syncthreads();
        }
    } else {
        #pragma unroll
        for (int u = 0; u < BLOCK_SIZE; ++u) {
            newPath = cacheGraph[idy][u] + cacheGraphBase[u][idx];

            if (newPath < currentPath) {
                cacheGraph[idy][idx] = newPath;
            }

            __syncthreads();
        }
    }

    if (v1 < n && v2 < n) {
        graph[cellId] = currentPath;
    }
}

__global__ void blocked_fw_phase3(const int blockId, const int n, int* const graph) {
    if (blockIdx.x == blockId || blockIdx.y == blockId) return; // 前两个阶段的块

    const int idx = threadIdx.x;
    const int idy = threadIdx.y;

    const int v1 = blockDim.y * blockIdx.y + idy; // 在graph矩阵中负责的位置
    const int v2 = blockDim.x * blockIdx.x + idx;

    __shared__ int cacheGraphBaseRow[BLOCK_SIZE][BLOCK_SIZE]; // 对应的行块
    __shared__ int cacheGraphBaseCol[BLOCK_SIZE][BLOCK_SIZE]; // 对应的列块

    int v1Row = BLOCK_SIZE * blockId + idy; // 此元素对应的行块元素的行号
    int v2Col = BLOCK_SIZE * blockId + idx;


    cacheGraphBaseRow[idy][idx] = (v1Row < n && v2 < n)?graph[v1Row * n + v2]:MAX_DISTANCE;
    cacheGraphBaseCol[idy][idx] = (v1  < n && v2Col < n)?graph[v1 * n + v2Col]:MAX_DISTANCE;

    // Synchronize to make sure the all value are loaded in virtual block
   __syncthreads();

   int cellId;
   int currentPath;
   int newPath;

   // 不需要同步, 因为用不上
   if (v1  < n && v2 < n) {
       cellId = v1 * n + v2;
       currentPath = graph[cellId];

        #pragma unroll
       for (int u = 0; u < BLOCK_SIZE; ++u) {
           newPath = cacheGraphBaseCol[idy][u] + cacheGraphBaseRow[u][idx];
           if (currentPath > newPath) {
               graph[cellId] = newPath;
           }
       }
    } 
}

void apsp(int n, /* device */ int *graph) { // graph是device内存上的
    int numBlock = (n - 1) / BLOCK_SIZE + 1;
    dim3 gridPhase1(1 ,1, 1);
    dim3 gridPhase2(numBlock, 2 , 1);
    dim3 gridPhase3(numBlock, numBlock, 1);
    dim3 dimBlockSize(BLOCK_SIZE, BLOCK_SIZE, 1);

    for (int blockID = 0; blockID < numBlock; ++blockID) {
         blocked_fw_phase1<<<gridPhase1, dimBlockSize>>>(blockID, n, graph);
         blocked_fw_phase2<<<gridPhase2, dimBlockSize>>>(blockID, n, graph);
         blocked_fw_phase3<<<gridPhase3, dimBlockSize>>>(blockID, n, graph);
    }
}

