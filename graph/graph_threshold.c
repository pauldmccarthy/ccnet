/**
 * Various methods of removing edges from graphs, including:
 * 
 *   - removing edges by thresholding their weight value
 *   - removing a specified number of edges, according to some criteria,
 *     e.g. edge-betweenness or path-sharing
 *   - removing edges until a specified number of components has formed, 
 *     again, according to some criteria
 *   - removing edges until modularity is maximised, again, according to
 *     some criteria
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "graph/graph_threshold.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

static uint8_t _threshold_edges(
  graph_t *gin, graph_t *gout, double threshold);

uint8_t graph_threshold_weight(
  graph_t *gin, graph_t *gout, double threshold) {

  if (graph_create(         gout, graph_num_nodes(gin), 0)) goto fail;
  if (graph_copy_nodelabels(gin,  gout))                    goto fail;
  if (_threshold_edges(     gin,  gout, threshold))         goto fail;

  return 0;

fail:
  return 1;
}

uint8_t graph_threshold_edges(
  graph_t  *gin,
  graph_t  *gout,
  uint32_t  nedges,
  uint32_t  flags,
  void     *opt,
  uint8_t (*init)(graph_t *g),
  uint8_t (*remove)(
    graph_t      *g,
    double       *space,
    array_t      *edges,
    graph_edge_t *edge),
  uint8_t (*recalc)(
    graph_t      *g,
    graph_edge_t *edge)
) {

  uint64_t     i;
  uint32_t     nnodes;
  double      *space;
  array_t      edges;
  graph_edge_t edge;

  edges.data = NULL;
  space      = NULL;
  nnodes     = graph_num_nodes(gin);

  if (array_create(&edges, sizeof(graph_edge_t), 10)) goto fail;

  if (graph_copy(gin,  gout)) goto fail;
  if (stats_cache_init(gout)) goto fail;

  space = calloc(nnodes,sizeof(double));
  if (space == NULL) goto fail;

  init(gout);

  for (i = 0; i < nedges; i++) {

    array_clear(&edges);
    if (remove(gout, space, &edges, &edge)) goto fail;

    if (i == nedges -1)      break;
    if (recalc(gout, &edge)) goto fail;
  }

  array_free(&edges);
  free(space);
  return 0;
  
fail:
  if (edges.data != NULL) array_free(&edges);
  if (space      != NULL) free(space);
  return 1;
}

uint8_t graph_threshold_components(
  graph_t  *gin,
  graph_t  *gout,
  uint32_t  cmplimit,
  uint32_t  igndis,
  void     *opt,
  uint8_t (*init)(graph_t *g),
  uint8_t (*remove)(
    graph_t      *g,
    double       *space,
    array_t      *edges,
    graph_edge_t *edge),
  uint8_t (*recalc)(
    graph_t      *g,
    graph_edge_t *edge)
) {
  uint64_t     i;
  uint32_t     sz;
  uint32_t     nnodes;
  uint64_t     ncmps;
  double      *space;
  array_t      edges;
  graph_edge_t edge;
  array_t      cmpszs;

  edges.data  = NULL;
  cmpszs.data = NULL;
  space       = NULL;
  nnodes      = graph_num_nodes(gin);

  if (cmplimit > nnodes) goto fail;

  if (array_create(&edges,  sizeof(graph_edge_t), 10)) goto fail;
  if (array_create(&cmpszs, sizeof(uint32_t),     10)) goto fail;

  if (graph_copy(gin,  gout)) goto fail;
  if (stats_cache_init(gout)) goto fail;

  space = calloc(nnodes,sizeof(double));
  if (space == NULL) goto fail;

  init(gout);
  
  ncmps = stats_num_components(gout, 1, NULL, NULL);
  while (ncmps < cmplimit) {

    array_clear(&edges);
    array_clear(&cmpszs);

    if (remove(gout, space, &edges, &edge)) goto fail;

    ncmps = stats_num_components(gout, 1, &cmpszs, NULL);

    if (igndis) {
      for (i = 0; i < cmpszs.size; i++) {
      
        if (array_get(&cmpszs, i, &sz)) goto fail;
        if (sz <= igndis) ncmps--;
      }
    }

    if (ncmps >= cmplimit)   break;
    if (recalc(gout, &edge)) goto fail;
  }

  free(space);
  array_free(&edges);
  array_free(&cmpszs);
  return 0;
  
fail:
  if (space       != NULL) free(space);
  if (edges.data  != NULL) array_free(&edges);
  if (cmpszs.data != NULL) array_free(&cmpszs);
  return 1;
}

uint8_t graph_threshold_modularity(
  graph_t  *gin,
  graph_t  *gout,
  uint32_t  edgelimit,
  uint32_t  flags,
  void     *opt,
  uint8_t (*init)(graph_t *g),
  uint8_t (*remove)(
    graph_t      *g,
    double       *space,
    array_t      *edges,
    graph_edge_t *edge),
  uint8_t (*recalc)(
    graph_t      *g,
    graph_edge_t *edge)
) {
  uint32_t     nnodes;
  uint64_t     i;
  graph_t      lgin;
  array_t      edges;
  graph_edge_t edge;
  graph_t      gmod;
  double      *space;
  double       mod;
  double       maxmod;
  uint32_t     ncmps;
  uint32_t    *components;
  mod_opt_t   *modopt;

  maxmod          = -1.0;
  modopt          = opt;
  space           = NULL;
  edges.data      = NULL;
  gmod.neighbours = NULL;
  components      = NULL;

  if (modopt != NULL) {
    modopt->modularity = NULL;
    modopt->ncmps      = NULL;
    modopt->nvals      = edgelimit;
  }

  nnodes = graph_num_nodes(gin);

  if (graph_copy(gin,  &lgin)) goto fail;
  if (stats_cache_init(&lgin)) goto fail;

  if (array_create(&edges, sizeof(graph_edge_t), 10)) goto fail;

  components = calloc(nnodes,sizeof(uint32_t));
  if (components == NULL) goto fail;

  space = calloc(nnodes,sizeof(double));
  if (space == NULL) goto fail;

  if (modopt != NULL) {
    modopt->modularity = calloc(edgelimit,sizeof(double));
    if (modopt->modularity == NULL) goto fail;

    modopt->ncmps = calloc(edgelimit,sizeof(uint32_t));
    if (modopt->ncmps == NULL) goto fail;
  }

  init(&lgin);

  for (i = 0; i < edgelimit; i++) {

    array_clear(&edges);
    if (remove(&lgin, space, &edges, &edge)) goto fail;

    /*
     * modularity is calculated on the original graph, with 
     * the discovered components as the community structure
     */
    ncmps = stats_num_components(&lgin, 0, NULL, components);
    mod   = stats_modularity(gin, ncmps, components);

    if (modopt != NULL) {
      modopt->modularity[i] = mod;
      modopt->ncmps[i]      = ncmps;
    }

    if (mod >= maxmod) {

      maxmod = mod;

      if (gmod.neighbours != NULL) 
        graph_free(&gmod);
      gmod.neighbours = NULL;
      if (graph_copy(&lgin, &gmod)) goto fail;
    }

    if (recalc(&lgin, &edge)) goto fail;
  }

  if (graph_copy(&gmod, gout)) goto fail;

  free(space);
  array_free(&edges);

  return 0;

fail:
  if (space           != NULL) free(space);
  if (edges.data      != NULL) array_free(&edges);
  if (gmod.neighbours != NULL) graph_free(&gmod);
  if (components      != NULL) free(components);

  if (modopt != NULL) {
    if (modopt->modularity != NULL) free(modopt->modularity);
    if (modopt->ncmps      != NULL) free(modopt->ncmps);
  }
  return 1;
}

uint8_t _threshold_edges(graph_t *gin, graph_t *gout, double threshold) {

  uint32_t  u;
  uint32_t  v;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  float    *wts;

  nnodes = graph_num_nodes(gin);

  for (u = 0; u < nnodes; u++) {

    nnbrs = graph_num_neighbours(gin, u);
    nbrs  = graph_get_neighbours(gin, u);
    wts   = graph_get_weights   (gin, u);

    if (wts == NULL) goto fail;

    for (v = 0; v < nnbrs; v++) {

      if (wts[v] < threshold)                       continue;
      if (graph_add_edge(gout, u, nbrs[v], wts[v])) goto fail;
    }
  }

  return 0;

fail:
  return 1;
}
