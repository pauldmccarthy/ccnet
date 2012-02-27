/**
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "io/analyze75.h"

static uint8_t _label(
  graph_t *g,
  dsr_t   *hdr,
  uint8_t *img
);

static uint8_t _connect(
  graph_t *g,
  double   si,
  double   sx,
  double   rad,
  double   thres
);

static double _edge_weight(
  graph_t *g,
  double   si,
  double   sx,
  double   rad,
  double   thres,
  uint32_t i,
  uint32_t j
);

uint8_t graph_create_ncut(
  graph_t *g,
  dsr_t   *hdr,
  uint8_t *img,
  double   si,
  double   sx,
  double   rad,
  double   thres) {

  uint32_t nnodes;

  nnodes = analyze_num_vals(hdr);

  if (graph_create(g, nnodes, 0))      goto fail;
  if (_label(g, hdr, img))             goto fail;
  if (_connect(g, si, sx, rad, thres)) goto fail;

  return 0;

fail:
  return 1;
}

uint8_t _label(graph_t *g, dsr_t *hdr, uint8_t *img) {

  uint64_t      i;
  uint32_t      nnodes;
  double        val;  
  graph_label_t lbl;
  uint32_t      dims[3];

  memset(dims, 0, sizeof(dims));

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    analyze_get_indices(   hdr, i,   dims);
    val = analyze_read_val(hdr, img, dims);

    lbl.labelval = val;
    lbl.xval     = dims[0];
    lbl.yval     = dims[1];
    lbl.zval     = 0.0;

    if (graph_set_nodelabel(g, i, &lbl)) goto fail;
  }

  return 0;
  
fail:
  return 1;
}

uint8_t _connect(graph_t *g, double si, double sx, double rad, double thres) {

  uint64_t i;
  uint64_t j;
  double   wt;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    
    for (j = i+1; j < nnodes; j++) {

      wt = _edge_weight(g, si, sx, rad, thres, i, j);

      if (wt == 0.0)                   continue;
      if (graph_add_edge(g, i, j, wt)) goto fail;
    }
  }

  return 0;
  
fail:
  return 1;
}

double _edge_weight(
  graph_t *g,
  double   si,
  double   sx, 
  double   rad,
  double   thres,
  uint32_t i,
  uint32_t j) {

  double         wt;
  double         dx;
  double         df;
  graph_label_t *li;
  graph_label_t *lj;

  li = graph_get_nodelabel(g, i);
  lj = graph_get_nodelabel(g, j);

  df = (double)li->labelval - lj->labelval;
  dx = stats_edge_distance(g, i, j);

  if (dx > rad) return 0.0;
  
  wt = exp(-(df*df)/(si*si)) * exp(-(dx*dx)/(sx*sx));

  printf("%02u -- %02u: %0.4f (df %0.4f si %0.4f dx %0.4f sx %0.4f\n",
          i, j, wt, df, si, dx, sx);

  if (wt < thres) return 0.0;
  else            return wt;
}

