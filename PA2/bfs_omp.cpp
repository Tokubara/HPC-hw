#include "bfs_common.h"
#include "graph.h"
#include <cstdio>
#include <cstddef>
#include <stdlib.h>
#include <string.h>

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1 // 是-1是因为如果两点之间不存在, 就会是-1

#define ROOT_NODE_ID                    0
#define NOT_VISITED_MARKER              -1
#define BOTTOMUP_NOT_VISITED_MARKER     0
#define PADDING                         16
#define THRESHOLD                       10000000




void vertex_set_clear(vertex_set* list);

void vertex_set_init(vertex_set* list, int count);


// bottom up代表的是这样一种思路: 遍历所有的点, 如果它还是没遇到过的状态, 看那些能到达它的点, 有哪一个就在这一轮的点集中
void bottom_up_step(
    graph* g,
    vertex_set* frontier,    
    int* distances,
    int iteration) { // 怀疑iteration是level, 初始iteration是1
    
    // #pragma omp parallel num_threads(NUM_THREADS) private(local_count) 
    // #pragma omp parallel private(local_count)    
    // #pragma omp parallel num_threads(NUM_THREADS) private(local_count) 
    int local_count = 0;
    int padding[15];
    #pragma omp parallel 
    {
        #pragma omp for reduction(+:local_count)
        for (int i=0; i < g->num_nodes; i++) {                   
            if (frontier->vertices[i] == BOTTOMUP_NOT_VISITED_MARKER) {
                int start_edge = g->incoming_starts[i];
                int end_edge = (i == g->num_nodes-1)? g->num_edges : g->incoming_starts[i + 1];
                for(int neighbor = start_edge; neighbor < end_edge; neighbor++) {
                    int incoming = g->incoming_edges[neighbor]; // incoming是起点
                    // if(__sync_bool_compare_and_swap(&frontier->vertices[incoming], iteration, distances[node] + 1)) {
                    if(frontier->vertices[incoming] == iteration) {
                        distances[i] = distances[incoming] + 1;                        
                        local_count ++;
                        frontier->vertices[i] = iteration + 1;
                        break; // 怀疑这个break才是bottom_up的核心
                    }
                }
            }
        }
        // #pragma omp atomic
        //     frontier->count += local_count;
    }    
    frontier->count = local_count;

}

void bfs_bottom_up(graph* graph, solution* sol)
{

    vertex_set list1;
    
    vertex_set_init(&list1, graph->num_nodes);
    
    int iteration = 1;

    vertex_set* frontier = &list1; 
    
    memset(frontier->vertices, 0, sizeof(int) * graph->num_nodes); // 看框架, 感觉这是不需要的, 而且似乎可以直接用sizeof(graph->num_nodes)
            
		memset(sol->distances,0xff,sizeof(int) * graph->num_nodes);
    // setup frontier with the root node    
    // just like put the root into queue
    frontier->vertices[frontier->count++] = 1;

    // set the root distance with 0
    sol->distances[ROOT_NODE_ID] = 0;
    /* for (int i=0; i<graph->num_nodes; i++) */
    /*     sol->distances[i] = 0; //? 怎么能初始化为1? 难道不应该是-1么? */

    
    // printf("!!!!!!!!!!!!!!!!!!!!fro2: %-10d\n", frontier->count);
    // just like pop the queue
    while (frontier->count != 0) {
        
        frontier->count = 0;
        // double start_time = CycleTimer::currentSeconds();
        

        bottom_up_step(graph, frontier, sol->distances, iteration);

        // double end_time = CycleTimer::currentSeconds();
        // printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);

        iteration++;

    }

    
}



void top_down_step(
    graph* g,
    vertex_set* frontier,    
    int* distances,    
    int iteration)
{
    int local_count = 0; 
    int padding[15];  
    #pragma omp parallel 
    {
        #pragma omp for reduction(+ : local_count)
        for (int i=0; i < g->num_nodes; i++) {   
            if (frontier->vertices[i] == iteration) { //? 这一步会不会很慢
                int start_edge = g->outgoing_starts[i];
                int end_edge = (i == g->num_nodes-1) ? g->num_edges : g->outgoing_starts[i+1];

                for (int neighbor=start_edge; neighbor<end_edge; neighbor++) {
                    int outgoing = g->outgoing_edges[neighbor]; // 具有局部性
                    if(frontier->vertices[outgoing] == BOTTOMUP_NOT_VISITED_MARKER) {  
                        distances[outgoing] = distances[i] + 1; // 这里可能有data race, 也就是几个线程同时发现了outgoing可达, 但无所谓, 因为distances[i]+1是一样的
                        local_count ++;
                        frontier->vertices[outgoing] = iteration + 1;
                    }
                }
            }
        }
    }
    frontier->count = local_count; // count的含义还是没有变的, 仍然是new_frontier的点数
}








// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
// void bfs_bottom_up(graph* graph, solution* sol) {
void bfs_top_down(graph* graph, solution* sol) {
    vertex_set list1;
    vertex_set_init(&list1, graph->num_nodes);    
    int iteration = 1; // 为什么是1, 因为初始化present为0, 如果初始化为-1, 就可以是0
    vertex_set* frontier = &list1;
    memset(frontier->vertices, 0, sizeof(int) * graph->num_nodes);
    frontier->vertices[frontier->count++] = iteration;
    // set the root distance with 0
		memset(sol->distances,0xff,sizeof(int) * graph->num_nodes);
    sol->distances[ROOT_NODE_ID] = 0;    
    // just like pop the queue
    while (frontier->count != 0) {
        frontier->count = 0;
        top_down_step(graph, frontier, sol->distances, iteration);
        iteration++;
    }    
}






void bfs_hybrid(graph* graph, solution* sol) {

    vertex_set list1;
    
    vertex_set_init(&list1, graph->num_nodes);
    
    int iteration = 1;

    vertex_set* frontier = &list1;    
        
    // setup frontier with the root node    
    // just like put the root into queue
    memset(frontier->vertices, 0, sizeof(int) * graph->num_nodes);

    frontier->vertices[frontier->count++] = 1;

		memset(sol->distances,0xff,sizeof(int) * graph->num_nodes);
    // set the root distance with 0
    sol->distances[ROOT_NODE_ID] = 0;
    
    // just like pop the queue
    while (frontier->count != 0) {
        
        
        // double start_time = CycleTimer::currentSeconds();
        
        if(frontier->count >= THRESHOLD) {
            frontier->count = 0;
            bottom_up_step(graph, frontier, sol->distances, iteration);
        }
        else {
            frontier->count = 0;
            top_down_step(graph, frontier, sol->distances, iteration);
        }

        // double end_time = CycleTimer::currentSeconds();
        // printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);

        iteration++;


    }     
}

