/**
 * Provides a function to calculate the global and local efficiency for
 * a graph, or for a node in a graph.
 *
 * Definition of efficiency:
 *
 *   Massimo Marchiori, Vito Latora 2001. Efficient behaviour of 
 *   small-world networks. Physical Review Letters 87(19):198701
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/bfs.h"
#include "util/array.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"



/**
 * Callback function for the breadth first search. Updates the inverse
 * accumulation.
 *
 * \return 0 always.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< current search state      */
  void        *context /**< pointer to a double value */
);

double stats_global_efficiency(graph_t *g) {

  uint64_t i;
  uint64_t j;
  uint32_t nnodes;
  double  *pathlens;
  double   effic;

  pathlens = NULL;
  effic    = 0;
  nnodes   = graph_num_nodes(g);

  pathlens = malloc(nnodes*sizeof(double));
  if (pathlens == NULL) goto fail;

  for (i = 0; i < nnodes; i++) {

    stats_cache_pair_pathlength(g, i, pathlens);

    for (j = i+1; j < nnodes; j++) {
      if (pathlens[j] == 0) continue;
      effic += 1 / pathlens[j];
    }
  }

  /*
   * g is assumed to be undirected - only
   * half of the node-pairs were evaluated,
   * hence we multiply the result by 2
   */
  effic = 2*effic/(nnodes*(nnodes-1));
  
  free(pathlens);

  stats_cache_add(g,
                  STATS_CACHE_GLOBAL_EFFICIENCY,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_GLOBAL_EFFICIENCY, 0, -1, &effic);
  return effic;
  
fail:
  if (pathlens != NULL) free(pathlens);
  return -1;
}


double stats_avg_local_efficiency(graph_t *g) {

  uint64_t i;
  uint32_t nnodes;
  double   loceff;
  double   loceff_tally;

  nnodes       = graph_num_nodes(g);
  loceff_tally = 0;

  for (i = 0; i < nnodes; i++) {
    if (stats_cache_node_local_efficiency(g, i, &loceff)) goto fail;

    loceff_tally += loceff;
  }

  loceff_tally /= nnodes;

  stats_cache_add(g,
                  STATS_CACHE_LOCAL_EFFICIENCY,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_LOCAL_EFFICIENCY, 0, -1, &loceff_tally);

  return loceff_tally;

fail: 
  return -1;
}

double stats_sub_efficiency(graph_t *g, uint32_t nnodes, uint8_t *mask) {

  uint32_t  i;
  uint32_t  gnnodes;
  double    invsum;
  double    inv;
  double    effic;

  invsum  = 0;
  gnnodes = graph_num_nodes(g);

  for (i = 0; i < gnnodes; i++) {

    if (mask[i]) continue;

    inv = 0;
    if (bfs(g, &i, 1, mask, &inv, NULL, _bfs_cb, NULL)) goto fail;

    if (inv < 0) goto fail;
    invsum += inv;
  }

  effic = invsum / (nnodes*(nnodes-1));

  return effic;

fail:
  return -1;
}


double stats_local_efficiency(graph_t *g, uint32_t nidx) {

  uint32_t  i;
  uint32_t  numnodes;
  uint32_t  numnbrs;
  uint32_t *nbrs;
  uint8_t  *subgraphmask;
  double    invsum;
  double    inv;
  double    effic;

  invsum       = 0;
  subgraphmask = NULL;
  numnodes     = graph_num_nodes(     g);
  numnbrs      = graph_num_neighbours(g, nidx);
  nbrs         = graph_get_neighbours(g, nidx);

  if (numnbrs == 0 || numnbrs == 1) return 0.0;

  subgraphmask = malloc(numnodes*sizeof(uint8_t));
  if (subgraphmask == NULL) goto fail;

  /*
   * Fudge the subgraphmask to limit the search to
   * the subgraph formed by the neighbours of nidx
   */
  for (i = 0; i < numnodes; i++) subgraphmask[i]       = 1;
  for (i = 0; i < numnbrs;  i++) subgraphmask[nbrs[i]] = 0;

  for (i = 0; i < numnbrs; i++) {

    inv = 0;
    if (bfs(g, &(nbrs[i]), 1, subgraphmask, &inv, NULL, _bfs_cb, NULL))
      goto fail;

    if (inv < 0) goto fail;
    invsum += inv;
  }

  free(subgraphmask);

  effic = invsum / (numnbrs*(numnbrs-1));

  stats_cache_add(g,
                  STATS_CACHE_NODE_LOCAL_EFFICIENCY,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_NODE_LOCAL_EFFICIENCY, nidx, -1, &effic);

  return effic;

fail:
  if (subgraphmask != NULL) free(subgraphmask);
  return -1;
}

static uint8_t _bfs_cb(bfs_state_t *state, void *context) {
  
  double *inv;

  inv = (double *)context;

  (*inv) += (float)(state->thislevel.size)/(state->depth);

  return 0;
}

