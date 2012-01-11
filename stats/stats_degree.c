/**
 * Functions which calculate the degree of a node, or of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"

double stats_avg_degree(graph_t *g) {

  uint32_t nnodes;
  uint32_t nedges;

  nnodes = graph_num_nodes(g);
  nedges = graph_num_edges(g);

  return (2.0*nedges) / nnodes;
}

uint32_t stats_degree(graph_t *g, uint32_t nidx) {

  return graph_num_neighbours(g, nidx);
}
