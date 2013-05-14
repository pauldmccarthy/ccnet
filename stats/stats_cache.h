/**
 * Provides a primitive sort of memoisation for statistics which take
 * time to calculate. The following types of statistic are available:
 *
 * - Graph-level: statistics which are calculated over an entire graph; one
 *   double value is stored for each graph-level statistic. Assortativity is
 *   an example of a graph-level statistic.
 *
 *   List-level: multi-valued statistics calculated over an entire graph.
 *
 * - Node-level: statistics which are calculated for every node. Local
 *   efficiency is an example of a node-level statistic.
 *
 * - Pair-level: statistics which are calculated for every pair of nodes.
 *   One double value for each pair of nodes is stored, giving a total of
 *   (numnodes*numnodes) values. Number of shortest paths is an example of a
 *   pair-level statistic.
 *
 * - Edge-level: statistics which are calculated for every edge in the graph.
 *   One double value is stored for each edge. Edge-betweenness is an example
 *   of an edge-level statistic.
 *
 * Graph, node, list and edge-level statistics are kept in memory; pair-level
 * statistics take up too much space, so are stored offline in temporary
 * files, one per statistic.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __STATS_CACHE_H__
#define __STATS_CACHE_H__

#include <stdio.h>
#include <stdint.h>

#include "graph/graph.h"
#include "util/array.h"
#include "util/edge_array.h"

/**
 * Cache field identifiers.
 */
enum {
  /*graph-level statistics*/
  STATS_CACHE_APPROX_CLUSTERING = 1,
  STATS_CACHE_GRAPH_CLUSTERING,
  STATS_CACHE_GRAPH_PATHLENGTH,
  STATS_CACHE_ASSORTATIVITY,
  STATS_CACHE_NUM_COMPONENTS,
  STATS_CACHE_LARGEST_COMPONENT,
  STATS_CACHE_CONNECTED,
  STATS_CACHE_GLOBAL_EFFICIENCY,
  STATS_CACHE_LOCAL_EFFICIENCY,
  STATS_CACHE_MODULARITY,
  STATS_CACHE_INTRA_EDGES,
  STATS_CACHE_INTER_EDGES,
  STATS_CACHE_MAX_DEGREE,
  STATS_CACHE_CHIRA,

  /*list statistics*/

  /*node-level statistics*/
  STATS_CACHE_NODE_CLUSTERING,
  STATS_CACHE_NODE_PATHLENGTH,
  STATS_CACHE_NODE_LOCAL_EFFICIENCY,
  STATS_CACHE_BETWEENNESS_CENTRALITY,
  STATS_CACHE_NODE_NUMPATHS,
  STATS_CACHE_NODE_COMPONENT,
  STATS_CACHE_NODE_EDGEDIST,

  /*pair-level statistics*/
  STATS_CACHE_PAIR_PATHLENGTH,
  STATS_CACHE_PAIR_NUMPATHS,
  
  /*edge-level statistics*/
  STATS_CACHE_EDGE_PATHSHARING,
  STATS_CACHE_EDGE_BETWEENNESS
};

/**
 * Cache field types.
 */
typedef enum {
  
  STATS_CACHE_TYPE_GRAPH,
  STATS_CACHE_TYPE_LIST,
  STATS_CACHE_TYPE_NODE,
  STATS_CACHE_TYPE_PAIR,
  STATS_CACHE_TYPE_EDGE
  
} cache_type_t;

typedef struct _graph_cache {

  uint8_t cached; /**< Whether a value has been cached. */
  void   *data;   /**< pointer to a single value        */
  
} graph_cache_t;

typedef struct _list_cache {

  array_t data; /**< pointer to an array of data */ 
  
} list_cache_t;

typedef struct _node_cache {

  uint8_t *cached; /**< Per node mask, whether values
                        for that node are in the cache                  */
  array_t  data;   /**< array of values one for every node in the graph */
  
} node_cache_t;

typedef struct _edge_cache {

  uint8_t      *cached; /**< Per-node mask, whether values
                             for that node are in the cache     */
  edge_array_t  data;   /**< Values for every edge in the graph */

} edge_cache_t;

/**
 * Struct used for pair and edge cache fields.
 */
typedef struct _file_cache {

  uint8_t *cached;    /**< per-node mask, whether values
                           for that node are in the cache        */
  FILE    *cachefile; /**< temp file containing the cache values */

} file_cache_t;

/**
 * Struct created for each field that is added to the cache.
 */
typedef struct _cache_entry {

  uint16_t     id;    /**< globally unique field ID    */
  cache_type_t type;  /**< field type                  */
  uint16_t     size;  /**< size of one value           */
  void        *cache; /**< pointer to the cache struct */

} cache_entry_t;

/**
 * This struct is added to the graph_t context
 */
typedef struct _stats_cache {

  graph_t *g;             /**< the graph being cached             */
  array_t  cache_entries; /**< array of cache_entry_t structs     */

} stats_cache_t;

