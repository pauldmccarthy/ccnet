/**
 * A function which generates a scale free random graph. Assumes that the
 * random number generator has been seeded. Edge weights are set to random
 * values between -1 and 1.
 *
 *   A.L. Barabasi and R. Albert, 1999. Emergence of Scaling
 *   in Random Networks. Science, Vol. 286, pp. 509-512.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "graph/graph.h"

static void _mk_label(graph_label_t *lbl);

uint8_t graph_create_scalefree(
  graph_t *g, uint32_t nnodes, uint16_t m, uint16_t m0) {

  uint64_t      i;
  uint64_t      j;
  uint32_t      n;
  double        prob;
  uint64_t      tot_deg;
  double        wt;
  graph_label_t lbl;

  if (g      == NULL) goto fail;
  if (nnodes == 0)    goto fail;
  if (m      == 0)    goto fail;
  if (m0     == 0)    goto fail;
  if (m      >  m0)   goto fail;
  
  if (graph_create(g, nnodes, 0)) goto fail;

  for (i = 0; i < nnodes; i++) {
    _mk_label(&lbl);
    graph_set_nodelabel(g, i, &lbl);
  }

  /*fully connect the first m0 nodes*/
  tot_deg = (m0*m0) - m0;

  for (i = 0; i < m0; i++) {

    for (j = i+1; j < m0; j++) {
      
      wt = -1.0 + 2.0*((double)rand() / RAND_MAX); 
      if (graph_add_edge(g, i, j, wt)) goto fail;
    }
  }

  /*connect the rest of the nodes with preferential attachment*/
  for (i = m0; i < nnodes; i++) {

    j = 0;

    while (j < m) {

      n    = floor(i * ((double)rand() / RAND_MAX));
      prob = (double)graph_num_neighbours(g, n) / tot_deg;

      if (((double)rand() / RAND_MAX) > prob)
        continue;

      wt = -1.0 + 2.0*((double)rand() / RAND_MAX);
      
      if (graph_add_edge(g, i, n, wt))
        goto fail;

      j       += 1;
      tot_deg += 2;
    }
  }
  

  return 0;
fail:
  graph_free(g);
  return 1;
}

static void _mk_label(graph_label_t *lbl) {

  lbl->labelval = 0;
  lbl->xval     = 5*((double)rand()/RAND_MAX);
  lbl->yval     = 5*((double)rand()/RAND_MAX);
  lbl->zval     = 0;
}
