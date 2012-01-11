/**
 * Function which calculates the betweenness (or 'rush') of every edge in a
 * graph. Make sure you enable the stats_cache, otherwise each call to
 * stats_edge_betweenness will result in complete recalculation for the entire
 * grapoh.
 *
 *   M. J. Anthonisse 1971. The Rush In A Directed Graph (Technical
 *   Report). Stichting Mathematicsh Centrum, Amsterdam.
 *
 *   MEJ Newman & M Girvan 2004. Finding and evaluating community
 *   structure in networks. Physical Review E (69) 026113.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/array.h"
#include "util/edge_array.h"
#include "graph/graph.h"
#include "graph/bfs.h"
#include "graph/expand.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Calculates the betweenness of every edge in the given component. The
 * individual values for every edge are stored in the given ttlbetw array,
 * and also added to the stats_cache.
 *
 * \0 on success, non-0 on failure.
 */
static uint8_t _all_edge_betweenness(
  graph_t      *g,       /**< the graph                               */
  edge_array_t *betw,    /**< place to tally betweenness values       */
  edge_array_t *ttlbetw, /**< place to store final betweenness values */
  uint32_t      cmp      /**< component to calculate values for       */
);

  /**
 * Calculates edge-betweenness values for every edge in the graph, using the
 * given node as the source node. The betweenness values for this node are
 * stored in the betw pointer, and added to the values which are already in
 * the ttlbetw pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _node_betweenness(
  graph_t      *g,        /**< the graph                              */
  uint32_t      v,        /**< source node                            */
  edge_array_t *betw,     /**< betweenness values for this node       */
  edge_array_t *ttlbetw,  /**< betweenness tallies                    */
  double       *numpaths, /**< memory to use for storing path counts  */
  double       *pathlens  /**< memory to use for storing path lengths */
);

uint8_t stats_edge_betweenness(graph_t *g, uint32_t v, double *betweenness) {

  uint32_t     nnbrs;
  edge_array_t betw;
  edge_array_t ttlbetw;
  uint32_t     cmp;

  betw.vals    = NULL;
  ttlbetw.vals = NULL;

  nnbrs = graph_num_neighbours(      g, v);
  stats_cache_node_component(g, v, &cmp);

  if (edge_array_create(g, sizeof(double), &betw))    goto fail;
  if (edge_array_create(g, sizeof(double), &ttlbetw)) goto fail;

  if (_all_edge_betweenness(g, &betw, &ttlbetw, cmp)) goto fail;

  if (betweenness != NULL) 
    memcpy(betweenness,
           edge_array_get_all(&ttlbetw, v),
           nnbrs*sizeof(double));

  edge_array_free(&betw);
  edge_array_free(&ttlbetw);
  return 0;
  
fail:
  if (betw.vals    != NULL) edge_array_free(&betw);
  if (ttlbetw.vals != NULL) edge_array_free(&ttlbetw);
  return 1;
}

uint8_t _all_edge_betweenness(
  graph_t *g, edge_array_t *betw, edge_array_t *ttlbetw, uint32_t cmp) { 

  uint64_t     i;
  uint32_t     nedges;
  uint32_t     nnodes;
  double      *numpaths;
  double      *pathlens;
  uint32_t    *components;

  numpaths   = NULL;
  pathlens   = NULL;
  components = NULL;
  nnodes   = graph_num_nodes(g);
  nedges   = graph_num_edges(g);

  numpaths = calloc(nnodes, sizeof(double));
  if (numpaths == NULL) goto fail;
  
  pathlens = calloc(nnodes, sizeof(double));
  if (pathlens == NULL) goto fail;
  
  components = calloc(nnodes, sizeof(uint32_t));
  if (components == NULL) goto fail;

  stats_cache_node_component(g, -1, components);
  
  for (i = 0; i < nnodes; i++) {
    if (components[i] != cmp) continue;
    if (_node_betweenness(g, i, betw, ttlbetw, numpaths, pathlens))
      goto fail;
  }

  stats_cache_add(g,
                  STATS_CACHE_EDGE_BETWEENNESS,
                  STATS_CACHE_TYPE_EDGE,
                  sizeof(double));
  for (i = 0; i < nnodes; i++) {
    if (components[i] != cmp) continue;
    stats_cache_update(g, STATS_CACHE_EDGE_BETWEENNESS, i, -1,
                       edge_array_get_all(ttlbetw, i));
  }

  free(numpaths);
  free(pathlens);
  free(components);
  return 0;
  
fail:
  if (numpaths   != NULL) free(numpaths);
  if (pathlens   != NULL) free(pathlens);
  if (components != NULL) free(components);
  return 1;
}

uint8_t _node_betweenness(
  graph_t      *g,
  uint32_t      v,
  edge_array_t *betw,
  edge_array_t *ttlbetw,
  double       *numpaths,
  double       *pathlens) {

  uint64_t   i;
  uint64_t   j;
  uint32_t   ni;
  uint32_t   nj;
  uint32_t   nnodes;
  uint32_t   nnbrs;
  uint32_t  *nbrs;
  double     tmp;
  double     tmp2;
  double     tally;
  cstack_t   levels;
  array_t   *level;

  levels.data = NULL;
  nnodes      = graph_num_nodes(g);

  stats_cache_pair_numpaths(   g, v, numpaths);
  stats_cache_pair_pathlength( g, v, pathlens);

  if (graph_level_stack(g, v, &levels)) goto fail;

  level = stack_pop(&levels);
  while (level != NULL) {

    for (i = 0; i < level->size; i++) {
      
      array_get(level, i, &ni);

      nnbrs = graph_num_neighbours(g, ni);
      nbrs  = graph_get_neighbours(g, ni);

      /*
       * Sum up the betweenness values of the edges
       * between this level and the previous level
       */
      tally = 0;
      for (j = 0; j < nnbrs; j++) {

        nj = nbrs[j];
        
        /*
         * nodes which are further away than
         * node ni are in the previous level
         */
        if (pathlens[nj] <= pathlens[ni]) continue;
        
        tally += *(double *)edge_array_get_by_idx(betw, ni, j);
      }

      /*
       * Set the betweenness values for the edges
       * between this level and the next level
       */
      for (j = 0; j < nnbrs; j++) {
        
        nj = nbrs[j];

        /*ignore nodes from the previous level*/
        if (pathlens[nj] > pathlens[ni]) continue;

        /*edges to nodes in the current level*/
        else if (pathlens[nj] == pathlens[ni]) tmp = 0.0;

        /*edges to nodes in the next level*/
        else tmp = (1 + tally) * (numpaths[nj]/numpaths[ni]);

        edge_array_set(betw, ni, nj, &tmp);
        tmp2 = *(double *)edge_array_get(ttlbetw, ni, nj);
        tmp2 += tmp/2.0;
        edge_array_set(ttlbetw, ni, nj, &tmp2);
      }
    }

    array_free(level);
    level = stack_pop(&levels);
  }

  stack_free(&levels);
  return 0;

fail:
  return 1;
}
