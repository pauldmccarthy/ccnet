/**
 * Functions for querying a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "graph/graph_event.h"
#include "util/array.h"
#include "util/compare.h"

/**
 * Sub-function of graph_add_edge. Adds a directed edge from u to v,
 * increasing the capacity of the neighbour/weight list for u if required.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _graph_add_edge(
  graph_t  *g,  /**< the graph                                       */
  uint32_t  u,  /**< edge start point                                */
  uint32_t  v,  /**< edge end point                                  */
  float     wt, /**< edge weight                                     */
  uint32_t *idx /**< place to store index of v in u's neighbour list */
);

/**
 * Sub-function of graph_remove_edge. Removes a directed edge from u to v.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _graph_remove_edge(
  graph_t  *g,  /**< the graph                                       */
  uint32_t  u,  /**< edge start point                                */
  uint32_t  v,  /**< edge end point                                  */
  uint32_t *idx /**< place to store index of v in u's neighbour list */
);

uint8_t graph_is_directed(graph_t *g) {
  return ((g->flags) >> GRAPH_FLAG_DIRECTED) & 1;
}

uint32_t graph_num_nodes(graph_t *g) {
  return g->numnodes;
}

uint32_t graph_num_edges(graph_t *g) {
  return g->numedges;
}

uint32_t graph_num_labelvals(graph_t *g) {
  return g->labelvals.size;
}

uint32_t *graph_get_labelvals(graph_t *g) {
  return (uint32_t *)(g->labelvals.data);
}

graph_label_t *graph_get_nodelabel(graph_t *g, uint32_t nidx) {

  if (nidx >= g->numnodes)   return NULL;
  
  return array_getd(&(g->nodelabels), nidx);
}

uint32_t *graph_get_neighbours(graph_t *g, uint32_t nidx) {
  return (uint32_t *)(g->neighbours[nidx].data);
}

double graph_get_weight(graph_t *g, uint32_t u, uint32_t v) {

  int64_t vidx;
  float   wt;

  if (g->weights == NULL) return 0;

  vidx = graph_get_nbr_idx(g, u, v);

  if (vidx < 0) return 0;

  if (array_get(&g->weights[u], vidx, &wt)) return 0;

  return wt;
}

float *graph_get_weights(graph_t *g, uint32_t nidx) {

  return (g->weights == NULL) 
           ? NULL
           : (float *)(g->weights[nidx].data);
}

uint32_t graph_num_neighbours(graph_t *g, uint32_t nidx) {
  
  uint32_t nnbrs;

  array_get(&(g->numneighbours), nidx, &nnbrs);
  
  return nnbrs;
}

uint8_t graph_are_neighbours(graph_t *g, uint32_t u, uint32_t v) {

  uint32_t  unnbrs;
  uint32_t  vnnbrs;
  uint32_t *unbrs;
  uint32_t *vnbrs;

  unnbrs = graph_num_neighbours(g, u);
  vnnbrs = graph_num_neighbours(g, v);
  unbrs  = graph_get_neighbours(g, u);
  vnbrs  = graph_get_neighbours(g, v);

  if (bsearch(&v, unbrs, unnbrs, sizeof(uint32_t), compare_u32))
    return 1;
  if (graph_is_directed(g) && 
      bsearch(&u, vnbrs, vnnbrs, sizeof(uint32_t), compare_u32))
    return 1;

  return 0;
}

int64_t graph_get_nbr_idx(graph_t *g, uint32_t i, uint32_t j) {

  uint32_t *loc;
  uint32_t *nbrs;
  uint32_t  nnbrs;

  nnbrs = graph_num_neighbours(g, i);
  nbrs  = graph_get_neighbours(g, i);

  loc = bsearch(&j, nbrs, nnbrs, sizeof(uint32_t), compare_u32);

  if (loc == NULL) return -1;
  return loc - nbrs;
}

uint8_t graph_create(graph_t *g, uint32_t numnodes, uint8_t directed) {

  int64_t i;

  memset(g, 0, sizeof(graph_t));
  g->numnodes = numnodes;
  g->data     = NULL;
  g->datalen  = 0;
  g->flags    = 0;

  if (directed) g->flags |= 1 << GRAPH_FLAG_DIRECTED;

  if (array_create(&g->nodelabels, sizeof(graph_label_t), numnodes))
    goto fail;

  if (array_create(&g->numneighbours, sizeof(uint32_t), numnodes))
    goto fail;

  if (array_create(&g->labelvals, sizeof(uint32_t), 60))
    goto fail;
  array_set_cmps(&g->labelvals, compare_u32, compare_u32_insert);

  if (array_create(&g->event_listeners, sizeof(graph_event_listener_t), 5))
    goto fail;
  array_set_cmps(&g->event_listeners, graph_compare_event_listeners, NULL);

  g->neighbours = calloc(numnodes, sizeof(array_t));
  if (g->neighbours == NULL) goto fail;

  g->weights = calloc(numnodes, sizeof(array_t));
  if (g->weights == NULL) goto fail;

  for (i = 0; i < numnodes; i++) {
    if (array_create(&(g->neighbours[i]), sizeof(uint32_t), 10)) goto fail;
    if (array_create(&(g->weights   [i]), sizeof(float),    10)) goto fail;

    array_set_cmps(&(g->neighbours[i]), compare_u32, compare_u32_insert);
  }

  return 0;
fail:
  graph_free(g);
  return 1;
}

