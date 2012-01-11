/**
 * Function to retrieve a component of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "util/array.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

uint8_t graph_get_component(graph_t *g, uint32_t cmp, array_t *nodeids) {

  uint64_t  i;
  uint32_t  nnodes;
  uint32_t *cmpnums;

  cmpnums = NULL;
  nnodes  = graph_num_nodes(g);

  cmpnums = calloc(nnodes, sizeof(uint32_t));
  if (cmpnums == NULL) goto fail;

  if (stats_cache_node_component(g, -1, cmpnums)) goto fail;

  for (i = 0; i < nnodes; i++) {

    if (cmpnums[i] != cmp) continue;

    if (array_append(nodeids, &i)) goto fail;
  }

  free(cmpnums);
  return 0;

fail:
  if (cmpnums != NULL) free(cmpnums);
  return -1;
}
