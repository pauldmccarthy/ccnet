/**
 * Functions which count the number of within- and between-cluster edges.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <math.h>
#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_num_intra_edges(graph_t *g, double *inter_out) {

  uint64_t       i;
  uint64_t       j;
  uint32_t       nnodes;
  uint32_t       nnbrs;
  uint32_t      *nbrs;
  graph_label_t *ilbl;
  graph_label_t *jlbl;
  double         intra;
  double         inter;

  if (graph_num_labelvals(g) <= 1) {
    if (inter_out != NULL) *inter_out = 0;
    return graph_num_edges(g);
  }

  intra  = 0;
  inter  = 0;
  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);
    ilbl  = graph_get_nodelabel( g, i);
    
    for (j = 0; j < nnbrs; j++) {

      if (i > nbrs[j]) continue;

      jlbl = graph_get_nodelabel(g, nbrs[j]);

      if (ilbl->labelval == jlbl->labelval) intra ++;
      else                                  inter ++;
    }
  }

  stats_cache_add(g,
                  STATS_CACHE_INTRA_EDGES,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_add(g,
                  STATS_CACHE_INTER_EDGES,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_INTRA_EDGES, 0, -1, &intra);
  stats_cache_update(g, STATS_CACHE_INTER_EDGES, 0, -1, &inter);

  if (inter_out != NULL) *inter_out = inter;
  return intra;
}

double stats_edge_distance(graph_t *g, uint32_t u, uint32_t v) {

  double         dist;
  double         dx;
  double         dy;
  double         dz;
  graph_label_t *lu;
  graph_label_t *lv;

  lu = graph_get_nodelabel(g, u);
  lv = graph_get_nodelabel(g, v);

  if (lu == NULL) return -1.0;
  if (lv == NULL) return -1.0;

  dx = lu->xval - lv->xval;
  dy = lu->yval - lv->yval;
  dz = lu->zval - lv->zval;

  dist = pow((dx*dx) + (dy*dy) + (dz*dz), 0.5);
  
  return dist;
}

double stats_avg_edge_distance(graph_t *g, uint32_t u) {

  uint64_t  i;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  double    avg;

  
  avg   = 0;
  nnbrs = graph_num_neighbours(g, u);
  nbrs  = graph_get_neighbours(g, u);

  for (i = 0; i < nnbrs; i++) {

    avg += stats_edge_distance(g, u, nbrs[i]);
  }

  avg /= nnbrs;

  stats_cache_add(g,
                  STATS_CACHE_NODE_EDGEDIST,
                  STATS_CACHE_TYPE_NODE,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_NODE_EDGEDIST, u, -1, &avg); 

  return avg;
}
