/**
 * Function which builds a list of the 'communities' present in a graph.
 *
 * A community is a group of nodes with the same label value, which are in the
 * same component.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

/**
 * Compares two node_group_t structs first by the component number, and then
 * by the label value.
 */
static int _group_cmp(const void *g1, const void *g2);

/**
 * Compares two node)group_t structs by the number of nodes.
 */
static int _group_size_cmp(const void *g1, const void *g2);

uint8_t graph_communities(graph_t *g, uint32_t sz, array_t *groups) {
  
  uint64_t       i;
  uint64_t       j;
  int64_t        gidx;
  uint32_t       nnodes;
  uint32_t      *lblvals;
  uint32_t       nlblvals;
  node_group_t   group;
  uint32_t       tmp;
  
  nnodes     = graph_num_nodes(g);
  lblvals    = graph_get_labelvals(g);
  nlblvals   = graph_num_labelvals(g);

  array_set_cmps(groups, _group_cmp, NULL);

  for (i = 0; i < nnodes; i++) {
    
    stats_cache_node_component(g, i, &tmp);
    group.labelval  = graph_get_nodelabel(g, i)->labelval;
    group.component = tmp;

    gidx = array_find(groups, &group, 1);
    if (gidx == -1) {

      //lookup labelval index
      for (j = 0; j < nlblvals; j++) {
        if (lblvals[j] == group.labelval) break;
      }
      
      group.labelidx = j;
      group.nnodes   = 1;
      if (array_insert_sorted(groups, &group, 1, NULL) == 2)
        goto fail;
    }
    else {
      
      if (array_get(groups, gidx, &group)) goto fail;
      group.nnodes ++;
      array_set(groups, gidx, &group);
    }
  }

  array_set_cmps(groups, _group_size_cmp, NULL);
  array_sort(groups);

  /*remove any groups which are below the specified minimum size*/
  for (i = 1; i < sz; i++) {
    
    group.nnodes = i;
    while (array_remove_by_val(groups, &group, 1) >= 0);
  }

  return 0;
  
fail:
  return 1;
}

int _group_cmp(const void *g1, const void *g2) {

  node_group_t *ng1;
  node_group_t *ng2;

  ng1 = (node_group_t *)g1;
  ng2 = (node_group_t *)g2;

  if      (ng1->component > ng2->component) return 1;
  else if (ng1->component < ng2->component) return -1;

  if      (ng1->labelval > ng2->labelval) return 1;
  else if (ng1->labelval < ng2->labelval) return -1;

  return 0;
}

int _group_size_cmp(const void *g1, const void *g2) {

  node_group_t *ng1;
  node_group_t *ng2;

  ng1 = (node_group_t *)g1;
  ng2 = (node_group_t *)g2;

  if      (ng1->nnodes > ng2->nnodes) return 1;
  else if (ng1->nnodes < ng2->nnodes) return -1;

  return 0;
}
