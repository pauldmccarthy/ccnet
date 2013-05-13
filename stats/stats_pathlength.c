/**
 * Functions for calculating the path length of a node, or of a graph. 
 *
 * Definition of characteristic path length:
 *
 *   Watts DJ & Strogatz Sh 1998. Collective dynamics 
 *   of small world networks. Nature, 393:440-442.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/bfs.h"
#include "util/array.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Structure used to save state during the breadth first search.
 */
typedef struct _ctx {

  double  *pathlens;   /**< list of path lengths      */
  double   tally;      /**< current path length tally */
  uint32_t count;      /**< current path length count */
} ctx_t;

/**
 * Callback function for the breadth first search. Updates path lengths,
 * efficiency, tally and count as needed.
 *
 * \return 0 always.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< current search state    */
  void        *context /**< pointer to ctx_t struct */
);

double stats_avg_pathlength(graph_t *g) {

  uint32_t i;
  uint32_t numnodes;
  uint32_t count;
  double   avgpath;
  double   path;

  avgpath  = 0;
  count    = 0;
  numnodes = graph_num_nodes(g); 

  for (i = 0; i < numnodes; i++) {

    path = stats_pathlength(g, i, NULL);
    if (path < 0) goto fail;
    if (isnan(path)) continue;

    count++;
    avgpath += path;
  }

  avgpath /= count;

  stats_cache_add(g,
                  STATS_CACHE_GRAPH_PATHLENGTH,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_GRAPH_PATHLENGTH, 0, -1, &avgpath);

  return avgpath;

fail:
  return -1;
}

double stats_pathlength(
  graph_t  *g, 
  uint32_t  nidx, 
  double   *pathlens)
{
  ctx_t    ctx;
  double   path;
  uint32_t nnodes;

  ctx.pathlens = NULL;

  nnodes = graph_num_nodes(g);

  ctx.tally      = 0;
  ctx.count      = 0;
  ctx.pathlens   = calloc(nnodes,sizeof(double));
  if (ctx.pathlens == NULL) goto fail;

  ctx.pathlens[nidx] = 0;

  if (bfs(g, &nidx, 1, NULL, &ctx, NULL, _bfs_cb, NULL)) goto fail;

  if (ctx.count == 0) path = 0;
  else                path = ctx.tally / ctx.count;

  stats_cache_add(g,
                  STATS_CACHE_NODE_PATHLENGTH,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(double));
  stats_cache_add(g,
                  STATS_CACHE_PAIR_PATHLENGTH,
                  STATS_CACHE_TYPE_PAIR,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_NODE_PATHLENGTH, nidx, -1, &path);
  stats_cache_update(g, STATS_CACHE_PAIR_PATHLENGTH, nidx, -1, ctx.pathlens);

  if (pathlens != NULL) {
    memcpy(pathlens, ctx.pathlens, nnodes*sizeof(double));
  }

  free(ctx.pathlens);
  return path;

fail:
  if (ctx.pathlens != NULL) free(ctx.pathlens);
  return -1;
}



double stats_sub_pathlength(
  graph_t *g,
  uint32_t nidx,
  uint32_t nnodes,
  uint8_t *mask,
  double  *pathlens) 
{
  ctx_t    ctx;
  double   path;
  uint64_t i;
  uint64_t j;
  uint32_t gnnodes;

  ctx.pathlens   = NULL;
  gnnodes        = graph_num_nodes(g);
  ctx.tally      = 0;
  ctx.count      = 0;
  ctx.pathlens   = calloc(gnnodes,sizeof(double));
  if (ctx.pathlens == NULL) goto fail;

  ctx.pathlens[nidx] = 0;

  if (bfs(g, &nidx, 1, mask, &ctx, NULL, _bfs_cb, NULL)) goto fail;

  if (ctx.count == 0) path = 0;
  else                path = ctx.tally / ctx.count;

  if (pathlens != NULL) {

    for (i = 0, j = 0; i < gnnodes; i++) {

      if (mask[i]) continue;

      /*guard against array overflow - if this occurs, the input
        arguments were invalid (either nnodes or mask). */
      if (j >= nnodes) goto fail;
      
      pathlens[j++] = ctx.pathlens[i];

    }
  }

  free(ctx.pathlens);
  return path;

fail:
  if (ctx.pathlens != NULL) free(ctx.pathlens);
  return -1;
}

uint8_t _bfs_cb(bfs_state_t *state, void *context) {
  
  uint64_t i;
  uint32_t ni;
  ctx_t   *ctx;
  
  ctx = (ctx_t *)context;

  for (i = 0; i < state->thislevel.size; i++) {

    array_get(&(state->thislevel), i, &ni);
    ctx->pathlens[ni] = state->depth;
  }

  ctx->tally += (state->thislevel.size)*(state->depth);
  ctx->count += (state->thislevel.size);

  return 0;
}
