/**
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "util/array.h"
#include "graph/bfs.h"
#include "graph/expand.h"
#include "graph/graph.h"

/**
 * Context information used to keep state during the breadth first search. The
 * same struct is shared by both the bfs and the expand callback functions.
 */
typedef struct _ctx {

  uint32_t src;        /**< source node (u)                */
  uint32_t dest;       /**< destination node (v)           */
  uint32_t pathlength; /**< the end result is stored here
                            (0 if no path was found)       */
  array_t  nodes;      /**< all visited nodes              */
  array_t  parents;    /**< parents of all visited nodes   */
  array_t  offsets;    /**< offsets of each depth, into
                            nodes and parents array        */
  array_t *path;       /**< if not NULL, indices of nodes
                            on the end path are saved here */
  
} ctx_t;

/**
 * Breadth first search callback function, called on each depth change in the
 * search. Saves the offset (into nodes and parents) of the depth that was
 * just searched, into the offsets array.
 *
 * \return 0.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< bfs state                 */
  void        *context /**< pointer to a ctx_t struct */
);

/**
 * Expand callback function, called on each edge traversal in the search.
 * Saves the child-parent relationship to the nodes/parents arrays. If/when
 * the destination node is found, terminates the search and constructs the
 * final path (if the path parameter is not NULL).
 *
 * \return 1 if the dest node has been found, 0 otherwise.
 */
static uint8_t _expand_cb(
  expand_state_t *state,  /**< expand state             */
  void           *context /**< pointe to a ctx_t struct */
);

/**
 * Constructs the path from src to dest by searching backwards through the
 * nodes/parents/offsets arrays.
 *
 * \return 0 on success, non-0 otherwise.
 */
static uint8_t _create_path(
  ctx_t *ctx /**< pointer to a ctx struct */
);

uint32_t graph_pathlength(
  graph_t *g, uint32_t u, uint32_t v, array_t *path) {

  ctx_t context;

  context.src        = u;
  context.dest       = v;
  context.pathlength = 0;
  context.path       = path;
  
  if (array_create(&(context.nodes),   sizeof(uint32_t), 50)) goto fail;
  if (array_create(&(context.parents), sizeof(uint32_t), 50)) goto fail;
  if (array_create(&(context.offsets), sizeof(uint32_t), 10)) goto fail;

  array_append(&(context.offsets), 0);

  if (bfs(g, &u, 1, NULL, &context, &context, _bfs_cb, _expand_cb))
    goto fail;
  
  array_free(&(context.nodes));
  array_free(&(context.parents));
  array_free(&(context.offsets));

  return context.pathlength;
  
fail:
  return 0;
}

uint8_t _bfs_cb(bfs_state_t *state, void *context) {

  ctx_t *ctx;

  ctx = (ctx_t *)context;

  /*
   * the _expand_cb function appended the indices of all
   * nodes found in the previous depth to the nodes
   * array - store the location in the nodes array into
   * the offsets array, so the _create_path function
   * knows where the nodes from this depth finish.
   */
  array_append(&(ctx->offsets), &(ctx->nodes.size));

  return 0;
}

uint8_t _expand_cb(expand_state_t *state, void *context) {

  ctx_t *ctx;

  ctx = (ctx_t *)context;

  if (state->visited) return 0;

  /*save the node id and its parent*/
  array_append(&(ctx->nodes),   &(state->child));
  array_append(&(ctx->parents), &(state->parent));

  /*we've found our destination*/
  if (state->child == ctx->dest) {
    
    array_append(&(ctx->offsets), &(ctx->nodes.size));
    
    ctx->pathlength = ctx->offsets.size-1;
    
    if (ctx->path != NULL) _create_path(ctx);

    /*kill the search here*/
    return 1;
  }
  return 0;
}

uint8_t _create_path(ctx_t *ctx) {

  array_t *path;
  int64_t  i;
  int64_t  j;
  uint32_t off1;
  uint32_t off2;
  uint32_t node1;
  uint32_t node2;
  uint32_t parent;

  path = ctx->path;

  /*
   * resize the path array to fit the
   * indices of every node in the path
   */
  if (array_expand(path, ctx->pathlength+1)) goto fail;

  /*we know the start and endpoints of the path*/
  array_set(path, ctx->pathlength, &(ctx->dest));
  array_set(path, 0,               &(ctx->src));

  /*
   * search backwards through the node/parent
   * list, looking for the parent of the node
   * that was last added to the path, and
   * appending the ids to the path as we go
   */
  for (i = ctx->offsets.size-1; i > 1; i--) {
    
    array_get(&(ctx->offsets), i,   &off1);
    array_get(&(ctx->offsets), i-1, &off2);

    for (j = off1; j > off2; j--) {

      array_get(&(ctx->nodes), j-1, &node1);
      array_get(path,          i,   &node2);

      if (node1 == node2) {

        array_get(&(ctx->parents), j-1, &parent);
        
        array_set(path, i-1, &parent);
        break;
      }
    }
  }

  return 0;
  
fail:
  return 1;
}
