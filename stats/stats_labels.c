/**
 * Provides node label related statistics.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>

#include "stats/stats.h"
#include "graph/graph.h"

uint32_t stats_num_labelled_nodes(graph_t *g, uint32_t labelval) {

  uint32_t       nnodes;
  uint32_t       count;
  uint64_t       i;
  graph_label_t *label;

  count  = 0;
  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    
    label = graph_get_nodelabel(g, i);

    if (label->labelval == labelval)
      count++;
  }

  return count;
}
