/**
 * Function which calculates the fitness of a partitioning of a graph into
 * communities, using the fitness function described in:
 *
 * C Chira & A Gog & D Iclanzan 2012 Evolutionary detection of community
 * structures in complex network: A new fitness function
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

static void _count_degrees(
  graph_t  *g,
  uint32_t *communities,
  uint32_t *indegrees,
  uint32_t *outdegrees
);

static void _count_community_sizes(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *communities,
  uint32_t *communtiy_sizes
);

static void _count_community_edges(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *communities,
  uint32_t *indegrees,
  double   *community_edges
);

static void _calc_node_strengths(
  graph_t  *g,
  uint32_t *indegrees,
  uint32_t *outdegrees,
  uint32_t *communities,
  uint32_t *community_sizes,
  double   *node_strengths
);

static void _calc_community_strengths(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *communities,
  uint32_t *community_sizes,
  double   *node_strengths,
  double   *comm_strengths);

static double _calc_partition_strength(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *community_sizes,
  double   *community_edges,
  double   *community_strengths
);

double stats_chira(graph_t *g, uint32_t ncommunities, uint32_t *communities) {

  uint32_t  nnodes;
  uint32_t *indegrees;
  uint32_t *outdegrees;
  uint32_t *comm_sizes;
  double   *comm_edges;
  double   *node_strengths;
  double   *comm_strengths;
  double    fitness;

  indegrees      = NULL;
  outdegrees     = NULL;
  comm_sizes     = NULL;
  comm_edges     = NULL;
  node_strengths = NULL;
  comm_strengths = NULL;
  nnodes         = graph_num_nodes(g);

  indegrees  = calloc(nnodes,sizeof(uint32_t));
  if (indegrees == NULL) goto fail;
  outdegrees = calloc(nnodes,sizeof(uint32_t));
  if (outdegrees == NULL) goto fail;
  comm_sizes = calloc(ncommunities,sizeof(uint32_t));
  if (comm_sizes == NULL) goto fail;
  comm_edges = calloc(ncommunities,sizeof(double));
  if (comm_edges == NULL) goto fail;
  node_strengths = calloc(nnodes,sizeof(double));
  if (node_strengths == NULL) goto fail;
  comm_strengths = calloc(ncommunities,sizeof(double));
  if (comm_strengths == NULL) goto fail; 

  _count_degrees(        g, communities,  indegrees,   outdegrees);
  _count_community_sizes(g, ncommunities, communities, comm_sizes);
  _count_community_edges(g, ncommunities, communities, indegrees, comm_edges);

  uint32_t i;

  for (i = 0; i < ncommunities; i++) {
    printf("comm %u size: %02u  edges: %3.0f\n",
           i, comm_sizes[i], comm_edges[i]);
  }

  _calc_node_strengths(
    g, indegrees, outdegrees,
    communities, comm_sizes,
    node_strengths);


  for (i = 0; i < nnodes; i++) {
    printf(
      "node %03u  comm: %u  in: %02u  out: %02u  deg: %02u  str: %0.2f\n",
      i, communities[i], indegrees[i], outdegrees[i],
      graph_num_neighbours(g, i), node_strengths[i]);
  } 

  _calc_community_strengths(
    g, ncommunities,
    communities, comm_sizes,
    node_strengths, comm_strengths);

  fitness = _calc_partition_strength(
    g, ncommunities, comm_sizes, comm_edges, comm_strengths);

  stats_cache_add(g,
                  STATS_CACHE_CHIRA,
                  STATS_CACHE_TYPE_GRAPH,
                  sizeof(double));
  stats_cache_update(g, STATS_CACHE_CHIRA, 0, -1, &fitness); 

  free(indegrees);
  free(outdegrees);
  free(comm_sizes);
  free(comm_edges);
  free(node_strengths);
  free(comm_strengths);
  return fitness;
  
fail:
  if (indegrees      != NULL) free(indegrees);
  if (outdegrees     != NULL) free(outdegrees);
  if (comm_sizes     != NULL) free(comm_sizes);
  if (comm_edges     != NULL) free(comm_edges);
  if (node_strengths != NULL) free(node_strengths);
  if (comm_strengths != NULL) free(comm_strengths);
  return -1;
}


void _count_degrees(
  graph_t  *g,
  uint32_t *communities,
  uint32_t *indegrees,
  uint32_t *outdegrees) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;

  nnodes = graph_num_nodes(g);

  memset(indegrees,  0, nnodes*sizeof(uint32_t));
  memset(outdegrees, 0, nnodes*sizeof(uint32_t));

  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);

    for (j = 0; j < nnbrs; j++) {

      if (communities[i] == communities[nbrs[j]]) indegrees [i] ++;
      else                                        outdegrees[i] ++;
    }
  }
}


void _count_community_sizes(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *communities,
  uint32_t *community_sizes) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  memset(community_sizes, 0, ncommunities*sizeof(uint32_t));

  for (i = 0; i < nnodes; i++)
    community_sizes[communities[i]]++;
}


void _count_community_edges(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *communities,
  uint32_t *indegrees,
  double   *community_edges) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  memset(community_edges, 0, ncommunities*sizeof(uint32_t));

  for (i = 0; i < nnodes; i++) 
    community_edges[communities[i]] += indegrees[i] / 2.0; 
}

void _calc_node_strengths(
  graph_t  *g,
  uint32_t *indegrees,
  uint32_t *outdegrees,
  uint32_t *communities,
  uint32_t *community_sizes,
  double   *node_strengths) {

  uint64_t i;
  uint32_t nnodes;
  uint32_t comsz;

  nnodes = graph_num_nodes(g);

  memset(node_strengths, 0, nnodes*sizeof(double));

  for (i = 0; i < nnodes; i++) {

    comsz             = community_sizes[communities[i]];
    node_strengths[i] = (((float)indegrees[i] - outdegrees[i])) / comsz;
   }
}

void _calc_community_strengths(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *communities,
  uint32_t *community_sizes,
  double   *node_strengths,
  double   *comm_strengths) {
  
  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  double    istr;
  uint32_t  nnbrs;
  uint32_t *nbrs;

  nnodes = graph_num_nodes(g);

  memset(comm_strengths, 0, ncommunities*sizeof(double));

  for (i = 0; i < nnodes; i++) {

    istr  = node_strengths[i];
    nbrs  = graph_get_neighbours(g, i);
    nnbrs = graph_num_neighbours(g, i);

    for (j = 0; j < nnbrs; j++) {

      if (communities[i] != communities[nbrs[j]]) continue;
      
      istr += 0.5 * node_strengths[nbrs[j]];
    }

    comm_strengths[communities[i]] += istr;
  }
}

double _calc_partition_strength(
  graph_t  *g,
  uint32_t  ncommunities,
  uint32_t *community_sizes,
  double   *community_edges,
  double   *community_strengths) {

  uint64_t i;
  double   nedges;
  double   norm;
  double   partstr;

  partstr = 0;
  nedges  = graph_num_edges(g);

  for (i = 0; i < ncommunities; i++) {

    norm     = (community_edges[i] / nedges) / community_sizes[i];
    partstr += community_strengths[i] * norm;
  }

  partstr /= ncommunities;

  return partstr;
}
