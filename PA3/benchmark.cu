// DO NOT MODIFY THIS FILE

#include <ctime>
#include <cstdio>
#include <cstdint>
#include <chrono>

#include "cuda_utils.h"
#include "apsp.h"

namespace {

constexpr int DATA_RANGE = 10000;

constexpr int TIMER_ROUNDS = 2;
constexpr int TIMER_WARMUP = 1;

// i,j号线程写data[i][j]
__global__ void __launch_bounds__(1024) genDataKernel(int n, int seed, int *data) {
    int64_t x = seed;
    x = (x * 179424673 + blockIdx.x + 275604541) % DATA_RANGE;
    x = (x * 373587883 + threadIdx.x + 472882027) % DATA_RANGE;
    x = (x * 179424673 + blockIdx.y + 275604541) % DATA_RANGE;
    x = (x * 373587883 + threadIdx.y + 472882027) % DATA_RANGE;
    auto i = blockIdx.y * 32 + threadIdx.y;
    auto j = blockIdx.x * 32 + threadIdx.x;
    if (i < n && j < n) {
        data[i * n + j] = i == j ? 0 : x;
    }
}

__global__ void __launch_bounds__(1024) compareKernel(int n, int *data1, int *data2, int *diff) {
    auto i = blockIdx.x * 1024 + threadIdx.x;
    if (i < n) {
        if (data1[i] != data2[i]) {
            atomicAdd(diff, 1); // 感觉这里不原子也没什么关系, 反正在乎的只是对不对
        }
    }
}

// cudaMalloc n*n个点
/* device */ int *allocGraph(int n) {
    int *data;
    CHK_CUDA_ERR(cudaMalloc(&data, n * n * sizeof(int)));
    return data;
}

void copyGraph(int n, /* device */ int *dst, /* device */ const int *src) {
    CHK_CUDA_ERR(cudaMemcpy(dst, src, n * n * sizeof(int), cudaMemcpyDefault));
}

// 虽然这里写着device, 但我看这是host调用的函数啊
/* device */ int *genData(int n) {
    int *data = allocGraph(n);
    dim3 thr(32, 32);
    dim3 blk((n - 1) / 32 + 1, (n - 1) / 32 + 1);
    genDataKernel<<<blk, thr>>>(n, time(0), data);
    CHK_CUDA_ERR(cudaDeviceSynchronize());
    return data;
}

} // Anonymous namespace

int main(int argc, char **argv) {
    int n;
    if (argc != 2 || sscanf(argv[1], "%d", &n) != 1) {
        printf("Usage: %s <graph_size>", argv[0]); //? 图大小是点数? 是边数?
        exit(-1);
    }

    int *data = genData(n);
    int *result = allocGraph(n);
    // 这里是在warmup
    for (int i = 0; i < TIMER_WARMUP; i++) {
        copyGraph(n, result, data); //? 为什么要把data拷贝给result, result难道不应该被写么?
        apsp(n, result);
        CHK_CUDA_ERR(cudaDeviceSynchronize());
    }
    double t = 0;
    //? 这里好像是在测时间?
    for (int i = 0; i < TIMER_ROUNDS; i++) {
        namespace ch = std::chrono;
        copyGraph(n, result, data);
        auto beg = ch::high_resolution_clock::now();
        apsp(n, result);
        auto err = cudaDeviceSynchronize();
        auto end = ch::high_resolution_clock::now();
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA Error\n");
            exit(-1);
        }
        t += ch::duration_cast<ch::duration<double>>(end - beg).count() * 1000; // ms
    }
    t /= TIMER_ROUNDS;

    // 这里应该是检查正确性
    int *ref = allocGraph(n);
    copyGraph(n, ref, data);
    apspRef(n, ref);
    CHK_CUDA_ERR(cudaDeviceSynchronize());
    int *diff, diffHost = 0;
    CHK_CUDA_ERR(cudaMalloc(&diff, sizeof(int)));
    CHK_CUDA_ERR(cudaMemcpy(diff, &diffHost, sizeof(int), cudaMemcpyDefault)); // 也就是device的diff初始化为0, 我感觉要这样写还是因为kernel不能有返回值
    compareKernel<<<(n * n - 1) / 1024 + 1, 1024>>>(n * n, result, ref, diff);
    CHK_CUDA_ERR(cudaMemcpy(&diffHost, diff, sizeof(int), cudaMemcpyDefault));
    if (diffHost == 0) {
        printf("Validation Passed\n");
    } else {
        printf("WRONG ANSWER!!!\n");
        exit(-1);
    }

    printf("Time: %f ms\n", t);
}

