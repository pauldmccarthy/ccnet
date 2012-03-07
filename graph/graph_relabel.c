/**
 * Function which relabels a graph using corresponding values from an
 * ANALYZE75 image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "io/analyze75.h"
#include "graph/graph.h"

uint8_t graph_relabel(graph_t *g, dsr_t *hdr, uint8_t *img, uint8_t real) {

  graph_label_t  lbl;
  graph_label_t *plbl;
  uint32_t       nnodes;
  uint64_t       i;
  double         xl;
  double         yl;
  double         zl;
  uint32_t       dims[3];

  nnodes = graph_num_nodes(g);
  xl     = analyze_pixdim_size(hdr, 0);
  yl     = analyze_pixdim_size(hdr, 1);
  zl     = analyze_pixdim_size(hdr, 2);

  if (analyze_num_dims(hdr) != 3) goto fail;

  for (i = 0; i < nnodes; i++) {

    plbl = graph_get_nodelabel(g, i);
    if (plbl == NULL) goto fail;

    memcpy(&lbl, plbl, sizeof(graph_label_t));

    if (real) {
      dims[0] = (uint32_t)(round(lbl.xval / xl));
      dims[1] = (uint32_t)(round(lbl.xval / yl));
      dims[2] = (uint32_t)(round(lbl.xval / zl));
    }
    else {
      dims[0] = (uint32_t)lbl.xval;
      dims[1] = (uint32_t)lbl.yval;
      dims[2] = (uint32_t)lbl.zval;
    }

    if (dims[0] >= analyze_dim_size(hdr, 0)) goto fail;
    if (dims[1] >= analyze_dim_size(hdr, 1)) goto fail;
    if (dims[2] >= analyze_dim_size(hdr, 2)) goto fail;

    lbl.labelval = analyze_read_val(hdr, img, dims);

    if (graph_set_nodelabel(g, i, &lbl)) goto fail;
  }

  return 0;

fail:
  return 1;
}
