/**
 * Manage an array of values, one for each edge, for a graph. Supports
 * directed or undirected graphs.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __EDGE_ARRAY_H__
#define __EDGE_ARRAY_H__

#include <stdint.h>

#include "graph/graph.h"
#include "graph/graph_event.h"

/**
 * Struct containing edge array data.
 */
typedef struct _edge_array {

  graph_t  *g;        /**< the graph                                   */
  uint16_t  valsz;    /**< size of one value                           */
  array_t  *vals;     /**< array of array_t structs, one for each node */

  graph_event_listener_t gel; /**< graph event listener, to track
                                   edge addition/removal events        */

} edge_array_t;

/**
 * Initialise the space required for the array of values.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t edge_array_create(
  graph_t      *g,        /**< the graph                         */
  uint16_t      valsz,    /**< size of one value                 */
  edge_array_t *ea        /**< pointer to an edge_array_t struct */
);

/**
 * Frees the space used by the given edge array. 
 */
void edge_array_free(
  edge_array_t *ea /**< pointer to an edge_array_t struct */
);

/**
 * \return the value for the given edge.
 */
void * edge_array_get(
  edge_array_t *ea, /**< pointer to an edge_array_t struct */
  uint32_t      u,  /**< source node                       */
  uint32_t      v   /**< target node                       */
);

/**
 * \return a pointer to all values for edges starting from the given node.
 */
void * edge_array_get_all(
  edge_array_t *ea, /**< pointer to an edge_array_struct */
  uint32_t      u   /**< source node                     */
);

/**
 * Sets the value for the given edge.
 */
void edge_array_set(
  edge_array_t *ea, /**< pointer to an edge_array_t struct */
  uint32_t      u,  /**< source node                       */
  uint32_t      v,  /**< target node                       */
  void         *val /**< the new value                     */
);

/**
 * Sets the value for every edge starting from the given node.
 */
void edge_array_set_all(
  edge_array_t *ea,  /**< pointer to an edge_array_t struct */
  uint32_t      u,   /**< source node                       */
  void         *vals /**< array of values                   */
);

/**
 * \return the value of the given directed edge.
 */
void * edge_array_get_by_idx(
  edge_array_t *ea,  /**< pointer to an edge_array_t struct              */
  uint32_t      u,   /**< source node                                    */
  uint32_t      vidx /**< index, into u's neighbours, of the target node */
);

/**
 * Sets the value for the given directed edge.
 */
void edge_array_set_by_idx(
  edge_array_t *ea,   /**< pointer to an edge_array_t struct              */
  uint32_t      u,    /**< source node                                    */
  uint32_t      vidx, /**< index, into u's neighbours, of the target node */
  void         *val   /**< the new value                                  */
);

#endif /* __EDGE_ARRAY_H__ */
