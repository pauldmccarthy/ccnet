/**
 * Model of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <stdint.h>

#include "io/analyze75.h"
#include "util/array.h"
#include "util/stack.h"

#define _GRAPH_CTX_SIZE_             5
#define _GRAPH_STATS_CACHE_CTX_LOC_  1
#define _GRAPH_LOG_CTX_LOC_          2

/**
 * Graph flag bit locations.
 */
typedef enum _graph_flags {

  GRAPH_FLAG_DIRECTED = 0

} graph_flags_t;

typedef struct _graph {

  uint32_t        numnodes;      /**< number of nodes in the graph       */
  uint32_t        numedges;      /**< number of edges in the graph       */
  array_t         numneighbours; /**< number of neighbours for each node */
  array_t         nodelabels;    /**< node labels                        */
  array_t         labelvals;     /**< all unique node label values       */
  array_t        *neighbours;    /**< neighbours for each node           */
  array_t        *weights;       /**< weights for each edge              */
  uint16_t        flags;         /**< graph flags                        */

  array_t         event_listeners; /**< array of registered event listeners */

  /*
   * There's no need to look at any of the stuff below. It provides space
   * for specific modules to attach a context to a graph struct.
   */

  void *ctx[_GRAPH_CTX_SIZE_];                /**< Context fields        */
  void (*ctx_free[_GRAPH_CTX_SIZE_])(void *); /**< Functions for freeing 
                                                   context.              */

} graph_t;

/**
 * Struct representing a label attached to nodes in a graph. I will make this
 * more generic when I really have to.
 */
typedef struct _graph_label {

  uint32_t labelval; /**< label value  */
  float    xval;     /**< x-coordinate */
  float    yval;     /**< y-coordinate */
  float    zval;     /**< z-coordinate */

} graph_label_t;

/**
 * Struct representing an edge between two nodes. This struct
 * is not used in the graph_t struct, but may be useful elsewhere.
 */
typedef struct _graph_edge {
  
  uint32_t u;   /**< node u                     */
  uint32_t v;   /**< node v                     */
  double   val; /**< value associated with edge */
  
} graph_edge_t;

/**
 * Struct representing a group of nodes with the same label, in the same
 * component. Used by the graph_communities function.
 */
typedef struct _node_group {

  uint32_t component; /**< component number                                */
  uint32_t labelval;  /**< label value                                     */
  uint32_t labelidx;  /**< index of label value in graph_get_nodelabels(g) */
  uint32_t nnodes;    /**< number of nodes in the group                    */

} node_group_t;

/**
 * Struct representing some grouping of nodes into partitions. Used by the
 * graph connect functions.
 */
typedef struct _node_partition {

  uint32_t nparts; /**< Number of partitions                  */
  uint32_t nnodes; /**< Total number of nodes                 */
  array_t *parts;  /**< List of arrays, each containing
                        uint32_t values representing node IDs */

} node_partition_t;

/**
 * \return 0 if the graph is undirected, non-0 otherwise.
 */
uint8_t graph_is_directed(
  graph_t *g /**< graph to query */
);

/**
 * \return the number of nodes in the graph,
 */
uint32_t graph_num_nodes(
  graph_t *g /**< graph to query */
);

/**
 * \return the number of edges in the graph.
 */
uint32_t graph_num_edges(
  graph_t *g /**< graph to query */
);

/**
 * \return the number of unique node label values in the graph.
 */
uint32_t graph_num_labelvals(
  graph_t *g /**< graph to query */
);

/**
 * \return a pointer to the unique node label values in the graph.
 */
uint32_t *graph_get_labelvals(
  graph_t *g /**< graph to query */
);

/**
 * \return a pointer to the label for the given node, NULL if this graph has
 * no labels.
 */
graph_label_t *graph_get_nodelabel(
  graph_t       *g,   /**< graph to query         */
  uint32_t       nidx /**< node to query          */
);

/**
 * \return the number of neighbours of the given node.
 */
uint32_t graph_num_neighbours(
  graph_t *g,   /**< graph to query */
  uint32_t nidx /**< node to query  */
);

/**
 * \return a pointer to the neighbours of the given node.
 */
uint32_t *graph_get_neighbours(
  graph_t *g,   /**< graph to query */
  uint32_t nidx /**< node to query  */
);

