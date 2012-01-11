/**
 * Provides a function which calculates an approximation of the clustering
 * coefficient of a graph.
 *
 *   Schank T & Wagner D 2005. Approximating Clustering
 *   Coefficient and Transitivity. Journal of Graph
 *   Algorithms and Applications, 9:2:265-275
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Randomly selects and tests a triple (a set of 3 connected nodes) from the
 * graph.
 *
 * \return non-0 if that triple forms a triangle (the 3 nodes are fully
 * connected), 0 otherwise.
 */
static uint8_t _test_next_triple(
  graph_t *g /**< the graph */
);

double stats_approx_clustering(graph_t *g, uint32_t ntriples) {

  uint64_t i;
  uint32_t ntriangles;
  double   clust;

  for (i = 0, ntriangles = 0; i < ntriples; i++) {

    if (_test_next_triple(g)) ntriangles ++;
  }

  clust = (double)ntriangles/ntriples;

  stats_cache_add(g,
                  STATS_CACHE_APPROX_CLUSTERING,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_APPROX_CLUSTERING, 0, -1, &clust);

  return clust;
}

uint8_t _test_next_triple(graph_t *g) {

  uint32_t  n;
  uint32_t  u;
  uint32_t  v;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;

  /*
   * 1. select a node, n, at random
   * 2. select two of n's neighbours, u and v, at random
   * 3. return true if u and v are neighbours of each other, 
   *    return false otherwise
   */

  nnodes = graph_num_nodes(g);

  nnbrs = 0;
  while (nnbrs < 2) {
    n     = ((uint32_t)rand()) % nnodes;
    nnbrs = graph_num_neighbours(g, n);
    nbrs  = graph_get_neighbours(g, n);
  }

  u = ((uint32_t)rand()) % nnbrs;
  v = u;

  while (v == u) {

    v = ((uint32_t)rand()) % nnbrs;
  }

  u = nbrs[u];
  v = nbrs[v];

  return graph_are_neighbours(g,u,v) ? 1 : 0;
}
