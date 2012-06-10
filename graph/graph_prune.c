/**
 * Removes minor components from a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>

#include "util/array.h"
#include "graph/graph.h"
#include "graph/bfs.h"
#include "graph/graph_prune.h"

/**
 * Breadth first search context. 
 */
typedef struct _ctx {

  uint32_t *components;   /**< per-node component IDs    */
  uint32_t  size;         /**< size of current component */
  uint32_t  component_id; /**< current component ID      */

} ctx_t;

/**
 * Finds all components in the given graph. Saves the component ID (1-indexed)
 * in the given components array (which must be graph_num_nodes(g) in length),
 * and the component sizes in the given sizes array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_components(
  graph_t  *g,          /**< a graph                                    */
  uint32_t *components, /**< array to store component IDs for each node */
  array_t  *sizes       /**< array to store component sizes             */
);

/**
 * Prunes the input graph according to the given threshold.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _prune(
  graph_t  *gin,         /**< graph to be pruned                      */
  graph_t  *gout,        /**< place to put output graph               */
  uint32_t  threshold,   /**< components <= this size are removed     */
  uint32_t *components,  /**< per-node component IDs                  */
  uint32_t  ncomponents, /**< number of components                    */
  uint32_t *sizes        /**< component sizes (length == ncomponents) */
);

/**
 * Breadth first search callback. Updates component IDs and component size.
 *
 * \return 0 always.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< search state              */
  void        *context /**< pointer to a ctx_t struct */
);

uint8_t graph_prune(graph_t *gin, graph_t *gout, uint32_t size) {

  uint32_t *components;
  array_t   sizes;
  uint32_t  nnodes;
  uint64_t  i;
  uint32_t  s;

  components = NULL;
  sizes.data = NULL;

  nnodes = graph_num_nodes(gin);
  
  components = calloc(nnodes,sizeof(uint32_t));
  if (components == NULL) goto fail;

  if (array_create(&sizes, sizeof(uint32_t), 10))
    goto fail;

  if (_find_components(gin, components, &sizes))
    goto fail;

  /*
   * if size was not specified, prune all but the largest component.
   * 
   */
  if (size == 0) {

    for (i = 0; i < sizes.size; i++) {
      
      array_get(&sizes, i, &s);
      if (s > size) size = s;
    }

    size -= 1;
  }

  if (_prune(gin, gout, size, components, sizes.size, (uint32_t *)sizes.data))
    goto fail;

  return 0;
fail:
  if (components != NULL) free(components);
  if (sizes.data != NULL) array_free(&sizes);
  return 1;
}

uint8_t _find_components(graph_t *g, uint32_t *components, array_t *sizes) {

  uint64_t i;
  uint32_t n;
  uint32_t nnodes;
  ctx_t    ctx;

  ctx.components = components;

  nnodes = graph_num_nodes(g);

  /*
   * traverse the whole graph by performing a series
   * of breadth first searches; record the size of each
   * component as it is found, and the component ID for
   * each node. Component IDs are 1-indexed.
   */
  ctx.component_id = 1;

  for (i = 0; i < nnodes; i++) {

    n = i;

    /*node has already been assigned to a component*/
    if (ctx.components[i]) continue;

    ctx.size = 1;
    ctx.components[i] = ctx.component_id;

    if (bfs(g, &n, 1, NULL, &ctx, NULL, _bfs_cb, NULL)) goto fail;

    ctx.component_id ++;
    if (array_append(sizes, &(ctx.size))) goto fail;
  }

  return 0;
  
fail:
  return 1;
}

uint8_t _bfs_cb(bfs_state_t *state, void *context) {

  uint64_t i;
  uint32_t ni;
  ctx_t *ctx;


  ctx = (ctx_t *)context;

  for (i = 0; i < state->thislevel.size; i++) {

    array_get(&(state->thislevel), i, &ni);
    ctx->components[ni] = ctx->component_id;
  }

  ctx->size += state->thislevel.size;

  return 0;
}

uint8_t _prune(
  graph_t  *gin,
  graph_t  *gout,
  uint32_t  threshold,
  uint32_t *components,
  uint32_t  ncomponents,
  uint32_t *sizes) {

  uint64_t       i;
  uint64_t       j;
  uint32_t      *nidmap;
  uint32_t       ninnodes;
  uint32_t       noutnodes;
  uint32_t      *nbrs;
  float         *wts;
  uint32_t       nnbrs;
  graph_label_t *lbl;

  nidmap = NULL;

  ninnodes = graph_num_nodes(gin);

  /*calculate the size of the output graph*/
  noutnodes = 0;
  for (i = 0; i < ncomponents; i++) {
    if (sizes[i] > threshold) noutnodes += sizes[i];
  }

  if (graph_create(gout, noutnodes, 0)) goto fail;

  /*
   * create the nid map - a mapping from
   * gin node indices to gout node indices
   */
  nidmap = malloc(ninnodes*sizeof(uint32_t));
  if (nidmap == NULL) goto fail;

  j = 0;
  for (i = 0; i < ninnodes; i++) {

    if (sizes[components[i]-1] <= threshold) nidmap[i] = 0xFFFFFFFF;
    else                                     nidmap[i] = j++;
  }

  /*copy edges from gin to gout*/
  for (i = 0; i < ninnodes; i++) {

    /*ignore nodes from thresholded components*/
    if (nidmap[i] == 0xFFFFFFFF) continue;

    nnbrs = graph_num_neighbours(gin, i);
    nbrs  = graph_get_neighbours(gin, i);
    wts   = graph_get_weights(   gin, i);

    for (j = 0; j < nnbrs; j++) {

      /*ignore edges to thresholded components*/
      if (nidmap[nbrs[j]] == 0xFFFFFFFF) continue;
      
      if (graph_add_edge(gout, nidmap[i], nidmap[nbrs[j]], wts[j]))
        goto fail;
    }
  }

  /*copy nodelabels from gin to gout*/
  for (i = 0; i < ninnodes; i++) {

    if (nidmap[i] == 0xFFFFFFFF) continue;

    /*assume that there are no node labels to copy, if the get call fails*/

    lbl = graph_get_nodelabel(gin, i);
    if (lbl == NULL) break;
    if (graph_set_nodelabel(gout, nidmap[i], lbl)) goto fail;
  }
  
  free(nidmap);
  return 0;
  
fail:
  if (nidmap != NULL) free(nidmap);
  return 1;
}
