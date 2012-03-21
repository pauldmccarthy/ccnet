/**
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_cache_approx_clustering(graph_t *g, uint32_t ntriples) {

  double clust;

  if (stats_cache_check(g, STATS_CACHE_APPROX_CLUSTERING, 0, -1, &clust) == 1)
    return clust;

  return stats_approx_clustering(g, ntriples);
}

double stats_cache_graph_clustering(graph_t *g) {

  double clust;
  
  if (stats_cache_check(g, STATS_CACHE_GRAPH_CLUSTERING, 0, -1, &clust) == 1)
    return clust;

  return stats_avg_clustering(g);
}

uint8_t stats_cache_node_clustering(graph_t *g, int64_t n, double *data) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  if (stats_cache_check(g, STATS_CACHE_NODE_CLUSTERING, n, -1, data) == 1)
    return 0;

  if (data != NULL) {
    
    if (n < 0 || n >= nnodes) {
      
      for (i = 0; i < nnodes; i++) {
        data[i] = stats_clustering(g, i);
      }
    }
    
    else {
      *data = stats_clustering(g, n);
    }
  }
  return 0;
}

double stats_cache_graph_pathlength(graph_t *g) {

  double path;

  if (stats_cache_check(g, STATS_CACHE_GRAPH_PATHLENGTH, 0, -1, &path) == 1)
    return path;

  return stats_avg_pathlength(g);
}

uint8_t stats_cache_node_pathlength(graph_t *g, int64_t n, double *data) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);
  
  if (stats_cache_check(g, STATS_CACHE_NODE_PATHLENGTH, n, -1, data) == 1)
    return 0;

  if (data != NULL) {
    
    if (n < 0 || n >= nnodes) {
      
      for (i = 0; i < nnodes; i++) {
        data[i] = stats_pathlength(g, i, NULL);
      }
    }
    
    else {
      *data = stats_pathlength(g, n, NULL);
    }
  }

  return 0;
}

uint8_t stats_cache_pair_pathlength(graph_t *g, uint32_t n, double *paths) {

  if (stats_cache_check(g, STATS_CACHE_PAIR_PATHLENGTH, n, -1, paths) == 1)
    return 0;

  stats_pathlength(g, n, paths);
  return 0;
}

double stats_cache_assortativity(graph_t *g) {

  double r;

  if (stats_cache_check(g, STATS_CACHE_ASSORTATIVITY, 0, -1, &r) == 1)
    return r;

  return stats_assortativity(g);
}

double stats_cache_num_components(graph_t *g) {

  double ncmps;

  if (stats_cache_check(g, STATS_CACHE_NUM_COMPONENTS, 0, -1, &ncmps) == 1)
    return ncmps;

  return stats_num_components(g, 1, NULL, NULL);
}

double stats_cache_largest_component(graph_t *g) {

  double lcmp;

  if (stats_cache_check(g, STATS_CACHE_LARGEST_COMPONENT, 0, -1, &lcmp) == 1)
    return lcmp;

  return stats_largest_component(g);
}

uint8_t stats_cache_node_component(graph_t *g, int64_t n, uint32_t *data) {

  uint32_t  nnodes;
  uint32_t *components;

  components = NULL;
  nnodes     = graph_num_nodes(g);

  if (stats_cache_check(g, STATS_CACHE_NODE_COMPONENT, n, -1, data) == 1)
    return 0;

  if (data != NULL) {

    if (n < 0 || n >= nnodes) {
      stats_num_components(g, 1, NULL, data);
    }

    else {

      components = calloc(nnodes,sizeof(uint32_t));
      if (components == NULL) goto fail;

      stats_num_components(g, 1, NULL, components);

      *data = components[n];

      free(components);
      components = NULL;
    }
  }
    
  return 0;
  
fail:
  if (components != NULL) free(components);
  return 1;
}

double stats_cache_connected(graph_t *g) {

  double connected;

  if (stats_cache_check(g, STATS_CACHE_CONNECTED, 0, -1, &connected) == 1)
    return connected;

  return stats_connected(g);
}

double stats_cache_global_efficiency(graph_t *g) {

  double eff;

  if (stats_cache_check(g, STATS_CACHE_GLOBAL_EFFICIENCY, 0, -1, &eff) == 1)
    return eff;

  return stats_global_efficiency(g);
}

uint8_t stats_cache_node_local_efficiency(
  graph_t *g, int64_t n, double *data) {

  int64_t  i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  if (stats_cache_check(g, STATS_CACHE_LOCAL_EFFICIENCY, n, -1, data) == 1)
    return 0;

  if (data != NULL) {

    if (n < 0 || n >= nnodes) {
      
      for (i = 0; i < nnodes; i++) {
        data[i] = stats_local_efficiency(g, i);
      }
    }
    
    else {
      *data = stats_local_efficiency(g, n);
    }
  }

  return 0;
}

double stats_cache_modularity(graph_t *g) {

  double    mod;
  uint32_t *comms;
  uint64_t  i;
  uint32_t  nnodes;
  uint32_t  ncomms;

  comms = NULL;

  if (stats_cache_check(g, STATS_CACHE_MODULARITY, 0, -1, &mod) == 1)
    return mod;

  nnodes = graph_num_nodes(    g);
  ncomms = graph_num_labelvals(g);

  comms = calloc(nnodes,sizeof(uint32_t));
  if (comms == NULL) goto fail;

  for (i = 0; i < nnodes; i++) 
    comms[i] = graph_get_nodelabel(g, i)->labelval;

  mod = stats_modularity(g, ncomms, comms);
  free(comms);
  return mod;

fail:

  if (comms != NULL) free(comms);
  return 0xFF;
}

double stats_cache_intra_edges(graph_t *g) {

  double intra;

  if (stats_cache_check(g, STATS_CACHE_INTRA_EDGES, 0, -1, &intra) == 1)
    return intra;

  return stats_num_intra_edges(g, NULL);
}

double stats_cache_inter_edges(graph_t *g) {

  double inter;

  if (stats_cache_check(g, STATS_CACHE_INTER_EDGES, 0, -1, &inter) == 1)
    return inter;
  
  stats_num_intra_edges(g, &inter);

  return inter;
}

double stats_cache_max_degree(graph_t *g) {

  double maxdeg;

  if (stats_cache_check(g, STATS_CACHE_MAX_DEGREE, 0, -1, &maxdeg) == 1)
    return maxdeg;

  maxdeg = stats_max_degree(g);


  return maxdeg;
}

uint8_t stats_cache_betweenness_centrality(
  graph_t *g, int64_t n, double *data) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  if (stats_cache_check(
      g, STATS_CACHE_BETWEENNESS_CENTRALITY, n, -1, data) == 1)
    return 0;

  if (data != NULL) {

    if (n < 0 || n >= nnodes) {

      for (i = 0; i < nnodes; i++) {
        data[i] = stats_betweenness_centrality(g, i);
      }
    }

    else {
      *data = stats_betweenness_centrality(g, n);
    }
  }

  return 0;
}

uint8_t stats_cache_node_numpaths(graph_t *g, int64_t n, double *data) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  if (stats_cache_check(g, STATS_CACHE_NODE_NUMPATHS, n, -1, data) == 1)
    return 0;

  if (data != NULL) {

    if (n < 0 || n >= nnodes) {

      for (i = 0; i < nnodes; i++) {
        data[i] = stats_numpaths(g, i, NULL);
      }
    }

    else {
      *data = stats_numpaths(g, n, NULL);
    }
  }

  return 0;
}

uint8_t stats_cache_node_edgedist(graph_t *g, int64_t n, double *data) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g); 

  if (stats_cache_check(g, STATS_CACHE_NODE_EDGEDIST, n, -1, data) == 1)
    return 0;

  if (data != NULL) {
    
    if (n < 0 || n >= nnodes) {

      for (i = 0; i < nnodes; i++) {
        data[i] = stats_avg_edge_distance(g, i);
      }
    }

    else {
      *data = stats_avg_edge_distance(g, n);
    }
  }
  


  return 0;
}

uint8_t stats_cache_pair_numpaths(graph_t *g, uint32_t n, double *paths) {

  if (stats_cache_check(g, STATS_CACHE_PAIR_NUMPATHS, n, -1, paths) == 1)
    return 0;

  stats_numpaths(g, n, paths);
  return 0;
}

uint8_t stats_cache_edge_pathsharing(graph_t *g, uint32_t n, double *ps) {

  uint64_t  i;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  double    tmp;
  
  if (stats_cache_check(g, STATS_CACHE_EDGE_PATHSHARING, n, -1, ps) == 1)
    return 0;

  nnbrs = graph_num_neighbours(g, n);
  nbrs  = graph_get_neighbours(g, n);

  for (i = 0; i < nnbrs; i++) {

    tmp = stats_edge_pathsharing(g, n, nbrs[i]);
    if (ps != NULL) ps[i] = tmp;
  }

  return 0;
}

uint8_t stats_cache_edge_betweenness(graph_t *g, uint32_t n, double *eb) {

  if (stats_cache_check(g, STATS_CACHE_EDGE_BETWEENNESS, n, -1, eb) == 1)
    return 0;

  stats_edge_betweenness(g, n, eb);
  return 0;
}
