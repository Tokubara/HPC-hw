#include "bfs_common.h"

#include "graph.h"
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1 // 是-1是因为如果两点之间不存在, 就会是-1

void vertex_set_clear(vertex_set *list) { list->count = 0; }

// vertex_set_init(&list1, graph->num_nodes);
void vertex_set_init(vertex_set *list, int count) {
  list->max_vertices = count;
  list->vertices = (int *)malloc(sizeof(int) * list->max_vertices);
  vertex_set_clear(list);
}

// 更新new_frontier, 为当前frontier未被标记过的相邻点集
void top_down_step(Graph g, vertex_set *frontier, vertex_set *new_frontier,
                   int *distances) {

  for (int i = 0; i < frontier->count; i++) {

    int node = frontier->vertices[i];

    int start_edge = g->outgoing_starts[node];
    int end_edge = (node == g->num_nodes - 1) ? g->num_edges
                                              : g->outgoing_starts[node + 1];

    // attempt to add all neighbors to the new frontier
    for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
      int outgoing = g->outgoing_edges[neighbor];

      if (distances[outgoing] == NOT_VISITED_MARKER) {
        distances[outgoing] = distances[node] + 1;
        int index = new_frontier->count++;
        new_frontier->vertices[index] = outgoing;
      }
    }
  }
}

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_serial(Graph graph, solution *sol) {

  vertex_set list1;
  vertex_set list2;
  vertex_set_init(&list1, graph->num_nodes);
  vertex_set_init(&list2, graph->num_nodes);

  vertex_set *frontier = &list1;
  vertex_set *new_frontier = &list2;

  // initialize all nodes to NOT_VISITED
  for (int i = 0; i < graph->num_nodes; i++)
    sol->distances[i] = NOT_VISITED_MARKER;

  // setup frontier with the root node
  frontier->vertices[frontier->count++] = ROOT_NODE_ID; // 初始点集只有根节点一个, count=1, 点号为根节点的点号
  sol->distances[ROOT_NODE_ID] = 0;

  while (frontier->count != 0) {
    vertex_set_clear(new_frontier); // count赋为0, 不需要清空数组, 直接被写了
    top_down_step(graph, frontier, new_frontier, sol->distances);

    // swap pointers
    vertex_set *tmp = frontier;
    frontier = new_frontier;
    new_frontier = tmp;
  }
}
