/**
 * Functions for masking (removing) nodes of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/array.h"
#include "graph/graph.h"
#include "graph/graph_mask.h"

/**
 * Struct which provides an index mapping for a node, 
 * between the input graph and the output graph
 */
typedef struct _nodemap {

  uint32_t gin_idx;  /**< node index in the input graph  */
  uint32_t gout_idx; /**< node index in the output graph */
} nodemap_t;

/**
 * Creates an array of mappings from input node IDs to output node IDs.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _create_node_mapping(
  array_t *nodemap, /**< array of node mappings         */
  uint8_t *mask,    /**< mask values for input nodes    */
  uint32_t nnodes   /**< number of nodes in input graph */
);
  
/**
 * Copies node labels from the input graph to the output graph, for the
 * given list of nodes.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _copy_nodelabels(
  graph_t *gin,   /**< input graph             */
  graph_t *gout,  /**< output graph            */
  array_t *nodes  /**< array of node_t structs */
);

/**
 * Copies edges from the input graph to the output graph, for the given
 * list of nodes.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _copy_edges(
  graph_t *gin,  /**< input graph         */
  graph_t *gout, /**< output graph        */
  array_t *nodes /**< node index mappings */
);

uint8_t graph_remove(
  graph_t *gin, graph_t *gout, uint32_t *nodes, uint32_t nnodes) {

  uint64_t i;
  uint32_t nginnodes;
  uint8_t *mask;
  uint8_t  result;

  mask = NULL;

  nginnodes = graph_num_nodes(gin);

  mask = calloc(nginnodes, sizeof(uint8_t));
  if (mask == NULL) goto fail;
  
  for (i = 0; i < nnodes; i++) mask[nodes[i]] = 1;

  result = graph_mask(gin, gout, mask);

  free(mask);
  return result;

fail:
  if (mask != NULL) free(mask);
  return 1;
}

uint8_t graph_mask(graph_t *gin, graph_t *gout, uint8_t *mask) {

  array_t nodemap;
  memset(&nodemap, 0, sizeof(nodemap));

  if (array_create(&nodemap, sizeof(nodemap_t), 100))             goto fail;
  if (_create_node_mapping(&nodemap, mask, graph_num_nodes(gin))) goto fail;
  if (graph_create(gout, nodemap.size, 0))                        goto fail;
  if (_copy_nodelabels(gin, gout, &nodemap))                      goto fail;
  if (_copy_edges(gin, gout, &nodemap))                           goto fail;

  array_free(&nodemap);
  return 0;

fail:

  if (nodemap.data != NULL) array_free(&nodemap);
  return 1;
}

uint8_t _create_node_mapping(
  array_t *nodemap, uint8_t *mask, uint32_t nnodes) {

  uint64_t  i;
  uint32_t  gout_idx;
  nodemap_t map;

  gout_idx = 0;

  for (i = 0; i < nnodes; i++) {

    if (mask[i]) {
      
      map.gin_idx  = i;
      map.gout_idx = gout_idx;
      gout_idx ++;

      if (array_append(nodemap, &map)) goto fail;
    }
  }

  return 0;

fail:
  return 1;
}

uint8_t _copy_edges(
  graph_t *gin, graph_t *gout, array_t *nodes) {

  uint64_t   i;
  uint64_t   j;
  nodemap_t *nodei;
  nodemap_t *nodej;

  for (i = 0; i < nodes->size; i++) {
    for (j = i+1; j < nodes->size; j++) {

      nodei = array_getd(nodes, i);
      nodej = array_getd(nodes, j);

      if (graph_are_neighbours(gin, nodei->gin_idx, nodej->gin_idx)) {
        if (graph_add_edge(gout, nodei->gout_idx, nodej->gout_idx, 1))
          goto fail;
      }
    }
  }

  return 0;

fail:
  return 1;
}

uint8_t _copy_nodelabels(
  graph_t *gin, graph_t *gout, array_t *nodes) {

  uint64_t       i;
  graph_label_t *label;
  nodemap_t     *node;

  for (i = 0; i < nodes->size; i++) {

    node = array_getd(nodes, i);

    label = graph_get_nodelabel(gin,  node->gin_idx);
    if (label == NULL) goto fail;
    
    if (graph_set_nodelabel(gout, node->gout_idx, label)) goto fail;
  }

  return 0;

fail:
  return 1;
}
