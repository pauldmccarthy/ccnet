/**
 * Functions for masking (removing) nodes of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef GRAPH_MASK_H
#define GRAPH_MASK_H

#include <stdint.h>

#include "graph/graph.h"

/**
 * Removes the nodes specified in the nodes array, from the input graph,
 * copying the result to the output graph.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_remove(
  graph_t  *gin,   /**< input graph                         */
  graph_t  *gout,  /**< uninitialised graph to store output */
  uint32_t *nodes, /**< list of IDs of nodes to remove      */
  uint32_t  nnodes /**< number of nodes to remove           */
);

/**
 * Remove nodes from the input graph as specified by the nodemask array. The
 * nodemask array must contain one value for every node in the input graph;
 * for a given node, a value of 1 in the nodemask array means that the node
 * should be copied to the output graph, whereas a value of 0 means that
 * the node should be removed.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_mask(
  graph_t *gin,     /**< graph to mask                       */
  graph_t *gout,    /**< uninitialised graph to store output */
  uint8_t *nodemask /**< mask specifying nodes to remove     */
);

#endif
