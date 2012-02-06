/**
 * Functions which calculate measures of centrality  of nodes in a graph.
 * 
 * The following measures are implemented:
 *
 *   - Degree centrality, the ratio of a node's degree to its possible degree.
 * 
 *   - Closeness centrality, the inverse of the average shortest path from a
 *     node to all other nodes.
 * 
 *   - Betweenness centrality, the ratio of shortest paths which contain this 
 *     node to all shortest paths, between every pair of nodes in the graph.
 *
 *   Freeman LC 1979 Centrality in Social Networks: 
 *   Conceptual Clarification. Social Networksw, 
 *   1:3:215-239
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <stdlib.h>
#include <float.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_degree_centrality(graph_t *g, uint32_t nidx) {

  double nnodes;
  double nnbrs;

  nnodes = graph_num_nodes(g);
  nnbrs  = graph_num_neighbours(g, nidx);

  return nnbrs / (nnodes-1);
}

double stats_closeness_centrality(graph_t *g, uint32_t nidx) {

  double    closeness;
  double    avg_pathlen;

  /*
   * Closeness centrality for a node i is (n-1)/(sli), where sli is the sum of
   * distances from node i to all other nodes in the graph. For a connected
   * graph, sli is equal to the average path length for node i (ali),
   * multiplied by (n-1). Thus, closeness centrality is (n-1)/(ali(n-1)), or
   * 1/ali.
   *
   * If the graph is not connected, closeness values will be bogus, but
   * closeness centrality values are meaningless for disconnected graphs
   * anyway.
   */
  stats_cache_node_pathlength(g, nidx, &avg_pathlen);
  if (avg_pathlen < 0) return -1;

  if (avg_pathlen > 0) closeness = 1/avg_pathlen;
  else                 closeness = 0;

  return closeness;
}

double stats_betweenness_centrality(graph_t *g, uint32_t v) {

  int64_t   s;
  int64_t   t;
  uint32_t  count;
  uint32_t  nnodes;
  uint32_t  stlen;
  uint32_t  svlen;
  uint32_t  vtlen;
  double   *vnumpaths;
  double   *snumpaths;
  double   *vpaths;
  double   *spaths;
  double    ratio;
  double    betweenness;

  betweenness = 0;
  count       = 0;
  vnumpaths   = NULL;
  snumpaths   = NULL;
  vpaths      = NULL;
  spaths      = NULL;
  nnodes      = graph_num_nodes(g);

  if (graph_num_neighbours(g, v) == 0) return 0;

  vnumpaths = calloc(nnodes,sizeof(double));
  if (vnumpaths == NULL) goto fail;

  snumpaths = calloc(nnodes,sizeof(double));
  if (snumpaths == NULL) goto fail;

  vpaths = calloc(nnodes,sizeof(double));
  if (vpaths == NULL) goto fail;

  spaths = calloc(nnodes,sizeof(double));
  if (spaths == NULL) goto fail;

  stats_cache_pair_pathlength(g, v, vpaths);
  stats_cache_pair_numpaths(  g, v, vnumpaths);

  for (s = 0; s < nnodes; s++) {
    
    if (s         == v) continue;
    if (vpaths[s] == 0) continue;

    count++;
    stats_cache_pair_pathlength(g, s, spaths);
    snumpaths[0] = DBL_MAX;
    
    for (t = s+1; t < nnodes; t++) {
      
      if (t         == v) continue;
      if (vpaths[t] == 0) continue;
      if (spaths[t] == 0) continue;

      stlen = spaths[t];
      svlen = spaths[v];
      vtlen = vpaths[t];

      /*
       * Bellman criterion - A vertex v lies on a
       * shortest path between vertices s and t if
       * and only if len(s,t) = len(s,v) + len(v,t)
       */
      if (svlen + vtlen != stlen) continue;

      ratio = vnumpaths[s] * vnumpaths[t];

      if (snumpaths[0] == DBL_MAX)
        stats_cache_pair_numpaths(g, s, snumpaths);
      ratio = ratio / snumpaths[t];
      
      betweenness += ratio;
    }
  }

  betweenness = (2*betweenness) / ((nnodes-1)*(nnodes-2));

  stats_cache_add(g,
                  STATS_CACHE_BETWEENNESS_CENTRALITY,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(double));
  stats_cache_update(
    g, STATS_CACHE_BETWEENNESS_CENTRALITY, v, -1, &betweenness);

  free(vnumpaths);
  free(snumpaths);
  free(vpaths);
  free(spaths);
  return betweenness;
  
fail:
  if (vnumpaths != NULL) free(vnumpaths);
  if (snumpaths != NULL) free(snumpaths);
  if (vpaths    != NULL) free(vpaths);
  if (spaths    != NULL) free(spaths);
  return -1;
}
