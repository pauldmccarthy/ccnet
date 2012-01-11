/**
 * Calculates the 'Newman error' of a graph, an indication of the success of a
 * classification algorithm upon the graph. The function assumes that
 * same-labelled nodes are part of the same community, and compares the
 * components which exist in the graph to groups of same-labelled nodes.
 *
 *     M. E. J. Newman 2004. Fast algorithm for
 *     detecting community structure in networks.
 *     Physical Review E, vol. 69, no. 6, pg. 066133.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "stats/stats.h"

/**
 * Compares two node_group_t structs by the component value.
 */
static int _group_component_cmp(const void *g1, const void *g2);

/**
 * For each unique label value in the graph, Finds the largest group in the
 * given array of node_group_t structs. Removes all other groups from the
 * array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_max_groups(
  graph_t *g,        /**< the graph                                        */
  array_t *allgroups,/**< array containing node_group_t structs            */
  array_t *maxgroups /**< pointer to initialised array to store max groups */
);

double stats_newman_error(graph_t *g) {

  uint64_t     i;
  uint32_t     nnodes;
  uint32_t     ncorrect;
  array_t      allgroups;
  array_t      maxgroups;
  node_group_t group;

  allgroups.data = NULL;
  maxgroups.data = NULL;
  ncorrect       = 0;
  nnodes         = graph_num_nodes(g);

  if (array_create(&allgroups, sizeof(node_group_t), 10)) goto fail;
  if (array_create(&maxgroups, sizeof(node_group_t), 10)) goto fail;
  if (graph_communities(g, 1, &allgroups))                goto fail;
  if (_find_max_groups( g, &allgroups, &maxgroups))       goto fail;

  array_set_cmps(&maxgroups, _group_component_cmp, NULL);

  for (i = 0; i < maxgroups.size; i++) {

    array_get(&maxgroups, i, &group);
    if (array_count(&maxgroups, &group) == 1)
      ncorrect += group.nnodes;
  }

  array_free(&maxgroups);
  array_free(&allgroups);
  
  return (double)ncorrect/nnodes;
  
fail:
  if (allgroups.data != NULL) array_free(&allgroups);
  if (maxgroups.data != NULL) array_free(&maxgroups);
  return -1;
}

static uint8_t _find_max_groups(
  graph_t *g, array_t *allgroups, array_t *maxgroups) {

  uint64_t     i;
  node_group_t grp1;
  node_group_t grp2;
  uint32_t     nlblvals;
  uint32_t    *lblvals;
  int64_t     *maxidx;

  maxidx      = NULL;
  nlblvals    = graph_num_labelvals(g);
  lblvals     = graph_get_labelvals(g);
  maxidx      = malloc(nlblvals*sizeof(int64_t));
  if (maxidx == NULL) goto fail;
  
  for (i = 0; i < nlblvals; i++) maxidx[i] = -1;

  /*find the largest group for each label value*/
  for (i = 0; i < allgroups->size; i++) {
    
    if (array_get(allgroups, i, &grp1)) goto fail;

    if (maxidx[grp1.labelidx] == -1) maxidx[grp1.labelidx] = i;
    else {
      if (array_get(allgroups, maxidx[grp1.labelidx], &grp2)) goto fail;
      if (grp1.nnodes > grp2.nnodes) maxidx[grp1.labelidx] = i;
    }
  }

  /*add the largest groups to the maxgroups array*/
  for (i = 0; i < nlblvals; i++) {

    if (maxidx[i] == -1) continue;
    
    if (array_get(allgroups, maxidx[i], &grp1)) goto fail;
    if (array_append(maxgroups, &grp1))         goto fail;
  }

  free(maxidx);
  return 0;
  
fail:
  if (maxidx != NULL) free(maxidx);
  return 1;
}

static int _group_component_cmp(const void *g1, const void *g2) {

  node_group_t *ng1;
  node_group_t *ng2;

  ng1 = (node_group_t *)g1;
  ng2 = (node_group_t *)g2;

  if      (ng1->component > ng2->component) return 1;
  else if (ng1->component < ng2->component) return -1;

  return 0;
}