/**
 * Attach a cache to the given graph. A call to graph_free (see graph_free.h)
 * will free the memory used by the cache.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t stats_cache_init(
  graph_t *g /**< the graph which needs caching */
);

/**
 * Resets the cache on the given graph, discarding all cached values. This
 * function should be used after a graph has been modified, to discard
 * obsolete cache values.
 *
 * In particular, if graph edges have beeen added or removed, the cache memory
 * space will no longer be valid, so resetting the cache is necessary in such
 * a situation.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t stats_cache_reset(
  graph_t *g /**< the graph to reset */
);

/**
 * Adds a field to the cache. You will still need to populate the cache with
 * a value for the field, via stats_cache_update. 
 *
 * \return 0 on success, or if the cache already contains the given field,
 * non-0 on failure.
 */
uint8_t stats_cache_add(
  graph_t     *g,    /**< the graph                         */
  uint16_t     id,   /**< globally unique ID for this field */
  cache_type_t type, /**< the field type                    */
  uint16_t     size  /**< size of one value for this field  */
);

/**
 * Checks the cache for the given field. If the cache contains the field, and
 * the given value pointer is not NULL, the the cached value is copied into
 * the value pointer. If the field is pair-level, the value pointer must
 * contain enough space for (graph_num_nodes(g)) values; if the field is
 * edge-level, it must contain enough space for (graph_num_neighbours(g,u))
 * values; otherwise, it need only contain enough space for one value.
 *
 * For edge and pair-level fields, if you only want to retrieve one value for
 * a specific edge/pair, use the 'v' parameter to specify the second node;
 * otherwise, pass in a negative value for 'v'.
 *
 * \return 1 if the cache contains a value for the given field, 0 if it
 * doesn't, -1 if an error occurs.
 */
int8_t stats_cache_check(
  graph_t  *g,   /**< the graph                                   */
  uint16_t  id,  /**< the field to check                          */
  uint32_t  u,   /**< node index - ignored for global fields      */
  int64_t   v,   /**< node index - only used for edge/pair fields */
  void     *data /**< optional place to store cached value(s)     */
);

/**
 * Updates the cache so that the given field has the given value(s). For
 * graph-or node-level fields, the data pointer need only contain one value;
 * for pair-level fields, it must contain (graph_num_nodes(g)) values, and
 * for edge-level fields it must contain (graph_num_neighbours(g,u)) valurs..
 *
 * For edge and pair-level fields, if you only want to set one value for a
 * specific edge/pair, use the 'v' parameter to specify the second node;
 * otherwise, pass in a negative value for 'v'.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t stats_cache_update(
  graph_t  *g,   /**< the graph                                   */
  uint16_t  id,  /**< the field to update                         */
  uint32_t  u,   /**< node index - ignored for global fields      */
  int64_t   v,   /**< node index - only used for edge/pair fields */
  void     *data /**< pointer to the new value(s)                 */
);

/**
 * Wrapper functions for each type of data which may be in the cache.  These
 * functions are implemented in stats_cache_wrapper.c. Use these functions if
 * you are happy using the cached version of the data. If you want to force
 * recalculation, use the respective statistical function directly (this will
 * cause the cached version to be updated).
 */

double stats_cache_approx_clustering(graph_t *g, uint32_t ntriples);
double stats_cache_graph_clustering( graph_t *g);
double stats_cache_graph_pathlength( graph_t *g);
double stats_cache_assortativity(    graph_t *g);
double stats_cache_num_components(   graph_t *g);
double stats_cache_largest_component(graph_t *g);
double stats_cache_connected(        graph_t *g);
double stats_cache_global_efficiency(graph_t *g);
double stats_cache_local_efficiency( graph_t *g);
double stats_cache_modularity(       graph_t *g);
double stats_cache_intra_edges(      graph_t *g);
double stats_cache_inter_edges(      graph_t *g);
double stats_cache_max_degree(       graph_t *g);
double stats_cache_chira(            graph_t *g);

uint8_t stats_cache_node_clustering(
  graph_t *g, int64_t n, double *data);
uint8_t stats_cache_node_pathlength(
  graph_t *g, int64_t n, double *data);
uint8_t stats_cache_node_local_efficiency(
  graph_t *g, int64_t n, double *data);
uint8_t stats_cache_betweenness_centrality(
  graph_t *g, int64_t n, double *data);
uint8_t stats_cache_node_numpaths(
  graph_t *g, int64_t n, double *data);
uint8_t stats_cache_node_component(
  graph_t *g, int64_t n, uint32_t *data);
uint8_t stats_cache_node_edgedist(
  graph_t *g, int64_t n, double *data);

uint8_t stats_cache_pair_pathlength(graph_t *g, uint32_t n, double *paths);
uint8_t stats_cache_pair_numpaths(  graph_t *g, uint32_t n, double *paths);

uint8_t stats_cache_edge_pathsharing(graph_t *g, uint32_t n, double *ps);
uint8_t stats_cache_edge_betweenness(graph_t *g, uint32_t n, double *eb);

#endif /* __STATS_CACHE_H__ */
