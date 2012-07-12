/**
 * A function which generates an Erdos-Renyi random graph. Assumes that the
 * random number generator has been seeded. Edge weights are set to random
 * values between -1 and 1.
 *
 *   A. Renyi, P. Erdos, 1960. On the evolution of random
 *   graphs. Publications of the Mathematical Institute of 
 *   the Hungarian Academy of Sciences. 5:17-61.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"

static void _mk_label(graph_label_t *lbl);

uint8_t graph_create_er_random(
  graph_t *g, uint32_t nnodes, double density) {

  uint64_t      i;
  uint64_t      j;
  double        prob;
  double        wt;
  graph_label_t lbl;

  if (g      == NULL) goto fail;
  if (nnodes == 0)    goto fail;
  if (density > 1.0)  goto fail;
  if (density < 0.0)  goto fail;
  
  if (graph_create(g, nnodes, 0)) goto fail;

  for (i = 0; i < nnodes; i++) {
    _mk_label(&lbl);
    graph_set_nodelabel(g, i, &lbl);
    for (j = i+1; j < nnodes; j++) {

      prob = (double)rand() / RAND_MAX;

      if (prob <= density) {
        wt = -1.0 + 2.0*((double)rand() / RAND_MAX);
        if (graph_add_edge(g, i, j, wt)) goto fail;
      }
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
