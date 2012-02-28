/**
 * Create a graph from an ANALYZE75 image file, according to the greyscale
 * creation procedure introduced by Shi and Malik in their presentation of the
 * Normalized Cut algorithm:
 *
 * Jianbo Shi and Jitendra Malik 2000. Normalized Cuts and Image Segmentation.
 * IEEE Transactions on Pattern Analysis and Machine Intelligence, vol. 22,
 * no. 8, pp. 888-905.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "io/analyze75.h"

/**
 * Gives each node in the graph a label corresponding to its pixel
 * location and value in the image.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _label(
  graph_t *g,   /**< the graph    */
  dsr_t   *hdr, /**< image header */
  uint8_t *img  /**< image data   */
);

/**
 * Adds weighted edges to the graph, based on distance between nodes, and
 * similarity of their pixel value.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _connect(
  graph_t *g,    /**< the graph                                 */
  double   si,   /**< intensity sigma, strength of edge weight
                      decreases exponentially as the intensity
                      difference increases up to this value     */
  double   sx,   /**< distance sigma - strength of edge weight
                      decreases exponentially as the distance
                      increases up to this value                */
  double   rad,  /**< do not add edges between nodes which
                      are further away than this radius         */
  double   thres /**< do not add edges with a weight
                      below this threshold                      */
);

/**
 * Calculate the weight of the edge between the two specified nodes.
 *
 * \return the weight of the edge between the specified nodes.
 */
static double _edge_weight(
  graph_t *g,     /**< the graph             */
  double   si,    /**< intensity sigma       */
  double   sx,    /**< distance sigma        */
  double   rad,   /**< distance threshold    */
  double   thres, /**< edge weight threshold */
  uint32_t i,     /**< index of first node   */
  uint32_t j      /**< index of second node  */
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

  if (wt < thres) return 0.0;
  else            return wt;
}

