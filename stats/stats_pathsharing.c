/**
 * Function which implements path-sharing. The path-sharing between a pair of
 * nodes, u and v, is the ratio of edges which connect the subgraph formed by
 * u and its neighbours to the subgraph formed by v and its neighbours, to all
 * such possible edges.
 *
 * Path-sharing is symmetric, i.e. sharing(u,v) == sharing(v,u)
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/edge_array.h"
#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_edge_pathsharing(graph_t *g, uint32_t u, uint32_t v) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nunbrs;
  uint32_t  nvnbrs;
  uint32_t *unbrs;
  uint32_t *vnbrs;
  double    count;
  double    divisor;
  double    ps;

  if (u == v)                         return 0;
  if (!graph_are_neighbours(g, u, v)) return 0;

  nunbrs = graph_num_neighbours(g, u);
  nvnbrs = graph_num_neighbours(g, v);
  unbrs  = graph_get_neighbours(g, u);
  vnbrs  = graph_get_neighbours(g, v);

  count   = 1;
  divisor = (nunbrs) * (nvnbrs);

  for (j = 0; j < nvnbrs; j++) {
    
    if (vnbrs[j] == u) continue;
    if (graph_are_neighbours(g, u, vnbrs[j])) count ++;
  }

  for (i = 0; i < nunbrs; i++) {

    if (unbrs[i] == v) continue;
    if (graph_are_neighbours(g, v, unbrs[i])) count ++;

    for (j = 0; j < nvnbrs; j++) {

      if (vnbrs[j] == u)                       continue; 
      if (unbrs[i] == vnbrs[j]) {divisor -= 1; continue;}

      if (graph_are_neighbours(g, unbrs[i], vnbrs[j]))
          count++;
    }
  }

  ps = count / divisor;

  stats_cache_add(g,
                  STATS_CACHE_EDGE_PATHSHARING,
                  STATS_CACHE_TYPE_EDGE,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_EDGE_PATHSHARING, u, v, &ps);

  return ps;
}
