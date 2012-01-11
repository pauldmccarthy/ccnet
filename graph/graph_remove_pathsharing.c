/**
 * Remove edges from a graph based on pathsharing.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "graph/graph.h"
#include "graph/graph_threshold.h"


uint8_t graph_init_pathsharing(graph_t *g) {
  
  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;

  nnodes = graph_num_nodes(g);
  
  for (i = 0; i < nnodes; i++) {
    
    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);

    for (j = 0; j < nnbrs; j++) {

      if (i > nbrs[j]) continue;

      stats_edge_pathsharing(g, i, nbrs[j]);
    }
  }

  return 0;
}

uint8_t graph_remove_pathsharing(
  graph_t *g, double *share, array_t *edges, graph_edge_t *edge) {

  uint64_t     i;
  uint64_t     j;
  uint32_t     nnodes;
  uint32_t     nnbrs;
  uint32_t    *nbrs;
  double       min;

  min    = 1.0;
  nnodes = graph_num_nodes(g);

  /*find the edges with the minimum path-sharing value*/
  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);
    stats_cache_edge_pathsharing(g, i, share);

    for (j = 0; j < nnbrs; j++) {
      
      if (i        > nbrs[j]) continue;
      if (share[j] > min)     continue;

      edge->u = i;
      edge->v = nbrs[j];

      if (share[j] < min) {
        array_clear(edges);
        min = share[j];
      }

      if (array_append(edges, edge)) goto fail;
    }
  }

  /*randomly remove one of those edges*/
  i = floor((edges->size) * ((double)random() / RAND_MAX));

  if (array_get(edges, i, edge))              goto fail;
  if (graph_remove_edge(g, edge->u, edge->v)) goto fail;

  return 0;
  
fail:
  return 1;
}

uint8_t graph_recalculate_pathsharing(graph_t *g, graph_edge_t *edge) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  unnbrs;
  uint32_t *unbrs;
  uint32_t  vnnbrs;
  uint32_t *vnbrs;

  unnbrs = graph_num_neighbours(g, edge->u);
  unbrs  = graph_get_neighbours(g, edge->u);
  vnnbrs = graph_num_neighbours(g, edge->v);
  vnbrs  = graph_get_neighbours(g, edge->v);

  for (i = 0; i < unnbrs; i++)
    stats_edge_pathsharing(g, edge->u, unbrs[i]);
  
  for (i = 0; i < vnnbrs; i++) 
    stats_edge_pathsharing(g, edge->v, vnbrs[i]);
  
  for (i = 0; i < unnbrs; i++)
    for (j = 0; j < vnnbrs; j++)
      stats_edge_pathsharing(g, unbrs[i],vnbrs[j]);

  return 0;
}
