/**
 * A couple of convenience functions for checking, and ensuring, the
 * connectivity of groups of nodes within a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "graph/bfs.h"
#include "graph/graph.h"

/**
 * Breadth first search context.
 */
typedef struct _ctx {

  uint8_t *visited; /**< mask, one value for every node in
                         the graph; a non-0 value means the
                         node has been visited in the search */
} ctx_t;

/**
 * Breadth first search callback function, called every time the depth is
 * increased.
 *
 * \return 0.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< current state of search   */
  void        *context /**< pointer to a ctx_t struct */
);


uint8_t graph_are_connected(graph_t *g, uint32_t *group, uint32_t ngroup) {

  uint8_t *mask;
  uint32_t nnodes;
  uint64_t i;
  ctx_t    ctx;

  mask        = NULL;
  ctx.visited = NULL;
  nnodes      = graph_num_nodes(g);

  mask = calloc(nnodes, sizeof(uint8_t));
  if (mask == NULL) goto fail;

  ctx.visited = calloc(nnodes, sizeof(uint8_t));
  if (ctx.visited == NULL) goto fail;

  /* limit the search to the subgraph formed by the nodes in the group */
  memset(mask, 1, nnodes);
  for (i = 0; i < ngroup; i++)
    mask[group[i]] = 0;

  /* bfs through the subgraph, starting at the first node in the group */
  ctx.visited[group[0]] = 1;
  if (bfs(g, group, 1, mask, &ctx, NULL, &_bfs_cb, NULL))
    goto fail;

  /*
   * if any nodes in the group weren't visited,
   * the subgraph is not connected
   */
  for (i = 0; i < ngroup; i++)
    if (!ctx.visited[group[i]])
      break;

  free(mask);
  free(ctx.visited);
  
  return (i == ngroup) ? 1 : 0;

fail:
  if (mask        != NULL) free(mask);
  if (ctx.visited != NULL) free(ctx.visited);
  return 1;
}

static uint8_t _bfs_cb(bfs_state_t *state, void *context) {

  uint64_t i;
  uint32_t node;
  ctx_t   *ctx;

  ctx = (ctx_t *)context;

  /* set the visited mask for all nodes that have been visited */
  for (i = 0; i < state->thislevel.size; i++) {

    array_get(&state->thislevel, i, &node);
    ctx->visited[node] = 1;
  }

  return 0;
}

uint8_t graph_connect(graph_t *g, uint32_t *group, uint32_t ngroup) {

  uint64_t i;
  uint64_t j;

  /* add edges at random until the group is connected */
  printf("              ");
  while (!graph_are_connected(g, group, ngroup)) {

    i = (uint32_t)(rand() * ((double)ngroup / RAND_MAX));
    j = i;

    while (j == i) 
      j = (uint32_t)(rand() * ((double)ngroup / RAND_MAX));

    if (graph_add_edge(g, group[i], group[j], 1))
      goto fail;
  }
  
  return 0;
  
fail:
  return 1;
}

uint8_t graph_connect_from(
  graph_t *g, graph_t *src, uint32_t *group, uint32_t ngroup) {


  uint64_t i;
  uint64_t j;

  for (i = 0; i < ngroup; i++) {
    for (j = i+1; j < ngroup; j++) {

      if (graph_are_neighbours(src, group[i], group[j])) {
        if (graph_add_edge(g, group[i], group[j], 1.0))
          goto fail;
      }
    }
  }


  return 0;

fail:

  return 1;
}
