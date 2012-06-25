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
#include <string.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"


typedef struct _set {

  uint32_t lblval;
  array_t  idxs;

} set_t;

static uint8_t _partition(
  uint32_t  n,
  uint32_t *lbls,
  array_t  *sets
);

static uint32_t _intersection(
  set_t *setj,
  set_t *setk
);

static double _mutual_information(
  uint32_t  n,
  array_t  *sets1,
  array_t  *sets2
);


/**
 * \return the entropy of the given partitioning.
 */
static double _entropy(
  array_t *sets, /**< tne partitioning       */
  uint32_t n     /**< total number of values */
);


double smi(uint32_t n, uint32_t *lblsj, uint32_t *lblsk) {

  array_t  setsj;
  array_t  setsk;
  double   mi;
  double   nmi;
  double   entj;
  double   entk;

  memset(&setsj, 0, sizeof(array_t));
  memset(&setsk, 0, sizeof(array_t));

  if (_partition(n, lblsj, &setsj)) goto fail;
  if (_partition(n, lblsk, &setsk)) goto fail;

  mi   = _mutual_information(n, &setsj, &setsk);
  entj = _entropy(&setsj, n);
  entk = _entropy(&setsk, n);
  
  nmi  = mi / ((entj + entk)/2.0);

  return nmi;

fail:
  return -1;
}


double stats_mutual_information(graph_t *g, uint8_t disco) {

  uint64_t  i;
  uint32_t  nnodes;
  uint32_t *lblsj;
  uint32_t *lblsk;

  nnodes = graph_num_nodes(g);

  lblsj = calloc(nnodes, sizeof(uint32_t));
  lblsk = calloc(nnodes, sizeof(uint32_t));

  stats_num_components(g, 0, NULL, lblsj);

  for (i = 0; i < nnodes; i++) 
    lblsk[i] = graph_get_nodelabel(g, i)->labelval;

  return smi(nnodes, lblsj, lblsk);
}


uint8_t _partition(
  uint32_t  n,
  uint32_t *lbls,
  array_t  *sets) {

  uint32_t  i;
  uint32_t  j;
  set_t    *s;

  if (array_create(sets, sizeof(set_t), 10)) goto fail;

  for (i = 0; i < n; i++) {

    for (j = 0; j < sets->size; j++) {
      
      s = (set_t *)array_getd(sets, j);

      if (s->lblval == lbls[i]) {
        
        if (array_append(&(s->idxs), &i)) goto fail;
        break;
      }
    }

    if (j == sets->size) {
      
      s = calloc(sizeof(set_t), 1);
      if (s == NULL) goto fail;
      
      s->lblval = lbls[i];

      if (array_create(&(s->idxs), sizeof(uint32_t), 10)) goto fail;
      if (array_append(&(s->idxs), &i))                   goto fail;
      if (array_append(sets, s))                          goto fail;
    }
  }

  free(lbls);
  return 0;
  
fail:
  return 1;
}


uint32_t _intersection(set_t *setj, set_t *setk) {

  uint64_t j;
  uint64_t k;
  uint32_t idxj;
  uint32_t idxk;
  uint32_t count;

  count = 0;

  for (j = 0; j < setj->idxs.size; j++) {

    idxj = *(uint32_t *)array_getd(&(setj->idxs), j);

    for (k = 0; k < setk->idxs.size; k++) {

      idxk = *(uint32_t *)array_getd(&(setk->idxs), k);

      if (idxj == idxk) {
        count++;
        break;
      }
    }
  }

  return count;
}


double _mutual_information(uint32_t n, array_t *setsj, array_t *setsk) {

  uint64_t j;
  uint64_t k;
  uint32_t intcount;
  double   jkval;
  double   mi;
  set_t   *setj;
  set_t   *setk;

  mi = 0;

  for (j = 0; j < setsj->size; j++) {

    setj = (set_t *)array_getd(setsj, j);

    for (k = 0; k < setsk->size; k++) {

      setk      = (set_t *)array_getd(setsk, k);
      intcount  = _intersection(setj, setk);
      jkval     = n*((double)intcount);
      jkval    /= (setj->idxs.size)*(setk->idxs.size);
      jkval     = log2(jkval);
      jkval    *= ((double)intcount) / n;

      if (!isfinite(jkval))
        mi += jkval;
    }
  }
  
  return mi;
}


double _entropy(array_t *sets, uint32_t n) {

  uint64_t i;
  double   ent;
  double   enti;
  set_t   *seti;
  ent = 0;

  for (i = 0; i < sets->size; i++) {

    seti  = (set_t *)array_getd(sets, i);

    enti  = ((double)seti->idxs.size) / n;
    enti *= log2(enti);
    ent  += enti;
  }

  return -ent;
}
