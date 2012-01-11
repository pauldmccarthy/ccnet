/**
 * Framework for notifications of graph events.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdlib.h>
#include <stdint.h>

#include "graph/graph.h"
#include "graph/graph_event.h"
#include "util/array.h"

/**
 * Global counter used for assigning IDs to event listeners.
 */
static uint64_t _id_counter = 0;

/**
 * Fires the edge added event on all listeners.
 */
static void _fire_edge_added(
  graph_t          *g,
  edge_added_ctx_t *ctx
);

/**
 * Fires the edge removed event on all listeners.
 */
static void _fire_edge_removed(
  graph_t            *g,
  edge_removed_ctx_t *ctx
);

uint8_t graph_add_event_listener(graph_t *g, graph_event_listener_t *l) {

  l->id = _id_counter ++;

  if (array_append(&g->event_listeners, l)) return 1;

  return 0;
}

void graph_remove_event_listener(graph_t *g, graph_event_listener_t *l) {

  array_remove_by_val(&g->event_listeners, l, 0);
}

void graph_event_fire(graph_t *g, graph_event_t type, void *ctx) {

  switch(type) {
    case GRAPH_EVENT_EDGE_ADDED:   _fire_edge_added(  g, ctx); break;
    case GRAPH_EVENT_EDGE_REMOVED: _fire_edge_removed(g, ctx); break;
  }
}

void _fire_edge_added(graph_t *g, edge_added_ctx_t *ctx) {

  uint64_t               i;
  graph_event_listener_t l;

  for (i = 0; i < g->event_listeners.size; i++) {
    
    if (array_get(&g->event_listeners, i, &l)) return;
    if (!l.edge_added) continue;
    l.edge_added(g, l.ctx, ctx->u, ctx->v, ctx->uidx, ctx->vidx, ctx->wt);
  }
}

void _fire_edge_removed(graph_t *g, edge_removed_ctx_t *ctx) {

  uint64_t               i;
  graph_event_listener_t l;

  for (i = 0; i < g->event_listeners.size; i++) {

    if (array_get(&g->event_listeners, i, &l)) return;
    if (!l.edge_removed) continue;
    l.edge_removed(g, l.ctx, ctx->u, ctx->v, ctx->uidx, ctx->vidx);
  }
}

int graph_compare_event_listeners(const void *a, const void *b) {

  graph_event_listener_t *gela;
  graph_event_listener_t *gelb;

  gela = (graph_event_listener_t *)a;
  gelb = (graph_event_listener_t *)b;

  if (gela->id == gelb->id) return 0;
  return 1;
}
