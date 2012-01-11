/**
 * Manages an array of values for the edges in a graph.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/graph_event.h"
#include "util/edge_array.h"

/**
 * Called when an edge is added to the graph
 */
static void _edge_added(
  graph_t *g,    /**< the graph                         */
  void    *ctx,  /**< listener context                  */
  uint32_t u,    /**< edge start point                  */
  uint32_t v,    /**< edge end point                    */
  uint32_t uidx, /**< index of u, in v's neighbour list */
  uint32_t vidx, /**< index of v, in u's neighbour list */
  float    wt    /**< edge weight                       */
);

/**
 * Called when an edge is removed from the graph
 */
static void _edge_removed(
  graph_t *g,    /**< the graph             */
  void    *ctx,  /**< listener context      */
  uint32_t u,    /**< edge start point      */
  uint32_t v,    /**< edge end point        */
  uint32_t uidx, /**< index of u, in v's neighbour list */
  uint32_t vidx  /**< index of v, in u's neighbour list */
);

uint8_t edge_array_create(
  graph_t *g, uint16_t valsz, edge_array_t *ea) {

  uint64_t i;
  uint32_t nnodes;
  uint32_t nnbrs;

  ea->g        = g;
  ea->valsz    = valsz;
  ea->vals     = NULL;

  nnodes = graph_num_nodes(g);

  ea->vals = calloc(nnodes, sizeof(array_t));
  if (ea->vals == NULL) goto fail;

  for (i = 0; i < nnodes; i++) {
    
    nnbrs = graph_num_neighbours(g, i);
    if (array_create(&ea->vals[i], valsz, nnbrs))
      goto fail;
  }

  memset(&ea->gel, 0, sizeof(graph_event_listener_t));
  ea->gel.edge_added   = _edge_added;
  ea->gel.edge_removed = _edge_removed;
  ea->gel.ctx          = ea;
  graph_add_event_listener(g, &ea->gel);
  
  return 0;
  
fail:
  if (ea->vals != NULL) {
    for (i = 0; i < nnodes; i++) {
      if (ea->vals[i].data != NULL) array_free(&ea->vals[i]);
    }
    free(ea->vals);
  }
                          
  return 1;
}

void edge_array_free(edge_array_t *ea) {

  uint64_t i;
  uint32_t nnodes;
  
  if (ea == NULL) return;

  graph_remove_event_listener(ea->g, &ea->gel);

  if (ea->vals != NULL) {
    
    nnodes = graph_num_nodes(ea->g);
    
    for (i = 0; i < nnodes; i++) 
      array_free(&ea->vals[i]);
    
    free(ea->vals);
  }

  ea->g    = NULL;
  ea->vals = NULL;
}

void * edge_array_get(edge_array_t *ea, uint32_t u, uint32_t v) {

  uint32_t vidx;

  vidx = graph_get_nbr_idx(ea->g, u, v);

  return edge_array_get_by_idx(ea, u, vidx);
}

void * edge_array_get_all(edge_array_t *ea, uint32_t u) {

  return ea->vals[u].data;
}

void edge_array_set(edge_array_t *ea, uint32_t u, uint32_t v, void * val) {

  uint32_t uidx;
  uint32_t vidx;

  vidx = graph_get_nbr_idx(ea->g, u, v);
  edge_array_set_by_idx(ea, u, vidx, val);
  
  if (!graph_is_directed(ea->g)) {
    
    uidx = graph_get_nbr_idx(ea->g, v, u);
    edge_array_set_by_idx(ea, v, uidx, val);
  }
}

void edge_array_set_all(edge_array_t *ea, uint32_t u, void *vals) {

  uint64_t i;
  uint32_t nnbrs;
  uint8_t *uvals;

  uvals = vals;

  nnbrs = graph_num_neighbours(ea->g, u);

  for (i = 0; i < nnbrs; i++) 
    array_set(&ea->vals[u], i, uvals + (i * (ea->valsz)));
}

void * edge_array_get_by_idx(edge_array_t *ea, uint32_t u, uint32_t vidx) {

  return array_getd(&ea->vals[u], vidx);
}

void edge_array_set_by_idx(
  edge_array_t *ea, uint32_t u, uint32_t vidx, void * val) {
  
  array_set(&ea->vals[u], vidx, val);
}


void _edge_added(
  graph_t *g,
  void    *ctx,
  uint32_t u,
  uint32_t v,
  uint32_t uidx,
  uint32_t vidx,
  float    wt) {

  uint8_t *data;
  data = NULL;
  
  edge_array_t *ea;
  ea = ctx;

  data = calloc(ea->valsz, 1);
  if (data == NULL) goto fail;

  if (array_insert(&ea->vals[u], vidx, data)) goto fail;

  if (!graph_is_directed(ea->g) &&
      array_insert(&ea->vals[v], uidx, data)) goto fail;

  free(data);
  
fail:
  return;
}

void _edge_removed(
  graph_t *g,
  void    *ctx,
  uint32_t u,
  uint32_t v,
  uint32_t uidx,
  uint32_t vidx) {

  edge_array_t *ea;
  ea = ctx;

  array_remove_by_idx(&ea->vals[u], vidx);
  if (!graph_is_directed(ea->g))
    array_remove_by_idx(&ea->vals[v], uidx);
}
