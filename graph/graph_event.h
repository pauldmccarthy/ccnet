/**
 * Framework for being notified of events on a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __GRAPH_EVENT_H__
#define __GRAPH_EVENT_H__

#include <stdint.h>

#include "graph/graph.h"

/**
 * You can register to listen for events on a graph_t struct, by filling in
 * this struct, and passing it to the graph_add_event_listener function. You
 * can de-register your listener at some point in the future, by passing the
 * same struct to the graph_remove_event_listener function (see the comment
 * for the id field in the struct though).
 *
 * Set the callback function to NULL for events you are not interested in.
 */
typedef struct _graph_event_listener {

  uint64_t id;         /**< ID of this event listener - this is set by
                            the graph_add_event_listener function, and
                            is used to identify this listener, so don't
                            change it.                                     */

  void *ctx;           /**< Listener context - anything can be stored
                            here. It is passed to event callbacks.         */
  
  void (*edge_added) ( /**< Called when an edge is added to the graph      */
    graph_t *g,        /**< the graph                                      */
    void    *ctx,      /**< listener context                               */
    uint32_t u,        /**< edge start point                               */
    uint32_t v,        /**< edge end point                                 */
    uint32_t uidx,     /**< index of u, in v's neighbour
                            list (if undirected)                           */
    uint32_t vidx,     /**< index of v, in u's neighbour list              */
    float    wt        /**< edge weight                                    */
  );
 
  void (*edge_removed)( /**< Called when an edge is removed from the graph */
    graph_t *g,         /**< the graph                                     */
    void    *ctx,       /**< listener context                              */
    uint32_t u,         /**< edge start point                              */
    uint32_t v,         /**< edge end point                                */
    uint32_t uidx,      /**< index of u, in v's neighbour
                             list (if undirected)                          */
    uint32_t vidx       /**< index of v, in u's neighbour list             */
  );

} graph_event_listener_t;

/**
 * Event identifiers.
 */
typedef enum {
  
  GRAPH_EVENT_EDGE_ADDED,
  GRAPH_EVENT_EDGE_REMOVED,
  
} graph_event_t;

/**
 * Context struct for the edge added event.
 */
typedef struct _edge_added_ctx {

  uint32_t u;
  uint32_t v;
  float    wt;
  uint32_t uidx;
  uint32_t vidx;

} edge_added_ctx_t;

/**
 * Context struct for the edge removed event.
 */
typedef struct _edge_removed_ctx {

  uint32_t u;
  uint32_t v;
  uint32_t uidx;
  uint32_t vidx;
  
} edge_removed_ctx_t;

/**
 * Registers the given listener with the given graph.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_add_event_listener(
  graph_t                 *g, /**< the graph    */
  graph_event_listener_t  *l  /**< the listener */
);

/**
 * De-registers the given listener from the given graph.
 */
void graph_remove_event_listener(
  graph_t                 *g, /**< the graph    */
  graph_event_listener_t  *l  /**< the listener */
);

/**
 * Fires the event for the given graph.
 */
void graph_event_fire(
  graph_t       *g,    /**< the graph     */
  graph_event_t  type, /**< event type    */
  void          *ctx   /**< event context */
);

/**
 * Equality function for graph_event_listener_t structs.
 *
 * \return 0 if a->id == b->id, non-0 otherwise.
 */
int graph_compare_event_listeners(
  const void *a, /**< pointer to a graph_event_listener_t struct       */
  const void *b  /**< pointer to another graph_event_listener_t struct */
);

#endif /* __GRAPH_EVENT_H__ */
