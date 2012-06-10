/**
 *
 * Extract a subgraph from a seed node, by breadth-first searching out from
 * the seed a specified depth.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph_seed.h"
#include "graph/graph_mask.h"
#include "graph/bfs.h"
#include "util/array.h"

typedef struct __ctx {

  uint8_t  maxdepth;
  uint8_t *nodemask;

} ctx_t;

/**
 * Breadth first search callback function, called each time the search depth
 * is increased.
 *
 * \return 1 when the maximum depth has been reached.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< current search state */
  void        *context /**< pointer to a ctx_t   */
);

uint8_t graph_seed(graph_t *gin, graph_t *gout, uint32_t n, uint8_t depth) {

  ctx_t    ctx;
  uint32_t nnodes;

  nnodes       = graph_num_nodes(gin);
  ctx.maxdepth = depth;
  ctx.nodemask = calloc(nnodes, 1);

  if (ctx.nodemask == NULL) goto fail;

  if (bfs(gin, &n, 1, NULL, &ctx, NULL, _bfs_cb, NULL)) goto fail;

  if (graph_mask(gin, gout, ctx.nodemask)) goto fail;

  free(ctx.nodemask);
  
  return 0;
  
fail:
  if (ctx.nodemask != NULL) free(ctx.nodemask);
  return 1;
}


uint8_t _bfs_cb(bfs_state_t *state, void *context) {

  ctx_t   *ctx;
  uint64_t i;
  uint32_t n;

  ctx = (ctx_t *)context;

  for (i = 0; i < state->thislevel.size; i++) {

    array_get(&(state->thislevel), i, &n);
    ctx->nodemask[n] = 1;
  }

  if (state->depth >= ctx->maxdepth) return 1;
  else                               return 0;
}
