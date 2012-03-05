/**
 * Function which counts the number of components in a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/bfs.h"
#include "util/array.h"
#include "util/compare.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Context information passed between calls to _bfs_cb.
 */
typedef struct _ctx {

  uint8_t  *visited;
  uint32_t *component;
  uint32_t  cmpnum;
  uint32_t  size;

} ctx_t;

/**
 * Callback function for the breadth first search. Updates the nodes which
 * have been visited.
 *
 * \return 0 always.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< current search state       */
  void        *context /**< pointer to a visited array */
);

uint32_t stats_num_components(
  graph_t *g, uint32_t sz, array_t *sizes, uint32_t *cmpnums) {

  uint64_t i;
  uint32_t root;
  uint32_t numnodes;
  uint32_t cmpnum;
  ctx_t    ctx;

  ctx.visited   = NULL;
  ctx.component = NULL;
  ctx.cmpnum    = 0;
  ctx.size      = 0;
  numnodes      = graph_num_nodes(g);

  if (sizes != NULL) array_clear(sizes);

  ctx.visited = calloc(numnodes, sizeof(uint8_t));
  if (ctx.visited == NULL) goto fail;

  ctx.component = calloc(numnodes, sizeof(uint32_t));
  if (ctx.component == NULL) goto fail;

  for (i = 0; i < numnodes; i++) {

    if (ctx.visited[i]) continue;

    ctx.size = 1;
    ctx.component[i] = ctx.cmpnum;
    root = i;
    ctx.visited[i] = 1;
    if (bfs(g, &root, 1, NULL, &ctx, NULL, _bfs_cb, NULL)) goto fail;

    if (ctx.size < sz) continue;
    
    ctx.cmpnum++;
    if (sizes != NULL) array_append(sizes, &(ctx.size));
  }

  if (cmpnums != NULL)
    memcpy(cmpnums, ctx.component, numnodes*sizeof(uint32_t));

  cmpnum = ctx.cmpnum;
  
  stats_cache_add(g,
                  STATS_CACHE_NUM_COMPONENTS,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(uint32_t));
  stats_cache_add(g,
                  STATS_CACHE_NODE_COMPONENT,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(uint32_t));
  stats_cache_update(g, STATS_CACHE_NUM_COMPONENTS, 0, -1, &cmpnum);

  for (i = 0; i < numnodes; i++){
    stats_cache_update(g, STATS_CACHE_NODE_COMPONENT, i, -1, ctx.component+i);
  }

  free(ctx.visited);
  free(ctx.component);
  
  return ctx.cmpnum;

fail:

  if (ctx.visited   != NULL) free(ctx.visited);
  if (ctx.component != NULL) free(ctx.component);
  return 0;
}

double stats_component_span(graph_t *g, uint32_t cmp) {

  uint64_t i;
  uint64_t j;
  uint32_t ni;
  uint32_t nj;
  uint32_t cmpsz;
  double   dist;
  double   maxdist;
  array_t  nodes;

  nodes.data = NULL;
  maxdist    = -1.0;

  if (array_create(&nodes, sizeof(uint32_t), 100)) goto fail;

  if (graph_get_component(g, cmp, &nodes)) goto fail;

  cmpsz = nodes.size;

  for (i = 0; i < cmpsz; i++) {
    for (j = i+1; j < cmpsz; j++) {

      array_get(&nodes, i, &ni);
      array_get(&nodes, j, &nj);

      dist = stats_edge_distance(g, ni, nj);

      if (dist > maxdist) maxdist = dist;
    }
  }

  array_free(&nodes);
  return maxdist;

fail:
  if (nodes.data != NULL) array_free(&nodes);
  return -1.0;
}

static uint8_t _bfs_cb(bfs_state_t *state, void *context) {
  
  uint64_t i;
  uint32_t ni;
  ctx_t   *ctx;

  ctx = (ctx_t *)context;

  for (i = 0; i < state->thislevel.size; i++) {
    
    array_get(&(state->thislevel), i, &ni);
    ctx->visited[ni]   = 1;
    ctx->component[ni] = ctx->cmpnum;
  }
  
  ctx->size += state->thislevel.size;

  return 0;
}

uint32_t stats_largest_component(graph_t *g) {

  uint32_t lcmp;
  array_t  sizes;
  
  memset(&sizes, 0, sizeof(sizes));

  if (array_create(&sizes, sizeof(uint32_t), 10)) goto fail;
  array_set_cmps(&sizes, &compare_u32, &compare_u32_insert);

  stats_num_components(g, 0, &sizes, NULL);

  array_sort(&sizes);

  if (array_get(&sizes, sizes.size - 1, &lcmp)) goto fail;

  stats_cache_add(g,
                  STATS_CACHE_LARGEST_COMPONENT,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(uint32_t));
  stats_cache_update(g, STATS_CACHE_LARGEST_COMPONENT, 0, -1, &lcmp);

  array_free(&sizes);
  
  return lcmp;
  
fail:
  if (sizes.data != NULL) array_free(&sizes);
  return 0;
}
