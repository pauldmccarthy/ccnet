/**
 * Provides a function which calculates the assortativity of a graph.
 *
 * Definition of assortativity:
 *
 *   Newman MEJ 2002. Assortative mixing in networks.
 *   Physical Review Letters, Vol. 89, No. 2.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <math.h>
#include <stdint.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

double stats_assortativity(graph_t *g) {

  uint32_t  j;
  uint32_t  k;
  uint32_t  numnodes;
  uint32_t  numedges;
  uint32_t *nbrsj;
  uint32_t  degj;
  uint32_t  degk;
  double    r;

  /*
   * these bits are used for the summations which 
   * form most of the assortativity equation
   */
  double    jkprod;
  double    jkhalfsum;
  double    jkhalfsumsq;

  jkprod      = 0;
  jkhalfsum   = 0;
  jkhalfsumsq = 0;

  numnodes = graph_num_nodes(g);
  numedges = graph_num_edges(g);

  /*iterate through every edge in the graph*/
  for (j = 0; j < numnodes; j++) {

    degj  = graph_num_neighbours(g, j);
    nbrsj = graph_get_neighbours(g, j);

    for (k = 0; k < degj; k++) {

      /*undirected - we only count edges once*/
      if (nbrsj[k] <= j) continue;

      degk = graph_num_neighbours(g, nbrsj[k]);

      jkprod      +=         degj * degk;
      jkhalfsum   += 0.5 *  (degj + degk);
      jkhalfsumsq += 0.5 * ((degj * degj) + (degk * degk));
    }
  }

  r  = (jkprod      / numedges) - (pow(jkhalfsum / numedges, 2));
  r /= (jkhalfsumsq / numedges) - (pow(jkhalfsum / numedges, 2));

  stats_cache_add(g,
                  STATS_CACHE_ASSORTATIVITY,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_ASSORTATIVITY, 0, -1, &r);

  return r;
}
