/**
 * Breadth first search through a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __BFS_H__
#define __BFS_H__

#include <stdint.h>

#include "graph/graph.h"
#include "graph/expand.h"
#include "util/array.h"

/**
 * Struct used to maintain state during a breadth first search. This struct is
 * passed to the callback function at each depth.
 */
typedef struct _bfs_state {

  uint32_t depth;      /**< current depth from the root node               */
  array_t  thislevel;  /**< nodes at this level                            */
  uint8_t *visited;    /**< whether or not nodes have been visited
                            (includes nodes that were found at this level) */

} bfs_state_t;

/**
 * Performs a breadth first search through the graph, starting from the given
 * root nodes. Every time the search expands out to the next depth, the given
 * callback function is called with the current search state, and an optional
 * context parameter. If you don't need a callback or a context, pass in NULL.
 *
 * A second callback and context, for the expand function, may be passed
 * in. These parameters are passed directly to expand - see expand.h for
 * details. Pass in NULL for both if you don't need an expand function.
 *
 * If, at any time during the search, the callback function returns non-zero,
 * the search is terminated immediately.
 *
 * The optional subgraph mask can be used to limit the search to a specific
 * subgraph, by setting the mask to non-zero for all nodes which are to be
 * excluded from the search. If you want to search the entire graph, pass in
 * NULL.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t bfs(
  graph_t    *g,               /**< the graph to search                    */
  uint32_t   *roots,           /**< nodes to start the search from         */
  uint32_t    nroots,          /**< number of root nodes                   */
  uint8_t    *subgraphmask,    /**< subgraph to search                     */
  void       *lvl_context,     /**< context to pass to callback function   */
  void       *edge_context,    /**< expand callback context                */
  uint8_t   (*lvl_callback) (  /**< optional callback called at each depth */
    bfs_state_t *state,        /**< current search state                   */
    void        *context),     /**< callback context                       */
  uint8_t   (*edge_callback) ( /**< optional callback called at each edge  */
    expand_state_t *state,     /**< current expand state                   */
    void           *context)   /**< expand callback context                */
);

#endif /* __BFS_H__ */
