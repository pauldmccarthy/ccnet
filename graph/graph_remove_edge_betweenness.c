/**
 * Function which thresholds edges of a graph based on their edge betweennes
 * value.
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

uint8_t graph_init_edge_betweenness(graph_t *g) {

  stats_edge_betweenness(g, 0, NULL);

  return 0;
}

uint8_t graph_remove_edge_betweenness(
  graph_t *g, double *betw, array_t *edges, graph_edge_t *edge) {

  uint64_t     i;
  uint64_t     j;
  uint32_t     nnodes;
  uint32_t     nnbrs;
  uint32_t    *nbrs;
  double       max;

  max    = 0;
  nnodes = graph_num_nodes(g);

  /*find the edges with the maximum edge-betweenness value*/
  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);

    stats_cache_edge_betweenness(g, i, betw);

    for (j = 0; j < nnbrs; j++) {

      if (i       > nbrs[j]) continue;
      if (betw[j] < max)     continue;

      edge->u = i;
      edge->v = nbrs[j];

      if (betw[j] > max) {
        array_clear(edges);
        max = betw[j];
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

uint8_t graph_recalculate_edge_betweenness(graph_t *g, graph_edge_t *edge) {

  uint64_t  i;
  uint32_t  nnodes;
  double    icmp;
  double    ucmp;
  double    vcmp;
  uint32_t *components;

  components = NULL;
  nnodes     = graph_num_nodes(g);

  /*
   * I'm assuming that component IDs have been recalculated
   * in the graph_threshold_components function, in
   * graph_threshold.c
   */
  components = calloc(nnodes,sizeof(uint32_t));
  if (components == NULL) goto fail;

  stats_cache_node_component(g, -1, components);
  ucmp = components[edge->u];
  vcmp = components[edge->v];

  for (i = 0; i < nnodes; i++) {

    if (i == edge->u || i == edge->v) continue;

    icmp = components[i];

    if (icmp == ucmp || icmp == vcmp) {
      
      stats_pathlength(g, i, NULL);
      stats_numpaths(  g, i, NULL);
    }
  }

  stats_pathlength(g, edge->u, NULL);
  stats_pathlength(g, edge->v, NULL);
  stats_numpaths(  g, edge->u, NULL);
  stats_numpaths(  g, edge->v, NULL);

  stats_edge_betweenness(g, edge->u, NULL);
  
  if (ucmp != vcmp)
    stats_edge_betweenness(g, edge->v, NULL);

  free(components);
  return 0;
  
fail:
  if (components != NULL) free(components);
  return 1;
}
