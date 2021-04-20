#include "bfs_common.h"
#include "graph.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mpi.h>
#include <algorithm>

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
  MPI_Comm col_comm, row_comm, diag_comm;
  MPI_Comm_split(MPI_COMM_WORLD, col_no, row_no, &col_comm); // 注意这里的key并不是原来的rank, 而是行号, 也就是说, 应该在0到m_size之间, split以后, 都以为自己在这个通信域中, 只是不同的进程, 这个变量中包含的进程不同
  MPI_Comm_split(MPI_COMM_WORLD, row_no, col_no, &row_comm);
  int diag_color=col_no==row_no;
  MPI_Comm_split(MPI_COMM_WORLD, diag_color, col_no, &diag_comm);

  // {{{1 本进程矩阵大小, 和负责的点的范围
  int n_proc = (graph->num_nodes + m_size - 1) / m_size; // 每行每列进程最多负责的点数, 但是最后一行/列进程负责的点数不一样多
  // 矩阵大小, row_num*col_num
  int row_num = (row_no + 1 == m_size) ? (graph->num_nodes - (m_size - 1) * n_proc) : n_proc; // 其实应该也是graph->num_nodes%m_size
  int col_num = (col_no + 1 == m_size) ? (graph->num_nodes - (m_size - 1) * n_proc) : n_proc;
  int stp_st = col_no * n_proc;
  int stp_end = stp_st + col_num; // 负责的始点的范围, 这是由列决定的, 右开
  int endp_st = row_no * n_proc;
  int endp_end = endp_st + row_num; // 负责的终点的范围, 右开, 没用上过
  // {{{1 创建xk, visited, new_xk数组(长度与xk一样)
  int* xk = (int*)malloc(sizeof(int) * col_num);         // 用稀疏存法, 只存非0下标
  int xk_len=0;
  int our_xk_len;
  int* new_xk = (int*)malloc(sizeof(int) * row_num); 
  int new_xk_len;
  bool* visited = (bool*)malloc(sizeof(bool) * row_num); // visited是矩阵计算中会使用的, 并且不会通信, 每个进程各自算
  // {{{1 唯二需要memset的:visited, sol->distances
  memset(visited, 0, sizeof(bool) * row_num);
  memset(sol->distances, -1, sizeof(int)*graph->num_nodes);

  // 添加row_index_arr和send_buf
  int* row_index_arr;
  int* displace;
  int* send_buf;
  bool update;
  int recvcount_sum;
  if(col_no==row_no) {
    row_index_arr = (int*)malloc(sizeof(int)*m_size);
    displace = (int*)malloc(sizeof(int)*m_size);
    send_buf = (int*)malloc(sizeof(int)*row_num*m_size); // 最坏可能是要*m_size, 但其余矩阵不是
  } else {
    send_buf = (int*)malloc(sizeof(int)*row_num);
  }
  // {{{1 相当于第一次迭代, 设置xk和visited(就是src点)
  if(col_no==0) {
    xk[xk_len++]=ROOT_NODE_ID;
    visited[ROOT_NODE_ID] = true;
  }
  sol->distances[ROOT_NODE_ID] = 0; // 虽然其它的进程本来也不会负责这部分
  int iter = 1;
  while (true) {
    // {{{1 while的初始化
    MPI_Request request_col = MPI_REQUEST_NULL;
    MPI_Request request_row = MPI_REQUEST_NULL;
    new_xk_len = 0;
    // {{{1 矩阵运算
// #pragma omp parallel for // 在这里会有new_xk_len数据竞争的问题, 用临界区大概会更慢 
    // TODO 看看这里xk_len是不是正确设置了
    for (int i = 0; i < xk_len; i++) { //? 这个循环范围应该是什么, 是end, 注意, 得到的结果的向量长度是与终点范围一样多的
      // [endp_st, endp_end)
      // 外层循环, 计算new_xk[i]
      int out_point = xk[i];
      int start_edge = graph->outgoing_starts[out_point];      int end_edge = (out_point == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[out_point+1];
      for (int neighbor=start_edge; neighbor<end_edge; neighbor++) {
        // TODO 看看这里visited设置好没有
        int outgoing = graph->outgoing_edges[neighbor];
        if(outgoing>=endp_st && outgoing < endp_end && !visited[outgoing-endp_st]) {
          new_xk[new_xk_len++]=outgoing;
        }
      }
    }
    // {{{1 是否退出的逻辑
    update=new_xk_len>0;
    MPI_Allreduce(MPI_IN_PLACE, &update, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);     
    if(!update) {
      break;
    }
    // {{{1 发送new_xk_len和new_xk, 集中到对角线, gather没有inplace操作
    MPI_Gather(&new_xk_len, 1, MPI_INT, row_index_arr, 1, MPI_INT, row_no, row_comm); // 这个地方似乎可以改为仅root才需要这个数组
    if(row_no==col_no) {
      displace[0]=0;
      recvcount_sum=0;
      int i;
      for(i = 0; i<m_size-1; i++) {
        displace[i+1]=displace[i]+row_index_arr[i];
        recvcount_sum+=row_index_arr[i];
      }
      recvcount_sum+=row_index_arr[i];
    }
    MPI_Gatherv(new_xk, new_xk_len, MPI_INT, send_buf, row_index_arr, displace, MPI_INT, row_no, row_comm);
    // {{{1 对角线处理new_xk并发送xk和xk_len, 维护distances, 
    if (col_no == row_no) {                                                        // 这就是leader
      // {{{2 处理new_xk
      std::sort(send_buf, send_buf+recvcount_sum);
      our_xk_len=xk_len=(int*)std::unique(send_buf, send_buf+recvcount_sum)-send_buf;
     }
    MPI_Bcast(&our_xk_len, 1, MPI_INT, row_no, row_comm); // 为了知道send_buf有多大
    // xk_len不能是异步的吧, 否则接收方都不知道要接受多大
    MPI_Ibcast(send_buf, our_xk_len, MPI_INT, row_no, row_comm, &request_row); // send_buf用于设置visited
    if (col_no == row_no) {
      memcpy(xk,send_buf,xk_len*sizeof(int)); 
      for(int i = 0; i<xk_len; i++) {
        sol->distances[send_buf[i]]=iter;
      }
    }
    MPI_Bcast(&xk_len, 1, MPI_INT, col_no, col_comm);
    MPI_Ibcast(xk, xk_len, MPI_INT, col_no, col_comm, &request_col); // TODO 看看改成异步的
    MPI_Wait(&request_row, MPI_STATUSES_IGNORE);
    for(int i = 0; i<our_xk_len; i++) {
      visited[send_buf[i]-endp_st]=true;
    }
    MPI_Wait(&request_col, MPI_STATUSES_IGNORE);
    iter++;
  }
  // {{{1 gatherv
  if(rank==ROOT_NODE_ID) {
    MPI_Reduce(MPI_IN_PLACE, sol->distances, graph->num_nodes, MPI_INT, MPI_MAX, ROOT_NODE_ID, diag_comm); // 接收方是对角线
  } else if(col_no==row_no) {
    MPI_Reduce(sol->distances, sol->distances, graph->num_nodes, MPI_INT, MPI_MAX, ROOT_NODE_ID, diag_comm); // 接收方是对角线
  }
  // {{{1 清理工作
  MPI_Comm_free(&row_comm);
  MPI_Comm_free(&col_comm);
  MPI_Comm_free(&diag_comm);
  free(xk);
  free(new_xk);
  free(visited);
  if(col_no==row_no) {
    free(row_index_arr);
    free(displace);
  }
  free(send_buf);
}
