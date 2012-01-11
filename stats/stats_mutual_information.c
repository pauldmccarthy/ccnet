/**
 * Calculates the normalised mutual information of a graph, with respect to
 * the line-up between unique label values, and components in the graph. The
 * normalised mutual information may be used to gauge the success of a
 * community detection algorithm.
 *
 *   Danon L, Dutch J, Diaz-Guilera A, Arenas A. 2005. Comparing
 *   community structure identification. Journal of Statistical
 *   Mechanics: Theory and Experiment, vol. 2005, no. 9, pg. 09008.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Populates the 'confusion matrix', a 2D matrix where rows represesnt 'real'
 * communities (unique label values), and columns represent 'found'
 * communities (components). The value of each element N[i,j] "is simply the
 * number of nodes in the real community i that appear in the found community
 * j."
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mk_confusion_matrix(
  uint32_t *matrix, /**< flattened 2D matrix to store community counts */
  uint32_t  ncmps,  /**< number of components, or 'found' communities  */
  uint32_t  nlbls,  /**< number of label values, or 'real' communities */
  array_t  *groups  /**< array of communities in the graph             */
);

/**
 * Calculates the mutual information from the given matrix.
 *
 * \return the normalised mutual information, a value between 0.0 and 1.0, or
 * -1 on failure.
 */
static double _mutual_information(
  uint32_t *matrix, /**< flattened 2D confusion matrix                 */
  array_t  *cmpszs, /**< array containing sizes of 'found' communities */
  uint32_t  nlbls,  /**< number of 'real' communities                  */
  uint32_t  nnodes, /**< number of nodes in the graph                  */
  uint8_t   disco   /**< non-0: do not include disconnected nodes      */
);

double stats_mutual_information(graph_t *g, uint8_t disco) {

  array_t   groups;
  array_t   cmpszs;
  uint32_t  ncmps;
  uint32_t  nlbls;
  uint32_t  nnodes;
  uint32_t  connected;
  uint32_t *conf_matrix;
  double    mi;

  groups.data = NULL;
  cmpszs.data = NULL;
  conf_matrix = NULL;

  if (array_create(&cmpszs, sizeof(uint32_t), 10)) goto fail;
  
  ncmps     = stats_num_components( g, 1, &cmpszs, NULL);
  connected = stats_cache_connected(g);
  nlbls     = graph_num_labelvals(  g);
  nnodes    = graph_num_nodes(      g);

  conf_matrix = calloc(ncmps*nlbls, sizeof(uint32_t));
  if (conf_matrix == NULL) goto fail;

  if (array_create(&groups, sizeof(node_group_t), 10))          goto fail;
  if (graph_communities(g, 1, &groups))                         goto fail;
  if (_mk_confusion_matrix(conf_matrix, ncmps, nlbls, &groups)) goto fail;

  if (disco) nnodes = connected;
  mi = _mutual_information(conf_matrix, &cmpszs, nlbls, nnodes, disco);

  if (mi == -1) goto fail;

  free(conf_matrix);
  array_free(&groups);
  array_free(&cmpszs);
  return mi;
  
fail:
  if (cmpszs.data != NULL) array_free(&cmpszs);
  if (groups.data != NULL) array_free(&groups);
  if (conf_matrix != NULL) free(conf_matrix);
  return -1;
}

uint8_t _mk_confusion_matrix(
  uint32_t *matrix, uint32_t ncmps, uint32_t nlbls, array_t *groups) {

  uint64_t     i;
  uint32_t     midx;
  node_group_t g;

  for (i = 0; i < groups->size; i++) {

    if (array_get(groups, i, &g)) goto fail;

    midx = ncmps * (g.labelidx) + g.component;
    matrix[midx] += g.nnodes;
  }

  return 0;
fail:
  return 1;
}

double _mutual_information(
  uint32_t *matrix,
  array_t  *cmpszs,
  uint32_t  nlbls,
  uint32_t  nnodes,
  uint8_t   disco) {

  uint64_t i;
  uint64_t j;
  uint32_t off;
  uint32_t cmpsz;
  double  *rowsums;
  double  *colsums;
  double   numer;
  double   denom1;
  double   denom2;
  double   tmp;

  rowsums = NULL;
  colsums = NULL;

  rowsums = calloc(nlbls, sizeof(double));
  if (rowsums == NULL) goto fail;  
  colsums = calloc(cmpszs->size, sizeof(double));
  if (colsums == NULL) goto fail;

  /*tally up rows and columns*/
  for (i = 0; i < nlbls; i++) {
    
    off = i*(cmpszs->size);

    for (j = 0; j < cmpszs->size; j++) {

      if (array_get(cmpszs, j, &cmpsz)) goto fail;
      if (disco && (cmpsz == 1)) continue;
      
      rowsums[i] += matrix[off+j];
      colsums[j] += matrix[off+j];
    }
  }

  /*tally up the sections of the mutual info equation*/
  numer  = 0;
  denom1 = 0;
  denom2 = 0;

  /*numerator*/
  for (i = 0; i < nlbls; i++) {

    off = i*(cmpszs->size);

    for (j = 0; j < cmpszs->size; j++) {

      if (array_get(cmpszs, j, &cmpsz)) goto fail;
      if (disco && (cmpsz == 1)) continue;

      /*running log(0) will cause the result to be NaN*/
      if (matrix[off+j] == 0) continue;
      
      tmp = ((double)matrix[off+j] * nnodes) / (rowsums[i] * colsums[j]);
      numer += matrix[off+j] * log(tmp);
    }
  }

  /*first denominator*/
  for (i = 0; i < nlbls; i++) {
    
    if (rowsums[i] == 0) continue;
    denom1 += rowsums[i] * log(rowsums[i] / nnodes);
  }

  /*second denominator*/
  for (j = 0; j < cmpszs->size; j++) {
    
    if (array_get(cmpszs, j, &cmpsz)) goto fail;
    if (disco && (cmpsz == 1)) continue;

    if (colsums[j] == 0) continue;
    
    denom2 += colsums[j] * log(colsums[j] / nnodes);
  }

  free(rowsums);
  free(colsums);

  if (denom1 + denom2 == 0) return 0;
  else                      return (-2*numer) / (denom1 + denom2);
  
fail:
  if (rowsums != NULL) free(rowsums);
  if (colsums != NULL) free(colsums);
  return -1;
}
