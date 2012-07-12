/**
 * Functions for thresholding the edges of graphs.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __GRAPH_THRESHOLD_H__
#define __GRAPH_THRESHOLD_H__

#include <stdint.h>

#include "graph/graph.h"

/**
 * Data structure used by graph_threshold_modularity 
 * to return optional data.
 */
typedef struct _mod_opt {

  uint32_t  nvals;      /**< number of values in each of the arrays, 
                             equivalent to number of edges removed       */
  double   *modularity; /**< modularity after each edge has been removed */
  uint32_t *ncmps;      /**< number of components after 
                             each edge has been removed                  */

} mod_opt_t;

/**
 * Creates a new, unweighted graph from the weighted input graph by applying
 * the given threshold to the edges of the input graph. All edges which have 
 * a weight greater than or equal to the threshold are added to the output
 * graph. All other edges are excluded.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_threshold_weight(
  graph_t *gin,       /**< weighted input graph                           */
  graph_t *gout,      /**< pointer to an empty output graph               */
  double   threshold, /**< threshold to apply                             */
  uint8_t  absval,    /**< use absolute values for thresholding           */
  uint8_t  reverse    /**< remove edges above threshold, instead of below */
);

/**
 * Removes the given number of edges using the given remove function.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_threshold_edges(
  graph_t  *gin,     /**< input graph                                 */
  graph_t  *gout,    /**< place to put output graph                   */
  uint32_t  nedges,  /**< number of edges to remove                   */
  uint32_t  flags,   /**< Unused                                      */
  void     *opt,     /**< Unused                                      */
  uint8_t (*init)(   /**< function which does required initialisation */
    graph_t     *g),
  uint8_t (*remove)( /**< function which removes a single edge        */
    graph_t      *g,     /**< the graph                                    */
    double       *space, /**< space to store graph_num_nodes(gin) doubles  */
    array_t      *edges, /**< array to put graph_edge_t structs, if needed */
    graph_edge_t *edge), /**< set to the edge which was removed            */
  uint8_t (*recalc)( /**< function which recalculates graph statistics
                           after an edge has been removed             */
    graph_t      *g,    /**< the graph                           */
    graph_edge_t *edge) /**< end point of edge which was removed */
);

/**
 * Removes edges from the graph until it has broken up into the given number
 * of components.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_threshold_components(
  graph_t  *gin,      /**< input graph                                   */
  graph_t  *gout,     /**< output graph                                  */
  uint32_t  cmplimit, /**< number of components to stop on               */
  uint32_t  igndis,   /**< ignore components below this size             */
  void     *opt,      /**< Unused                                        */
  uint8_t (*init)(   /**< function which does required initialisation    */
    graph_t     *g), 
  uint8_t (*remove)( /**< function which removes a single edge        */
    graph_t      *g,     /**< the graph                                    */
    double       *space, /**< space to store graph_num_nodes(gin) doubles  */
    array_t      *edges, /**< array to put graph_edge_t structs, if needed */
    graph_edge_t *edge), /**< set to the edge which was removed            */
  uint8_t (*recalc)(  /**< function which recalculates graph statistics
                           after an edge has been removed                */
    graph_t      *g,    /**< the graph                           */
    graph_edge_t *edge) /**< end point of edge which was removed */
);

/**
 * Removes edges from the graph until the modularity is at its maximum
 * possible value. Removes at most the given number of edges.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_threshold_modularity(
  graph_t  *gin,       /**< input graph                                   */
  graph_t  *gout,      /**< output graph                                  */
  uint32_t  edgelimit, /**< number of edges to stop on                    */
  uint32_t  flags,     /**< unused                                        */
  void     *opt,       /**< If not NULL, assumed to be a pointer to a 
                            mod_opt_t struct - memory is allocated to 
                            store the modularity value and number of 
                            components after each edge has been removed   */
  uint8_t (*init)(     /**< function which does required initialisation   */
    graph_t     *g), 
  uint8_t (*remove)( /**< function which removes a single edge            */
    graph_t      *g,     /**< the graph                                    */
    double       *space, /**< space to store graph_num_nodes(gin) doubles  */
    array_t      *edges, /**< array to put graph_edge_t structs, if needed */
    graph_edge_t *edge), /**< set to the edge which was removed            */
  uint8_t (*recalc)(  /**< function which recalculates graph statistics
                           after an edge has been removed                */
    graph_t      *g,    /**< the graph                           */
    graph_edge_t *edge) /**< end point of edge which was removed */
);

/**
 * Removes edges from the graph until the chira fitness is at its maximum
 * possible value. Removes at most the given number of edges.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_threshold_chira(
  graph_t  *gin,       /**< input graph                                   */
  graph_t  *gout,      /**< output graph                                  */
  uint32_t  edgelimit, /**< number of edges to stop on                    */
  uint32_t  flags,     /**< unused                                        */
  void     *opt,       /**< If not NULL, assumed to be a pointer to a 
                            mod_opt_t struct - memory is allocated to 
                            store the chira value and number of 
                            components after each edge has been removed   */
  uint8_t (*init)(     /**< function which does required initialisation   */
    graph_t     *g), 
  uint8_t (*remove)( /**< function which removes a single edge            */
    graph_t      *g,     /**< the graph                                    */
    double       *space, /**< space to store graph_num_nodes(gin) doubles  */
    array_t      *edges, /**< array to put graph_edge_t structs, if needed */
    graph_edge_t *edge), /**< set to the edge which was removed            */
  uint8_t (*recalc)(  /**< function which recalculates graph statistics
                           after an edge has been removed                */
    graph_t      *g,    /**< the graph                           */
    graph_edge_t *edge) /**< end point of edge which was removed */
);

/**
 * Peforms any initialisation required for graph thresholding via
 * path-sharing (e.g. initial calculation/caching of path-sharing values).
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_init_pathsharing(
  graph_t *g /**< the graph */
);

/**
 * Removes the edge with the minimum pathsharing value. 
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_remove_pathsharing(
  graph_t      *g,     /**< the graph                                */
  double       *space, /**< space used to store path-sharing values  */
  array_t      *edges, /**< array used to store graph_edge_t structs */
  graph_edge_t *edge   /**< the removed edge is stored here          */
);

/**
 * Recalculates path-sharing values on the graph, after the given edge has
 * been removed. Only recalculates those edges which will have changed.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_recalculate_pathsharing(
  graph_t      *g,   /**< the graph              */
  graph_edge_t *edge /**< edge which was removed */
);

/**
 * Peforms any initialisation required for graph thresholding via
 * edge-betweenness.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_init_edge_betweenness(
  graph_t *g /**< the graph */
);

/**
 * Removes the edge with the maximum edge betweenness value. 
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_remove_edge_betweenness(
  graph_t      *g,     /**< the graph                                  */
  double       *space, /**< space used to store edge-betweennessvalues */
  array_t      *edges, /**< array used to store graph_edge_t structs   */
  graph_edge_t *edge   /**< the removed edge is stored here            */
);

/**
 * Recalculates edge-betweenness values on the graph, after the given edge has
 * been removed. Only recalculates those edges which will have changed.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_recalculate_edge_betweenness(
  graph_t      *g,   /**< the graph              */
  graph_edge_t *edge /**< edge which was removed */
);

#endif /* __GRAPH_THRESHOLD_H__ */
