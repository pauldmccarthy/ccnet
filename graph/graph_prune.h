/**
 * Provides a function, graph_prune, which may be used to remove components,
 * less than a specified size, from a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __GRAPH_PRUNE_H__
#define __GRAPH_PRUNE_H__

#endif /* __GRAPH_PRUNE_H__ */

#include <stdint.h>

#include "graph/graph.h"

/**
 * Creates a new graph by pruning the input graph of any disconnected nodes,
 * and components which are smaller than or equal to the given size. If
 * size is 0, the graph is pruned such that only the largest component
 * remains.
 *
 * \return 0 on success, non-0 otherwise.
 */
uint8_t graph_prune(
  graph_t *gin,  /**< input graph                             */
  graph_t *gout, /**< place to create output graph            */
  uint32_t size  /**< components <= this size will be removed */
);

#include "graph/graph.h"
