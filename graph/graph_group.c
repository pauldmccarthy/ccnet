/**
 * Functions which create a group, or partition of nodes.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <string.h>

#include "graph/graph.h"
#include "util/array.h"
#include "util/compare.h"

/**
 * Adds the specified node to the appropriate partition,
 * according to its label value.
 */
static uint8_t _add_node_to_ptn(
  graph_t          *g,  /**< the graph              */
  uint32_t          n,  /**< node id                */
  node_partition_t *ptn /**< the current partitions */
);

uint8_t graph_group_by_label(graph_t *g, node_partition_t *ptn ) {

  uint64_t i;
  uint32_t nnodes;

  memset(ptn, 0, sizeof(node_partition_t));

  nnodes = graph_num_nodes(g);

  ptn->partids =  calloc(1, sizeof(array_t));
  if (ptn->partids == NULL) goto fail;
  
  ptn->parts =  calloc(1, sizeof(array_t));
  if (ptn->parts == NULL) goto fail;

  if (array_create(ptn->partids, sizeof(uint32_t), 10)) goto fail;
  if (array_create(ptn->parts,   sizeof(array_t),  10)) goto fail;

  array_set_cmps(ptn->partids, compare_u32, compare_u32_insert);

  ptn->nparts = 0;
  ptn->nnodes = nnodes;

  for (i = 0; i < nnodes; i++) {

    if (_add_node_to_ptn(g, i, ptn)) goto fail;
  }
  
  ptn->nparts = ptn->parts->size;

  return 0;

fail:
  return 1;
}

uint8_t _add_node_to_ptn(graph_t *g, uint32_t n, node_partition_t *ptn) {

  int64_t        idx;
  uint32_t       uidx;
  array_t        nodeids;
  array_t       *pnodeids;
  graph_label_t *lbl;

  lbl  = graph_get_nodelabel(g, n);
  idx  = array_find(ptn->partids, &(lbl->labelval), 1);
  uidx = idx;

  if (idx < 0) {

    if (array_insert_sorted(ptn->partids, &(lbl->labelval), 1, &uidx) > 1)
      goto fail;
    
    if (array_create(&nodeids, sizeof(uint32_t), 100)) goto fail;
    if (array_insert(ptn->parts, uidx, &nodeids))      goto fail;
  }
  
  pnodeids = array_getd(ptn->parts, uidx);
  if (array_append(pnodeids, &n)) goto fail;

  return 0;

fail:
  return 1;
}