/**
 * \return the index of node j in node i's list of neighbours, -1 if node
 * j is not a neighbour of node i.
 */
int64_t graph_get_nbr_idx(
  graph_t *g, /**< graph to query */
  uint32_t i, /**< node to query  */
  uint32_t j  /**< node to find   */
);

/**
 * \return the weight of the given edge.
 */
double graph_get_weight(
  graph_t *g, /**< graph to query      */
  uint32_t u, /**< edge endpoint       */
  uint32_t v  /**< other edge endpoint */
);

/**
 * Sets the weight for the given edge.
 */
uint8_t graph_set_weight(
  graph_t *g, /**< the graph           */
  uint32_t u, /**< edge endpoint       */
  uint32_t v, /**< other edge endpoint */
  float    wt /**< new edge weight     */
);

/**
 * \return a pointer to the weights for neighbours of the given node.
 */
float *graph_get_weights(
  graph_t *g,   /**< graph to query */
  uint32_t nidx /**< node to query  */
);

/**
 * \return 1 if the given nodes are neighbours, 0 otherwise.
 */
uint8_t graph_are_neighbours(
  graph_t *g, /**< graph to query */
  uint32_t u, /**< first node     */
  uint32_t v  /**< second node    */
);

/**
 * \return the length of the shortest path from u
 * to v, 0 if there is no path from u to v. If the
 * path parameter is not NULL, the nodes contained
 * in the path from u to v are stored in the path
 * parameter.
 */
uint32_t graph_pathlength(
  graph_t *g,   /**< the graph to query                              */
  uint32_t u,   /**< first node                                      */
  uint32_t v,   /**< second node                                     */
  array_t *path /**< if not NULL, intermediate nodes are stored here */
);

/**
 * Populates the given array with the IDs of the nodes in the given
 * component.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_get_component(
  graph_t  *g,      /**< the graph                 */
  uint32_t  cmp,    /**< the component to retrieve */
  array_t  *nodeids /**< array to put node IDs     */
);

/**
 * Populates the given stack with arrays (pointers to array_t
 * structs) of node indices; the top array contains the indices
 * of the nodes which are the furthest from the source node,
 * and the bottom array contains those nodes which are closest
 * to the source node.
 *
 * The caller is responsible for freeing the stack elements,
 * and the stack itself.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_level_stack(
  graph_t  *g,    /**< the graph                           */
  uint32_t  u,    /**< source node                         */
  cstack_t *stack /**< uninitialised stack to be populated */
);

/**
 * Builds a list of the 'communities' present in the graph, where a community
 * is defined as a group of nodes with the same label value, which are in the
 * same component.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_communities(
  graph_t *g,     /**< the graph                                            */
  uint32_t sz,    /**< minimum component size to be considered a community  */
  array_t *groups /**< an initialised array, to put node_group_t structs in */
);

/**
 * Initialise the given graph_t struct for the given number of nodes. 
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_create(
  graph_t  *g,        /**< pointer to an empty graph_t struct */
  uint32_t  numnodes, /**< number of nodes                    */
  uint8_t   directed  /**< directed or undirected             */
);

/**
 * Frees the memory used by the given graph. Does not attempt to free the
 * graph_t struct itself.
 */
void graph_free(
  graph_t *g /**< the graph to free */
);

/**
 * Creates a complete copy of the input graph; the gout pointer is
 * initialised, and updated so that it points to the copy.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_copy(
  graph_t *gin, /**< graph to copy                  */
  graph_t *gout /**< pointer to uninitialised graph */
);

/**
 * Add an edge to the given graph. If the graph is undirected,
 * two edges are added - one from u to v, and one from v to u.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_add_edge(
  graph_t *g, /**< the graph             */
  uint32_t u, /**< edge start point      */
  uint32_t v, /**< edge end point        */
  float    wt /**< edge weight           */
);

/**
 * Remove an edge from the given graph. 
 *
 * \return 0 on success, non-0 on failure, or if the edge does not exist.
 */
uint8_t graph_remove_edge(
  graph_t *g, /**< the graph             */
  uint32_t u, /**< edge start point      */
  uint32_t v  /**< edge end point        */
);

