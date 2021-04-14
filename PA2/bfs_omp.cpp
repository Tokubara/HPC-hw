#include "bfs_common.h"
#include "graph.h"
#include <cstdio>
#include <cstddef>
#include <stdlib.h>
#include <string.h>

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1 // 是-1是因为如果两点之间不存在, 就会是-1

void omp_top_down_step(Graph g, vertex_set *frontier, vertex_set *new_frontier, int *distances);

extern void vertex_set_clear(vertex_set *list);

// vertex_set_init(&list1, graph->num_nodes);
extern void vertex_set_init(vertex_set *list, int count);

void bfs_omp(Graph graph, solution *sol) {
  vertex_set list1;
  vertex_set list2;
  vertex_set_init(&list1, graph->num_nodes);
  vertex_set_init(&list2, graph->num_nodes);

  vertex_set *frontier = &list1;
  vertex_set *new_frontier = &list2;

	memset(sol->distances, 0xff, graph->num_nodes<<2);

  // setup frontier with the root node
  frontier->vertices[frontier->count++] = ROOT_NODE_ID; // 初始点集只有根节点一个, count=1, 点号为根节点的点号
  sol->distances[ROOT_NODE_ID] = 0;

  while (frontier->count != 0) {
    new_frontier->count=0; // count赋为0, 不需要清空数组, 直接被写了
    omp_top_down_step(graph, frontier, new_frontier, sol->distances);

    // swap pointers
    vertex_set *tmp = frontier;
    frontier = new_frontier;
    new_frontier = tmp;
  }
}

// 更新new_frontier, 为当前frontier未被标记过的相邻点集
void omp_top_down_step(Graph g, vertex_set *frontier, vertex_set *new_frontier,
                   int *distances) {

#pragma omp parallel for
  for (int i = 0; i < frontier->count; i++) {

    int node = frontier->vertices[i];

    int start_edge = g->outgoing_starts[node];
    int end_edge = (node == g->num_nodes - 1) ? g->num_edges
                                              : g->outgoing_starts[node + 1];

    for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
      int outgoing = g->outgoing_edges[neighbor];

      if (__sync_bool_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, distances[node] + 1)) {
        int index = new_frontier->count+1;
        new_frontier->vertices[index] = outgoing;
#pragma omp atomic
				new_frontier->count++;
      }
    }
  }
}

