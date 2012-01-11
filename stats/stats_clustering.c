/**
 * Functions which calculate the clustering 
 * coefficient of a node, or of a graph.
 *
 * Definition of clustering coefficient:
 *
 *   Watts DJ & Strogatz Sh 1998. Collective dynamics 
 *   of small world networks. Nature, 393:440-442.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_avg_clustering(graph_t *g) {

  uint32_t i;
  uint32_t numnodes;
  double   avgclust;
  double   clust;

  numnodes = graph_num_nodes(g); 
  avgclust = 0;

  for (i = 0; i < numnodes; i++) {

    clust = stats_clustering(g, i);
    avgclust += clust;
  }

  avgclust /= numnodes;

  stats_cache_add(g,
                  STATS_CACHE_GRAPH_CLUSTERING,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_GRAPH_CLUSTERING, 0, -1, &avgclust);

  return avgclust;
}

double stats_clustering(graph_t *g, uint32_t nidx) {

  uint32_t  i;
  uint32_t  j;
  uint32_t  numedges;
  uint32_t  maxedges;
  uint32_t  nneighbours;
  uint32_t *neighbours;
  double    clust;

  nneighbours = graph_num_neighbours(g, nidx);
  neighbours  = graph_get_neighbours(g, nidx);
  maxedges    = nneighbours*(nneighbours-1) / 2;
  numedges    = 0;

  if (nneighbours == 0) return 0.0;
  if (nneighbours == 1) return 1.0;

  /*
   * count the number of edges which exist 
   * between the neighbours of node nidx
   */
  for (i = 0; i < nneighbours; i++) {
    for (j = i+1; j < nneighbours; j++) {

      if (graph_are_neighbours(g, neighbours[i], neighbours[j]))
        numedges ++;
    }
  }

  clust = (double)numedges / maxedges;

  stats_cache_add(g,
                  STATS_CACHE_NODE_CLUSTERING,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_NODE_CLUSTERING, nidx, -1, &clust);

  return clust;
}
