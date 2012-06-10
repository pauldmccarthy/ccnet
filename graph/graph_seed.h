/**
 * Extract a subgraph from one or more seed nodes, by breadth-first searching
 * out from the seed a specified depth.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#ifndef GRAPH_SEED_H
#define GRAPH_SEED_H

#include <stdint.h>
#include "graph/graph.h"

/**
 * Creates a new graph from the input graph, by executing a breadth first
 * search from the specified seed node(s) to the specified depth; all nodes
 * (and edges) in the input graph which are reached in the search are included
 * in the output graph.
 * 
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_seed(
  graph_t  *gin,    /**< input graph                   */
  graph_t  *gout,   /**< uninitialised output graph    */
  uint32_t *seeds,  /**< array of seed node IDs        */
  uint32_t  nseeds, /**< number of seed nodes          */
  uint8_t   depth   /**< depth of breadth first search */
);

#endif
