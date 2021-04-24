#include "bfs_common.h"
#include "graph.h"
#include <cstdio>
#include <cstddef>
#include <stdlib.h>
#include <string.h>

#define ROOT_NODE_ID                    0
#define NOT_VISITED_MARKER              -1

void bfs_bottom_up(graph* graph, solution* sol) {
    int iteration = 1;
    // {{{2 初始化
    int* frontier=sol->distances; 
		memset(frontier,0xff,sizeof(int) * graph->num_nodes);
    frontier[ROOT_NODE_ID] = 0;
    bool update = true;
    while (update) {
        #pragma omp parallel for reduction(|:update)
        for (int i=0; i < graph->num_nodes; i++) {                   
            if (frontier[i] == NOT_VISITED_MARKER) {
                int start_edge = graph->incoming_starts[i];
                int end_edge = (i == graph->num_nodes-1)? graph->num_edges : graph->incoming_starts[i + 1];
                for(int neighbor = start_edge; neighbor < end_edge; neighbor++) {
                    int incoming = graph->incoming_edges[neighbor]; // incoming是起点
                    if(frontier[incoming] == iteration-1) {
                        update=true;
                        frontier[i] = iteration;                        
                        break;
                    }
                }
            }
        }
        // if(!update) {
        //   break;
        // }
        iteration++;
    }
}

void bfs_top_down(graph* graph, solution* sol) {
    int iteration = 1;
    // {{{2 初始化
    int* frontier=sol->distances; 
		memset(frontier,0xff,sizeof(int) * graph->num_nodes);
    frontier[ROOT_NODE_ID] = 0;
    bool update = true;

    while (update) {
        #pragma omp parallel for reduction(|:update)
        for (int i=0; i < graph->num_nodes; i++) {   
            if (frontier[i] == iteration-1) {
                int start_edge = graph->outgoing_starts[i];
                int end_edge = (i == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[i+1];

                for (int neighbor=start_edge; neighbor<end_edge; neighbor++) {
                    int outgoing = graph->outgoing_edges[neighbor];
                    if(frontier[outgoing] == NOT_VISITED_MARKER) {  
                        frontier[outgoing] = iteration;
                    }
                }
            }
        }
        iteration++;
    }    
}

