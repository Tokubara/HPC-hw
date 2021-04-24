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
        update = false;
        #pragma omp for reduction(|:update)
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
        iteration++;
    }
}



// void top_down_step(
//     graph* graph,
//     vertex_set* frontier,    
//     int* distances,    
//     int iteration)
// {
//     #pragma omp parallel 
//     {
//         #pragma omp for
//         for (int i=0; i < graph->num_nodes; i++) {   
//             if (frontier[i] == iteration) {
//                 int start_edge = graph->outgoing_starts[i];
//                 int end_edge = (i == graph->num_nodes-1) ? graph->num_edges : graph->outgoing_starts[i+1];

//                 for (int neighbor=start_edge; neighbor<end_edge; neighbor++) {
//                     int outgoing = graph->outgoing_edges[neighbor];
//                     if(frontier[outgoing] == NOT_VISITED_MARKER) {  
//                         distances[outgoing] = distances[i] + 1; // 这里可能有data race, 也就是几个线程同时发现了outgoing可达, 但无所谓, 因为distances[i]+1是一样的
//                         frontier[outgoing] = iteration + 1;
//                     }
//                 }
//             }
//         }
//     }
// }

// void bfs_top_down(graph* graph, solution* sol) {
//     vertex_set list1;
//     vertex_set_init(&list1, graph->num_nodes);    
//     int iteration = 1; // 为什么是1, 因为初始化present为0, 如果初始化为-1, 就可以是0
//     vertex_set* frontier = &list1;

// 		int len=graph->num_nodes;
// 		int i;
// 		// #pragma parallel for // 运行结果看没啥差别
// 		// for(i=0; i<len;i++) {
// 		// 	frontier->vertices[i]=0;
// 		// }
// 		// #pragma parallel for
// 		// for(i=0; i<len;i++) {
// 		// 	sol->distances[i]=-1;
// 		// }

// 	 	memset(frontier, 0, sizeof(int) * graph->num_nodes);
// 		memset(sol->distances,0xff,sizeof(int) * graph->num_nodes);
//     frontier[frontier->count++] = iteration;
//     sol->distances[ROOT_NODE_ID] = 0;    

//     while (frontier->count != 0) {
//         frontier->count = 0;
//         top_down_step(graph, frontier, sol->distances, iteration);
//         iteration++;
//     }    
// }