/**
 * Sets the label for the given graph. The given label is copied into the
 * graph struct.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_set_nodelabel(
  graph_t       *g,   /**< the graph        */
  uint32_t       nid, /**< node ID          */
  graph_label_t *lbl  /**< pointer to label */
);

/**
 * Copies node labels from gin to gout. The graphs must have the same number
 * of nodes.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_copy_nodelabels(
  graph_t *gin, /**< source graph      */
  graph_t *gout /**< destination graph */
);
  

/**
 * Generates an Erdos-Renyi random graph with the given number of nodes and
 * given density. All possible edges in the graph are included with the
 * probability specified by the 'density' parameter.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_create_er_random(
  graph_t *g,      /**< pointer to an uninitialised graph           */
  uint32_t nnodes, /**< number of nodes                             */
  double   density /**< desired graph density (between 0.0 and 1.0) */
);


/**
 * Checks to see whether a path exists between all nodes in the given group,
 * when it is taken as an independent subgraph; i.e. if two nodes in the graph
 * are connected, but only via a path which includes nodes that are not in the
 * group, those two nodes are considered to be disconnected.
 *
 * \return 1 if the given nodes are connected, 0 otherwise.
 */
uint8_t graph_are_connected(
  graph_t  *g,     /**< the graph            */
  uint32_t *group, /**< list of node indices */
  uint32_t  ngroup /**< number of nodes      */
);

/**
 * Modifies the given graph, to ensure that a path exists between all of the
 * nodes in the given group.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_connect(
  graph_t  *g,     /**< the graph            */
  uint32_t *group, /**< list of node indices */
  uint32_t  ngroup /**< number of nodes      */
);

/**
 * Copies the connectivity for the group from the source graph to the
 * graph g.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_connect_from(
  graph_t  *g,     /**< graph to connect                      */
  graph_t  *src,   /**< graph from which to copy connectivity */
  uint32_t *group, /**< list of node indices                  */
  uint32_t  ngroup /**< number of nodes in group              */
);

/**
 * Generates a random graph with approximately the given number of nodes,
 * containing densely connected clusters. The size of each cluster is
 * uniformly distributed over the range ((1-sizerange)*(nnodes/nclusters),
 * (1+sizerange)*(nnodes/nclusters)). Edges within clusters are included
 * with probability 'internal'. Edges between clusters are included with
 * probability 'external'.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_create_clustered(
  graph_t *graph,     /**< pointer to an uninitialised graph */
  uint32_t nnodes,    /**< desired number of nodes           */
  uint32_t nclusters, /**< number of clusters                */
  double   internal,  /**< intra-cluter density (0.0-1.0)    */
  double   external,  /**< inter-cluster density (0.0-1.0)   */
  double   sizerange  /**< size range (0.0-1.0)              */
);

/**
 * Create a clustered graph, the same as graph_create_clustered; this function
 * gives the option of specifying the total graph density; the inter-cluster
 * density will be automatically calculated.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_create_clustered_by_total(
  graph_t *graph,     /**< pointer to an uninitialised graph */
  uint32_t nnodes,    /**< desired number of nodes           */
  uint32_t nclusters, /**< number of clusters                */
  double   internal,  /**< intra-cluter density (0.0-1.0)    */
  double   total,     /**< total graph density (0.0-1.0)     */
  double   sizerange  /**< size range (0.0-1.0)              */
);


/**
 * Create a clustered graph, the same as graph_create_clustered; this function
 * gives the option of specifying average intra- and inter-cluster degrees,
 * rather than densities.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_create_clustered_by_degree(
  graph_t *graph,     /**< pointer to an uninitialised graph */
  uint32_t nnodes,    /**< desired number of nodes           */
  uint32_t nclusters, /**< number of clusters                */
  double   intdegree, /**< intra-cluster degree              */
  double   extdegree, /**< inter-cluster degree              */
  double   sizerange  /**< size range (0.0-1.0)              */
);

/**
 * Creates a graph from an ANALYZE75 image, according to the Normalized cut
 * graph creation algorithm.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_create_ncut(
  graph_t *graph,
  dsr_t   *hdr,
  uint8_t *img,
  double   si,
  double   sx,
  double   rad,
  double   thres
);

#endif /* __GRAPH_H__ */
