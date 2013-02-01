/**
 * A function which generates a small world random graph. Assumes that the
 * random number generator has been seeded. Edge weights are set to random
 * values between -1 and 1.
 *
 *   D. J. Watts & S. H. Strogatz, 1998. Collective dynamics of
 *   `small-world' networks. Letters to Nature, Vol. 393, pp. 440-442.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"

static void _mk_label(graph_label_t *lbl);

uint8_t graph_create_smallworld(
  graph_t *g, uint32_t nnodes, double p, uint16_t k) {

  uint64_t      i;
  uint64_t      j;
  uint32_t      oldnbr;
  uint32_t      newnbr;
  uint32_t      nnbrs;
  uint32_t     *nbrs;
  double        wt;
  graph_label_t lbl;

  if (g      == NULL) goto fail;
  if (nnodes == 0)    goto fail;
  if (p      >  1.0)  goto fail;
  if (p      <  0.0)  goto fail;
  if (k      == 0)    goto fail;

  /* degree must be even, so round up if we've been given an odd value*/
  if (k % 2)
    k ++;
  
  if (graph_create(g, nnodes, 0)) goto fail;

  for (i = 0; i < nnodes; i++) {
    _mk_label(&lbl);
  }
  
  /* create a ring lattice */
  for (i = 0; i < nnodes; i++) {

    /* by adding an edge from the current node to the next (k/2) nodes */
    for (j = i+1; j < (i + 1 + k/2); j++) {

      wt = -1.0 + 2.0*((double)rand() / RAND_MAX);

      if (graph_add_edge(g, i, (j % nnodes), wt)) goto fail;
    }
  }

  /* randomly rewire all edges with probability p */
  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);

    for (j = 0; j < nnbrs; j++) {

      if (i < nbrs[j])
        continue;

      if (((double)rand() / RAND_MAX) > p)
        continue;

      wt = -1.0 + 2.0*((double)rand() / RAND_MAX);

      oldnbr = nbrs[j];

      /*remove old edge*/
      if (graph_remove_edge(g, i, oldnbr)) goto fail;


      /*choose a new neighbour*/
      while (1) {
        
        newnbr = floor(nnodes * ((double)rand() / RAND_MAX));

        if (newnbr == i)                        continue;
        if (newnbr == oldnbr)                   continue;
        if (graph_are_neighbours(g, i, newnbr)) continue;
        
        break;
      } 

      if (graph_add_edge(g, i, newnbr, wt)) goto fail;
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
