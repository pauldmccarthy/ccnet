/**
 * Randomly generate a clustered graph.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "graph/graph.h"
#include "util/array.h"

/**
 * Randomly generates sizes for each cluster over the range [clustersz -
 * (range*clustersz),clustersz+(range*clustersz)]. Range must be a value
 * between 0.0 and 1.0. The sum of the sizes of all generated clusters will,
 * on average, be equal to nnodes. The given sizes array is populated with
 * node index boundaries, rather than cluster sizes.
 *
 * \return non-0 on invalid input, 0 otherwise.
 */
static uint8_t _create_sizes(
  uint32_t nnodes,    /**< desired number of nodes     */
  uint32_t nclusters, /**< number of clusters          */
  array_t *sizes,     /**< an empty, initialised array */
  double   range      /**< cluster size range          */
);

/**
 * Sets the label value and location (in 2 dimensions). Individual clusters
 * are arranged in a circle, then clusters themselves are arranged in a larger
 * circle. 
 */
static void _mk_label(
  graph_label_t *lbl,  /**< pointer to label                     */
  uint32_t       ncs,  /**< total number of clusters             */
  uint32_t       cidx, /**< current cluster index                */
  uint32_t       csz,  /**< number of nodes in current cluster   */
  uint32_t       ncidx /**< index of current node within cluster */
);

uint8_t graph_create_clustered_by_degree(
  graph_t *g,
  uint32_t nnodes,
  uint32_t nclusters,
  double   intdegree,
  double   extdegree,
  double   sizerange
) {
  double clustsz;
  double maxintra;
  double maxinter;
  double nintra;
  double ninter;
  double intdens;
  double extdens;

  clustsz  = round((double)nnodes / nclusters);
  maxintra = round(nclusters * (clustsz*(clustsz-1))/2.0);
  maxinter = round((clustsz*clustsz) * ((nclusters * (nclusters-1))/2.0));
  nintra   = round((intdegree*nnodes)/2);
  ninter   = round((extdegree*nnodes)/2);
  intdens  = nintra / maxintra;
  extdens  = ninter / maxinter;

  return graph_create_clustered(
    g, nnodes, nclusters, intdens, extdens, sizerange);
}

uint8_t graph_create_clustered_by_total(
  graph_t *g, 
  uint32_t nnodes,
  uint32_t nclusters,
  double   internal,
  double   total,
  double   sizerange 
) {

  double clustsz;
  double external;
  double maxintra;
  double maxinter;

  if (total < 0.0) return 1;
  if (total > 1.0) return 1;

  clustsz   = round((double)nnodes / nclusters);
  maxintra  = round(nclusters * (clustsz*(clustsz-1))/2.0);
  maxinter  = round((clustsz*clustsz) * ((nclusters * (nclusters-1))/2.0));
  external  = (total * nnodes * (nnodes-1))/2.0 - internal*maxintra;
  external /= maxinter;

  /*
   * the requested internal density alone
   * would result in a graph with a density
   * greater than the requested total
   * density
   */
  if (external < 0.0) return 1;

  return graph_create_clustered(
    g, nnodes, nclusters, internal, external, sizerange);
}


uint8_t graph_create_clustered(
  graph_t *g,
  uint32_t nnodes,
  uint32_t nclusters, 
  double   internal,
  double   external,
  double   sizerange
) {

  uint64_t      ni;     /* node i        */
  uint64_t      nj;     /* node j        */
  uint64_t      ci;     /* cluster i     */
  uint64_t      cj;     /* cluster j     */
  array_t       sizes;  /* cluster sizes */
  uint64_t      nci;    /* index within cluster of node i */
  graph_label_t lbl;
  uint32_t      sz;
  uint32_t      tmp;

  if (g         == NULL)   goto fail;
  if (nnodes    == 0)      goto fail;
  if (nclusters == 0)      goto fail;
  if (nclusters >  nnodes) goto fail;
  if (internal  <  0.0)    goto fail;
  if (internal  >  1.0)    goto fail;
  if (external  <  0.0)    goto fail;
  if (external  >  1.0)    goto fail;
  if (sizerange <  0.0)    goto fail;
  if (sizerange >  1.0)    goto fail;

  if (array_create(&sizes, sizeof(uint32_t), nclusters))   goto fail;
  if (_create_sizes(nnodes, nclusters, &sizes, sizerange)) goto fail;

  array_get(&sizes, sizes.size-1, &nnodes);
  
  if (graph_create(g, nnodes, 0)) goto fail;

  memset(&lbl, 0, sizeof(graph_label_t));

  ci  = 0;
  for (nci = 0, ni = 0; ni < nnodes; ni++, nci++) {

    array_get(&sizes, ci, &sz);
    if (ni == sz) {
      ci++;
      nci = 0;
    }

    if (ci == 0) {
      array_get(&sizes, 0, &sz);
    }
    else {
      array_get(&sizes, ci-1, &tmp);
      array_get(&sizes, ci,   &sz);
      sz -= tmp;
    }
    _mk_label(&lbl, sizes.size, ci, sz, nci);
    graph_set_nodelabel(g, ni, &lbl);

    cj = ci;
    for (nj = ni+1; nj < nnodes; nj++) {

      array_get(&sizes, cj, &sz);

      if (nj == sz) cj++;

      /*intra-cluster edges are added with 'internal' probability*/
      if (ci == cj) {
        
        if (((double)rand()/RAND_MAX) <= internal) {
          if (graph_add_edge(g, ni, nj, 1.0)) goto fail;
        }
      }
      
      /*inter-cluster edges are added with 'external' probability*/
      else {
        
        if (((double)rand()/RAND_MAX) <= external) {
          if (graph_add_edge(g, ni, nj, 1.0)) goto fail;
        }
      }
    }
  }

  array_free(&sizes);

  return 0;
fail:
  return 1;
}

uint8_t _create_sizes(
  uint32_t nnodes, uint32_t nclusters, array_t *sizes, double range) {

  uint64_t i;
  uint32_t sz;
  uint32_t minsz;
  uint32_t maxsz;
  uint32_t tally;
  double   clustersz;

  if (nnodes    == 0)      return 1;
  if (nclusters == 0)      return 1;
  if (nclusters >  nnodes) return 1;
  if (sizes     == NULL)   return 1;
  if (range     <  0.0)    return 1;
  if (range     >  1.0)    return 1;

  clustersz = (double)nnodes/nclusters;

  minsz = (uint32_t)(clustersz - range*clustersz);
  maxsz = (uint32_t)(clustersz + range*clustersz);

  if (minsz == 0)      return 1;
  if (minsz >= nnodes) return 1;
  if (maxsz == 0)      return 1;
  if (maxsz >= nnodes) return 1;

  tally = 0;
  for (i = 0; i < nclusters; i++) {
      
    sz = minsz + ((double)rand() / RAND_MAX) * (maxsz - minsz);

    tally += sz;
    array_append(sizes, &tally);
  }

  return 0;
}

static void _mk_label(
  graph_label_t *lbl,
  uint32_t ncs,
  uint32_t cidx,
  uint32_t csz,
  uint32_t ncidx) {

  double angle;
  double xoff;
  double yoff;

  angle = 2 * M_PI * cidx / ncs;
  xoff = 5 + 5*cos(angle);
  yoff = 5 + 5*sin(angle);

  angle = 2 * M_PI * ncidx / csz;
  
  lbl->labelval = cidx;
  lbl->xval     = xoff + 1 + cos(angle);
  lbl->yval     = yoff + 1 + sin(angle);
  lbl->zval     = 0;
}
