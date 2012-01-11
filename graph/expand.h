/**
 * Prototype for the expand function, which is used for performing a breadth
 * first search through a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __EXPAND_H__
#define __EXPAND_H__

#include <stdint.h>

#include "graph/graph.h"
#include "util/array.h"

/**
 * Expand state struct, passed to the expand callback function on every edge
 * traversal. Passes the parent node (the node in the previous level), the
 * child node (the node in the next level), and whether the child node has
 * already been visited.
 */
typedef struct _expand_state {

  uint32_t parent;  /**< Parent node index                      */
  uint32_t child;   /**< Child node index                       */
  uint8_t  visited; /**< Whether child has already been visited */

} expand_state_t;

/**
 * Finds the nodes in the graph which are neighbours of the nodes listed in
 * thislevel, and have not yet been visited, as specified by the visited
 * mask. The indices of the nodes which are found will be stored in nextlevel.
 *
 * The visited array must contain one value for every node in the graph. For a
 * specific node, a value of one indicates that the node has already been
 * visited, and should be excluded from the search. All nodes which are
 * discovered are marked as visited in the visited array.
 *
 * The visited mask can also be used to limit a search to a subgraph, simply
 * by masking out all nodes which are not in the subgraph of interest.
 *
 * If a callback function is provided, it is called on every edge traversal,
 * and is passed details of that edge, and the optional context pointer. If
 * you don't need a callback on every edge, pass in NULL for both the callback
 * and the context. If the callback function returns anything but 0, the
 * search is terminated.
 *
 * \return 0 normally, non-0 if the callback function requested that the
 * search be terminated.
 */
uint8_t expand(
  graph_t  *g,              /**< the graph to query                     */
  array_t  *thislevel,      /**< list of nodes in this level            */
  array_t  *nextlevel,      /**< place to store newly found nodes       */
  uint8_t  *visited,        /**< visited mask, one entry for each node  */
  void     *context,        /**< Optional context pointer for callback  */
  uint8_t  (*callback) (    /**< Optional callback function             */
    expand_state_t *state,  /**< Current expand state                   */
    void           *context /**< Context                                */
  )
);

#endif /* __EXPAND_H__ */
