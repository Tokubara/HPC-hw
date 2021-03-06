#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <iostream>
#include <omp.h>
#include <sstream>
#include <sys/time.h>
#include <vector>

#include "bfs_common.h"
#include "graph.h"

#define USE_BINARY_GRAPH 1 // 用二进制图格式, 还是文本图格式

void bfs_top_down(Graph graph, solution *sol);
void bfs_hybrid(Graph graph, solution *sol);
void bfs_bottom_up(Graph graph, solution *sol);

#if M==1
#define bfs_omp bfs_bottom_up
#elif M==2
#define bfs_omp bfs_hybrid
#else
#define bfs_omp bfs_top_down
#endif

#define literal(x) #x

int main(int argc, char **argv) {

  std::string graph_filename;

  if (argc < 2) {
    std::cerr << "Usage: ./bfs_omp <path/to/graph/file> \n";
    exit(1);
  }

  graph_filename = argv[1];

  Graph g;

  printf("----------------------------------------------------------\n");
  printf("Max system threads = %d\n", omp_get_max_threads());
  printf("----------------------------------------------------------\n");

  printf("Loading graph...\n");
  if (USE_BINARY_GRAPH) {
    g = load_graph_binary(graph_filename.c_str());
  } else {
    g = load_graph(argv[1]);
    printf("storing binary form of graph!\n");
    store_graph_binary(graph_filename.append(".graph").c_str(), g);
    exit(1);
  }
  printf("\n");
  printf("Graph stats:\n");
  printf("  Edges: %d\n", g->num_edges);
  printf("  Nodes: %d\n", g->num_nodes);

  solution sol1;
  sol1.distances = (int *)malloc(sizeof(int) * g->num_nodes); // 存放答案的地方
  solution sol2;
  sol2.distances = (int *)malloc(sizeof(int) * g->num_nodes);
  bfs_omp(g, &sol1);
  bfs_serial(g, &sol2); // 标准答案, 用于对拍
  for (int j = 0; j < g->num_nodes; j++) {
    if (sol1.distances[j] != sol2.distances[j]) {
      printf("*** Results disagree at %d: %d, %d\n", j, sol1.distances[j],
             sol2.distances[j]);
      exit(1);
    }
  }
	puts("No difference");
  int repeat = 2;
  unsigned long total_time = 0.0;
  for (int i = 0; i < repeat; ++i) { // 这里专门用来测时间的?
    timeval start, end;
    gettimeofday(&start, NULL);
    bfs_omp(g, &sol2);
    gettimeofday(&end, NULL);
    total_time +=
        1000000.0 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
  }
  printf("Average execution time of function " literal(bfs_omp) " is %lf ms.\n",
         total_time / 1000.0 / repeat);

  return 0;
}
