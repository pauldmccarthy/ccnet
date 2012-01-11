/**
 * Function which counts the number of connected nodes 
 * (nodes with a degree >= 1) in a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

uint32_t stats_connected(graph_t *g) {

  uint32_t i;
  uint32_t nnodes;
  double   connected;

  connected = 0;
  nnodes    = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    if (graph_num_neighbours(g, i) > 0) connected++;
  }

  stats_cache_add(g,
                  STATS_CACHE_CONNECTED,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_CONNECTED, 0, -1, &connected);

  return (uint32_t)connected;
}
