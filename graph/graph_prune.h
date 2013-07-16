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
 * and components.
 *
 * If size is > 0, the prop parameter is ignored, and the graph is pruned such
 * that only those components which are larger than size remain.
 *
 * If size is 0 and prop is 0.0, the graph is pruned such that only the
 * largest component remains - if multiple components have equal largest size,
 * they are all retained.
 * 
 * If size is 0 and prop is > 0.0, the graph is pruned such that only the
 * largest component remains, but only if that component is (prop*100)% of the
 * total network size. If this is not the case, the graph is pruned as if
 * (size=1,prop=0.0) were passed in - i.e. only disconnected nodes are pruned.
 *
 * \return 0 on success, non-0 otherwise.
 */
uint8_t graph_prune(
  graph_t *gin,  /**< input graph                             */
  graph_t *gout, /**< place to create output graph            */
  uint32_t size, /**< components <= this size will be removed */
  double   prop  /**< minimum proportion of largest component */
);

#include "graph/graph.h"
