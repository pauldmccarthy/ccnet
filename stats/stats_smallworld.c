/**
 * Function for calculating the small world index of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <math.h>
#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"


double stats_local_smallworld_index(graph_t *g, uint32_t n) {

  double clustering;
  double pathlength;
  double randClustering;
  double randPathlength;
  double gamma;
  double lambda;

  stats_cache_node_pathlength(g, n, &pathlength);
  stats_cache_node_clustering(g, n, &clustering);

  randClustering = stats_er_clustering(g);
  randPathlength = stats_er_pathlength(g);

  gamma  = clustering / randClustering;
  lambda = pathlength / randPathlength;

  return gamma / lambda;
}


double stats_smallworld_index(graph_t *g) {

  double   clustering;
  double   pathlength;
  double   randClustering;
  double   randPathlength;
  double   gamma;
  double   lambda;

  pathlength = stats_cache_graph_pathlength(g);
  clustering = stats_cache_graph_clustering(g);

  randClustering = stats_er_clustering(g);
  randPathlength = stats_er_pathlength(g);

  gamma  = clustering / randClustering;
  lambda = pathlength / randPathlength;

  return gamma / lambda;
}


double stats_er_pathlength(graph_t *g) {

  double randPathlength;
  double numnodes;
  double degree;

  numnodes = graph_num_nodes( g);
  degree   = stats_avg_degree(g);

  /*
   * Approximation of characteristic path length in an 
   * Erdos-Renyi random graph :
   *
   *   Fronczak A, Fronczak P, Holyst JA. Average path 
   *   length in random networks. Physical Review E(70) 
   *   056110-1-7, 2004
   */
  randPathlength = 0.5 + (log(numnodes) - 0.5772) / log(degree);

  return randPathlength;
}

double stats_er_clustering(graph_t *g) {

  /*
   * In an Erdos Renyi random graph, the clustering 
   * coefficient of any node is, on average, equal to
   * the density of the graph.
   */
  return stats_density(g);
}
