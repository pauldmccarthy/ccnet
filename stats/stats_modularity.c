/**
 * Function which calculates the modularity of the given graph; modularity
 * gives an indication of the extent to which the graph is made up of densely
 * connected communities. The communities in this case are defined by node
 * label values. This file implements modularity as described in Newman & 
 * Girvan 2004; an alternate, but equivalent, definition is given in Newman 
 * 2006.
 *
 *   MEJ Newman & M Girvan 2004. Finding and evaluating community
 *   structure in networks. Physical Review E (69) 026113.
 *
 *   MEJ Newman 2006. Modularity and community structure
 *   in networks. PNAS, vol. 103, no. 23, pp. 8577-8582.
 *
 * Assumes that the graph is undirected.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "util/array.h"
#include "util/compare.h"

double stats_modularity(
  graph_t *g, uint32_t ncommunities, uint32_t *communities) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nedges;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  double   *mod_matrix;
  double    step;
  double    sum;
  double    mod;
  uint32_t  icom;
  uint32_t  jcom;

  mod_matrix = NULL;

  nnodes  = graph_num_nodes(g);
  nedges  = graph_num_edges(g);
  
  step    = 0.5 / nedges;

  mod_matrix = calloc(ncommunities*ncommunities,sizeof(double));
  if (mod_matrix == NULL) goto fail;

  /*
   * construct the modularity matrix, containing fractions 
   * of all edges within and between each community
   */
  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);
    icom  = communities[i];

    for (j = 0; j < nnbrs; j++) {

      jcom = communities[nbrs[j]];

      mod_matrix[(icom*ncommunities) + jcom] += step;
    }
  }

  mod = 0;
  for (i = 0; i < ncommunities; i++) {

    sum = 0;
    for (j = 0; j < ncommunities; j++)  
      sum += mod_matrix[(i*ncommunities) + j];

    mod += (mod_matrix[(i*ncommunities) + i] - (sum*sum));
  }

  free(mod_matrix);
  return mod;

fail:
  if (mod_matrix != NULL) free(mod_matrix);
  return 0xFF;
}
