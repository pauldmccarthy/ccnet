/**
 * Function which calculates the density of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"

double stats_density(graph_t *g) {

  double numnodes;
  double numedges;
  double maxedges;

  numnodes = graph_num_nodes(g);
  numedges = graph_num_edges(g);

  maxedges = numnodes*(numnodes-1) / 2.0;

  return (numedges / maxedges);
}
