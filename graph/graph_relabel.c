/**
 * Function which relabels a graph using corresponding values from an
 * ANALYZE75 image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "util/array.h"
#include "util/compare.h"
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
      dims[1] = (uint32_t)(round(lbl.yval / yl));
      dims[2] = (uint32_t)(round(lbl.zval / zl));
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


typedef struct _mapping_t {

  uint32_t oldlbl;
  uint32_t newlbl;

} mapping_t;


static int _cmp_mapping(const void *a, const void *b) {

  mapping_t *ma;
  mapping_t *mb;

  ma = (mapping_t *)a;
  mb = (mapping_t *)b;

  return compare_u32(&(ma->oldlbl), &(mb->oldlbl));
}


static uint8_t _load_mapfile(char *mapfile,  array_t *map) {

  FILE     *f;
  char     *line;
  size_t    len;
  uint32_t  u;
  uint32_t  v;
  mapping_t mapping;

  line = NULL;
  f    = NULL;

  if (array_create(map, sizeof(mapping_t), 500)) {
    map = NULL;
    goto fail;
  }

  array_set_cmps(map, &_cmp_mapping, NULL);

  line = malloc(64);
  if (line == NULL) goto fail;

  f = fopen(mapfile, "rt");
  if (f == NULL) goto fail;

  while (getline(&line, &len, f) != -1) {

    sscanf(line, "%u %u", &u, &v);

    mapping.oldlbl = u;
    mapping.newlbl = v;

    if (array_insert_sorted(map, &mapping, 1, NULL) == 2)
      goto fail;
  }

  fclose(f);
  free(line);
  return 0;

fail:
  if (line != NULL) free(line);
  if (map  != NULL) array_free(map);
  if (f    != NULL) fclose(f);
  return 1;  
}


static uint8_t _relabel_node(graph_t *g, uint32_t nidx, array_t *map) {

  int64_t        mapidx;
  graph_label_t  lbl;
  graph_label_t *plbl;
  mapping_t      mapping;

  plbl = graph_get_nodelabel(g, nidx);
  
  if (plbl == NULL) goto fail;

  memcpy(&lbl, plbl, sizeof(graph_label_t));

  /*
   * if a mapping does not exist for this node's
   * label, don't fail, just leave it as is
   */
  mapping.oldlbl = lbl.labelval;
  mapping.newlbl = lbl.labelval;

  mapidx = array_find(map, &mapping, 1);

  if (mapidx >= 0) {
    if (array_get(map, (uint32_t)mapidx, &mapping))
      goto fail;
  }

  lbl.labelval = mapping.newlbl;

  if (graph_set_nodelabel(g, nidx, &lbl))
    goto fail;

  return 0;

fail:
  return 1;
}


uint8_t graph_relabel_map(graph_t *g, char *lblmapfile) {

  uint64_t  i;
  uint32_t  nnodes;
  array_t   map;

  nnodes = graph_num_nodes(g);

  memset(&map, 0, sizeof(array_t));

  if (_load_mapfile(lblmapfile, &map))
    goto fail;

  for (i = 0; i < nnodes; i++)  {
    if (_relabel_node(g, i, &map))
      goto fail;
  }

  array_free(&map);
  return 0;

fail:
  return 1;
}

