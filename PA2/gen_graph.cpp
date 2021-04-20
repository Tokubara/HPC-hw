#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>

#include <iostream>
#include <omp.h>
#include <sstream>
#include <sys/time.h>
#include <vector>

#include "bfs_common.h"
#include "graph.h"

#define literal(x) #x

int main(int argc, char **argv) {

  std::string graph_filename;

  if (argc < 2) {
    std::cerr << "Usage: ./bfs_omp <path/to/graph/file> \n";
    exit(1);
  }
  graph_filename = argv[1];
  size_t dot_pos=graph_filename.rfind('.');

  Graph g;

  g = load_graph(argv[1]);
  printf("storing binary form of graph!\n");
  store_graph_binary(graph_filename.substr(0,dot_pos).append(".graph").c_str(), g);
  printf("Graph stats:\n");
  printf("  Edges: %d\n", g->num_edges);
  printf("  Nodes: %d\n", g->num_nodes);

  return 0;
}