void graph_free(graph_t *g) {

  uint32_t i;

  if (g == NULL) return;

  array_free(&g->nodelabels);
  array_free(&g->numneighbours);
  array_free(&g->labelvals);
 
  if (g->neighbours != NULL) {
    for (i = 0; i < g->numnodes; i++) {

      if (g->neighbours[i].data != NULL) array_free(&(g->neighbours[i]));
    }
    free(g->neighbours);
  }

  if (g->weights != NULL) {
    for (i = 0; i < g->numnodes; i++) {
      if (g->weights[i].data != NULL) array_free(&(g->weights[i]));
    }
    free(g->weights);
  }

  for (i = 0; i < _GRAPH_CTX_SIZE_; i++) {

    if (g->ctx[i] != NULL && g->ctx_free[i] != NULL) 
      g->ctx_free[i](g->ctx[i]);
  }
}

uint8_t graph_copy(graph_t *gin, graph_t *gout) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  float    *wts;

  if (gin  == NULL) goto fail;
  if (gout == NULL) goto fail;

  nnodes = graph_num_nodes(gin);

  if (graph_create(         gout, nnodes, graph_is_directed(gin))) goto fail;
  if (graph_copy_nodelabels(gin,  gout))                           goto fail;

  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(gin, i);
    nbrs  = graph_get_neighbours(gin, i);
    wts   = graph_get_weights(   gin, i);

    for (j = 0; j < nnbrs; j++) {
      if (graph_add_edge(gout, i, nbrs[j], wts[j])) goto fail;
    }
  }

  return 0;
  
fail:
  return 1;
}

uint8_t graph_add_edge(
  graph_t *g, uint32_t u, uint32_t v, float wt) {

  uint32_t         nu;
  uint32_t         nv;
  edge_added_ctx_t ctx;
  uint32_t         uidx;
  uint32_t         vidx;

  uidx = 0;
  vidx = 0;

  if (g == NULL)                     goto fail;
  if (u == v)                        goto fail;
  if (u >= g->numnodes)              goto fail;
  if (v >= g->numnodes)              goto fail;
  if (graph_are_neighbours(g, u, v)) return 0;

  nu = (u > v) ? v : u;
  nv = (u > v) ? u : v;

  if (_graph_add_edge(g, nu, nv, wt, &vidx)) goto fail;
  if (!graph_is_directed(g)) {
    if (_graph_add_edge(g, nv, nu, wt, &uidx)) goto fail;
  }

  g->numedges ++;

  ctx.u        = u;
  ctx.v        = v;
  ctx.uidx     = uidx;
  ctx.vidx     = vidx;
  ctx.wt       = wt;
  graph_event_fire(g, GRAPH_EVENT_EDGE_ADDED, &ctx);

  return 0;

fail:
  return 1;
}

uint8_t _graph_add_edge(
  graph_t *g, uint32_t u, uint32_t v, float wt, uint32_t *idx) {

  uint32_t  vidx;
  uint32_t *nnbrs;

  if (array_insert_sorted(&(g->neighbours[u]), &v, 1, &vidx))
    goto fail;
  
  if (array_insert(&(g->weights[u]), vidx, &wt)) goto fail;

  nnbrs = array_getd(&g->numneighbours, u);
  (*nnbrs)++;

  if (idx != NULL) *idx = vidx;

  return 0;
fail:
  return 1;
}

uint8_t graph_remove_edge(
  graph_t *g, uint32_t u, uint32_t v) {

  uint32_t           nnodes;
  edge_removed_ctx_t ctx;
  uint32_t           uidx;
  uint32_t           vidx;

  if (g == NULL) goto fail;

  nnodes = graph_num_nodes(g);
  uidx   = 0;
  vidx   = 0;
  
  if (u >= nnodes) goto fail;
  if (v >= nnodes) goto fail;

  if (             _graph_remove_edge(g, u, v, &vidx)) goto fail;
  if (!graph_is_directed(g) &&
                   _graph_remove_edge(g, v, u, &uidx)) goto fail;

  g->numedges--;

  ctx.u        = u;
  ctx.v        = v;
  ctx.uidx     = uidx;
  ctx.vidx     = vidx;
  graph_event_fire(g, GRAPH_EVENT_EDGE_REMOVED, &ctx); 

  return 0;
  
fail:
  return 1;
}

uint8_t _graph_remove_edge(
  graph_t *g, uint32_t u, uint32_t v, uint32_t *idx) {

  int64_t   vidx;
  uint32_t *nnbrs;

  vidx = graph_get_nbr_idx(g, u, v);

  if (vidx < 0) goto fail;

  array_remove_by_idx(&(g->neighbours[u]), vidx);
  array_remove_by_idx(&(g->weights   [u]), vidx);

  nnbrs = array_getd(&g->numneighbours, u);
  
  (*nnbrs)--;

  if (idx != NULL) *idx = vidx;

  return 0;
fail:
  return 1;
}

uint8_t graph_set_nodelabel(graph_t *g, uint32_t nid, graph_label_t *lbl) {

  graph_label_t newlbl;

  if (g   == NULL)        return 1;
  if (nid >= g->numnodes) return 1;

  if (lbl == NULL) memset(&newlbl, 0,   sizeof(graph_label_t));
  else             memcpy(&newlbl, lbl, sizeof(graph_label_t));

  array_set(&g->nodelabels, nid, &newlbl);

  if (array_insert_sorted(&g->labelvals,
                          &(newlbl.labelval),
                          1,
                          NULL) == 2) return 1;
  
  return 0;
}

uint8_t graph_copy_nodelabels(graph_t *gin, graph_t *gout) {

  uint64_t       i;
  graph_label_t *lbl;

  if (gin           == NULL)           goto fail;
  if (gout          == NULL)           goto fail;
  if (gin->numnodes != gout->numnodes) goto fail;

  for (i = 0; i < gin->numnodes; i++) {

    lbl = graph_get_nodelabel(gin, i);
    if (lbl == NULL) goto fail;

    if (graph_set_nodelabel(gout, i, lbl)) goto fail;
  }

  return 0;
  
fail:
  return 1;
}
