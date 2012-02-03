/**
 * Functions which calculate the degree of a node, or of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_avg_degree(graph_t *g) {

  uint32_t nnodes;
  uint32_t nedges;

  nnodes = graph_num_nodes(g);
  nedges = graph_num_edges(g);

  return (2.0*nedges) / nnodes;
}

double stats_max_degree(graph_t *g) {

  uint64_t i;
  uint32_t nnodes;
  uint32_t ideg;
  double   max;

  max    = 0;
  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    ideg = stats_degree(g, i);

    if (ideg > max) max = ideg;
  }

  stats_cache_add(g,
                  STATS_CACHE_MAX_DEGREE,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_MAX_DEGREE, 0, -1, &max);

  return max;
}

uint32_t stats_degree(graph_t *g, uint32_t nidx) {

  return graph_num_neighbours(g, nidx);
}
