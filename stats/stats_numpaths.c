/**
 * Function which counts the number of shortest paths that exist between
 * nodes.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/bfs.h"
#include "graph/expand.h"
#include "util/array.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Context struct passed to both the bfs and the expand callback functions.
 * The visited mask is required so that the expand callback can differentiate
 * between nodes in the current level, and nodes in previous levels; it is
 * updated in the bfs callback, at each level.
 */
typedef struct _bfs_ctx {
  uint32_t  root;     /**< root node                     */
  double    total;    /**< total number of paths         */
  double   *numpaths; /**< number of paths to each node  */
  uint8_t  *visited;  /**< nodes which have been visited */
} bfs_ctx_t;

/**
 * Breadth-first search callback. Updates the visited mask, and total path
 * count.
 *
 * \return 0.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< bfs state                   */
  void        *context /**< pointer to bfs_ctx_t struct */
);

/**
 * Expand callback. Updates the per-node path counts.
 *
 * \return 0.
 */
static uint8_t _exp_cb(
  expand_state_t *state,  /**< expand state                */
  void           *context /**< pointer to bfs_ctx_t struct */
);

double stats_numpaths(graph_t *g, uint32_t nidx, double *numpaths) {

  bfs_ctx_t ctx;
  uint32_t  nnodes;

  ctx.root     = nidx;
  ctx.total    = 0;
  ctx.numpaths = NULL;
  ctx.visited  = NULL;

  nnodes = graph_num_nodes(g);

  ctx.numpaths = calloc(nnodes, sizeof(double));
  if (ctx.numpaths == NULL) goto fail;

  ctx.visited = calloc(nnodes, sizeof(uint8_t));
  if (ctx.visited == NULL) goto fail;

  ctx.numpaths[nidx] = 1;

  if (bfs(g, &nidx, 1, NULL, &ctx, &ctx, _bfs_cb, _exp_cb)) goto fail;

  if (numpaths != NULL)
    memcpy(numpaths, ctx.numpaths, nnodes*sizeof(double));
  
  stats_cache_add(g,
                  STATS_CACHE_NODE_NUMPATHS,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(double));
  stats_cache_add(g,
                  STATS_CACHE_PAIR_NUMPATHS,
                  STATS_CACHE_TYPE_PAIR,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_NODE_NUMPATHS, nidx, -1, &(ctx.total));
  stats_cache_update(g, STATS_CACHE_PAIR_NUMPATHS, nidx, -1, ctx.numpaths);

  free(ctx.numpaths);
  free(ctx.visited);
  return ctx.total;
  
fail:
  if (ctx.numpaths != NULL) free(ctx.numpaths);
  if (ctx.visited  != NULL) free(ctx.visited);
  return 0;
}

uint8_t _bfs_cb(bfs_state_t *state, void *context) {

  uint64_t i;
  uint32_t ni;
  bfs_ctx_t *ctx;

  ctx = (bfs_ctx_t *)context;

  for (i = 0; i < state->thislevel.size; i++) {

    array_get(&(state->thislevel), i, &ni);

    ctx->visited[ni] = 1;
    ctx->total += ctx->numpaths[ni];
  }

  return 0;
}

uint8_t _exp_cb(expand_state_t *state, void *context) {

  bfs_ctx_t *ctx;

  ctx = (bfs_ctx_t *)context;

  if (state->child == ctx->root) {
    return 0;
  }

  /*
   * if the child is from a previously
   * visited level, do nothing
   */
  if (ctx->visited[state->child]) {
    return 0;
  }
  
  /*
   * if the parent is the root, it
   * means we are on the first level
   */
  if (state->parent == ctx->root) {
    ctx->numpaths[state->child] = 1;
  }

  /*
   * otherwise, the number of paths to
   * the child is equal to the sum of
   * the number of paths to all of its
   * parents
   */
  else {
    ctx->numpaths[state->child] += ctx->numpaths[state->parent];
  }

  return 0;
}
