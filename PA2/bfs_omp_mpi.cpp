#include "bfs_common.h"
#include "graph.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mpi.h>

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1 // 是-1是因为如果两点之间不存在, 就会是-1

void bfs_omp_mpi(Graph graph, solution* sol)
{
  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);  
  // MPI初始化已经做过了
  // TODO: 正确性测试的时候, 进程数可能是任意, 这样就会有问题, 这个地方需要修改, 还要考虑进程通信能不能进行
  int m_size = sqrt(nprocs); // 进程矩阵大小
  // {{{1 MPI划分行列通信域
  // 矩阵排列大概是这样:
  // 0 1 2 3
  // 4 5 6 7...
  int col_no = rank % m_size; // 这也是col_color
  int row_no = rank / m_size; // 这也是row_color
  MPI_Comm col_comm, row_comm;
  MPI_Comm_split(MPI_COMM_WORLD, col_no, row_no, &col_comm); // 注意这里的key并不是原来的rank, 而是行号, 也就是说, 应该在0到m_size之间, split以后, 都以为自己在这个通信域中, 只是不同的进程, 这个变量中包含的进程不同
  MPI_Comm_split(MPI_COMM_WORLD, row_no, col_no, &row_comm);
  // {{{1 本进程矩阵大小, 和负责的点的范围
  int n_proc = (graph->num_nodes + m_size - 1) / m_size; // 每行每列进程最多负责的点数, 但是最后一行/列进程负责的点数不一样多
  // 矩阵大小, row_num*col_num
  int row_num = (row_no + 1 == m_size) ? (graph->num_nodes - (m_size - 1) * n_proc) : n_proc; // 其实应该也是graph->num_nodes%m_size
  int col_num = (col_no + 1 == m_size) ? (graph->num_nodes - (m_size - 1) * n_proc) : n_proc;
  int stp_st = col_no * n_proc;
  int stp_end = stp_st + col_num; // 负责的始点的范围, 这是由列决定的, 右开
  int endp_st = row_no * n_proc;
  // int endp_end = endp_st + row_num; // 负责的终点的范围, 右开, 没用上过
  // {{{1 创建xk, visited数组(长度与xk一样)
  bool* xk = (bool*)malloc(sizeof(bool) * col_num);         // 这里我开始写的是row_num
  memset(xk, 0, sizeof(bool) * col_num);                  //? 这里可不可以是sizeof(xk)?
  bool* new_xk = (bool*)malloc(sizeof(bool) * row_num);     // 这是后面存矩阵计算结果的地方, 但它和xk大小本来就不一样,  不初始化, 因为每一次循环开始时会初始化
  bool* visited = (bool*)malloc(sizeof(bool) * col_num); // visited是矩阵计算中会使用的
  memset(visited, -1, sizeof(bool) * col_num);
  bool update = false;
  // {{{1 初始化xk(就是src点), 相当于第一次迭代
  xk[ROOT_NODE_ID] = col_no == 0 ? true : false;
  sol->distances[ROOT_NODE_ID] = 0;
  int iter = 1;
  // {{{1 while循环
  while (true) {
    // {{{2 矩阵运算
    update = false;
    memset(new_xk, 0, sizeof(bool) * row_num);
    for (int i = 0; i < row_num; i++) { //? 这个循环范围应该是什么, 是end, 注意, 得到的结果的向量长度是与终点范围一样多的
      // [endp_st, endp_end)
      // 外层循环, 计算new_xk[i]
      if (visited[i])
        continue;
      int start_edge = graph->incoming_starts[i + endp_st]; // 注意, 这个地方错了的
      int end_edge = (i + endp_st == graph->num_nodes - 1) ? graph->num_edges : graph->incoming_starts[i + endp_st + 1];
      for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
        int incoming = graph->incoming_edges[neighbor];                              // incoming是起点
        if (incoming >= stp_st && incoming < stp_end && xk[incoming - stp_st]) { // 如果这样写条件判断, 好像就不需要每次让xk为0,1向量了, >0就行了
          new_xk[i] = true;
          update = true;
          break;
        }
      }
    }
    // 是否退出的逻辑
    MPI_Allreduce(MPI_IN_PLACE, &update, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);
    if (!update) {
      break;
    }
    // {{{2 new_xk和distance, 还有visited的维护
    MPI_Reduce(MPI_IN_PLACE, new_xk, row_num, MPI_C_BOOL, MPI_BOR, row_no, row_comm); // 接收方是对角线
    if (col_no == row_no) {                                                        // 这就是leader
      for (int i = 0; i < row_num; i++) {
        // {{{2 leader更新visited, sol
        if (new_xk[i]) {
          ASSERT(!visited[i]);
          visited[i] = true;
          sol->distances[i + endp_st] = iter; //? 这里会不会有data race?我觉得不会
        }
      }
      // TODO 也可以考虑先allreduce, 再每一个计算
      memcpy(xk, new_xk, sizeof(bool) * row_num);
          // TODO Bcast visited
          // {{{2 leader bcast send
    }
      MPI_Bcast(visited, row_num, MPI_C_BOOL, row_no, row_comm); //? 这个地方总感觉有错?
      MPI_Bcast(xk, col_num, MPI_C_BOOL, col_no, col_comm); //? 这个地方总感觉有错?
      iter++;
  }
  // {{{1 清理工作
  free(xk);
  free(new_xk);
  free(visited);
}
