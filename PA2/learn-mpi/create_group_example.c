#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef DEBUG
#define DEBUG 0
#endif
int main(int argc, char** argv)
{
#include "globalinit.c" // 不知道这有啥用,  我猜测是做一些例行工作,  比如获得nprocs
  if (nprocs < 4) {
    fprintf(stderr, "This program needs at least four processes\n");
    return -1;
  }
  if (nprocs % 2 > 0) {
    fprintf(stderr, "This program needs an even number of processes\n");
    return -1;
  }
  int color, colors = 2; //? 两个相似变量名的区别是什么? 第一个color是split用来创建通信域的
  // {{{1 创建两个子通信域
  MPI_Comm split_half_comm;
  int mydata = procno;
  // create sub communicator first & second half 
  color = procno<nprocs/2 ? 0 : 1; // 划分依据并不是奇数偶数, 而是前一半, 后一半
  int key = procno; // 感觉这里是多余的
  if (color == 0)
    // first half rotated
    key = (procno + 1) % (nprocs / 2); // nprocs是4, 0,1->1,0 换序了, 感觉这个key有点奇怪
  else
    // second half numbered backwards
    key = nprocs - procno;
  MPI_Comm_split(MPI_COMM_WORLD, color, key, &split_half_comm); // split以后, 都以为自己在split_half_comm这个通信域中, 只是不同的进程, 这个变量中包含的进程不同
 // 
  int sub_procno, sub_nprocs;
  MPI_Comm_rank(split_half_comm, &sub_procno);
  MPI_Comm_size(split_half_comm, &sub_nprocs);
  if (DEBUG)
    fprintf(stderr, "[%d] key=%d local rank: %d\n", procno, key, sub_procno);
 //?! 通信的关键
  int
      local_leader_in_inter_comm
      = color == 0 ? 2 : (sub_nprocs - 2), // 0号group, 是2, 1号group, 是sub_nprocs - 2, 不知道为什么这么奇怪
      local_number_of_other_leader
      = color == 1 ? 2 : (sub_nprocs - 2); // 注意与上面对换了一下
  if (local_leader_in_inter_comm < 0 || local_leader_in_inter_comm >= sub_nprocs) {
    fprintf(stderr,
        "[%d] invalid local member: %d\n",
        procno, local_leader_in_inter_comm);
    MPI_Abort(2, comm);
  }
  int
      global_rank_of_other_leader
      = 1 + (procno < nprocs / 2 ? nprocs / 2 : 0); //? 这个变量是干啥的?
  int
      i_am_local_leader
      = sub_procno == local_leader_in_inter_comm, // 所以local_leader_in_inter_comm应该存的的确就是在local通信域中的rank
      inter_tag = 314;
  if (i_am_local_leader)
    fprintf(stderr, "[%d] creating intercomm with %d\n",
        procno, global_rank_of_other_leader);
  MPI_Comm intercomm; //? 这里是又要创建一个通信域么?
  MPI_Intercomm_create(/* local_comm:*/ split_half_comm,
      /* local_leader:*/ local_leader_in_inter_comm,
      /* peer_comm: */ MPI_COMM_WORLD,
      /* remote_peer_rank: */ global_rank_of_other_leader, //? 为什么需要对等方? 我在这里有一种感觉, 那就是intercomm不会只能是两个通信域玩吧, 说的一点也不错, 只能是两个通信域创建一个
      /* tag:*/ inter_tag,
      /* newintercomm:*/ &intercomm);

  if (DEBUG)
    fprintf(stderr, "[%d] intercomm created.\n", procno);
  if (i_am_local_leader) {
    int inter_rank, inter_size;
    MPI_Comm_size(intercomm, &inter_size);
    MPI_Comm_rank(intercomm, &inter_rank);
    if (DEBUG)
      fprintf(stderr, "[%d] inter rank/size: %d/%d\n", procno, inter_rank, inter_size);
  }
  double interdata = 0.;
  if (i_am_local_leader) {
    if (color == 0) {
      interdata = 1.2;
      int inter_target = local_number_of_other_leader;
      printf("[%d] sending interdata %e to %d\n",
          procno, interdata, inter_target);
      MPI_Send(&interdata, 1, MPI_DOUBLE, inter_target, 0, intercomm); // 注意通信域是intercomm
    } else {
      MPI_Status status;
      MPI_Recv(&interdata, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, intercomm, &status); //? 这是什么情况?从recv是点对点这一点来看, 我觉得是只有leader会收到
      int inter_source = status.MPI_SOURCE;
      printf("[%d] received interdata %e from %d\n",
          procno, interdata, inter_source);
      if (inter_source != local_number_of_other_leader)
        fprintf(stderr,
            "Got inter communication from unexpected %d; s/b %d\n", inter_source, local_number_of_other_leader);
    }
  }
  int root;
  int bcast_data = procno;
  if (color == 0) { // sending group: the local leader sends
    if (i_am_local_leader)
      root = MPI_ROOT;
    else
      root = MPI_PROC_NULL;
  } else { // receiving group: everyone indicates leader of other group root = local_number_of_other_leader;
  }
  if (DEBUG)
    fprintf(stderr, "[%d] using root value %d\n", procno, root);
  MPI_Bcast(&bcast_data, 1, MPI_INT, root, intercomm); //? 这个效果是啥? 这个函数和其它函数一样, 应该不会专门为intercomm设计吧, 但这样就与点对点通信完全没差别了
  fprintf(stderr, "[%d] bcast data: %d\n", procno, bcast_data);
  // 如果真的没差别, 应该只会输出两次
  if (procno == 0)
    fprintf(stderr, "Finished\n");
  MPI_Finalize();
  return 0;
}
