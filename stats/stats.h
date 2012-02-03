/**
 * Function prototypes for graph statistics. Unless otherwise noted, all
 * statistics assume the graph to be unweightedd and undirected.
 *
 * Many of these functions make use of the stats_cache.h functions - complex
 * statistics are calculated once, and then stored in the cache, so future
 * calls to the function doesn't result in recalculation.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __STATS_H__
#define __STATS_H__

#include <stdint.h>

#include "util/array.h"
#include "graph/graph.h"

/**
 * \return the density of the given graph.
 */
double stats_density(
  graph_t *g /**< the graph to query */
);

/**
 * \return the average degree of the given graph.
 */
double stats_avg_degree(
  graph_t *g /**< the graph to query */
);

/**
 * \return the maximum degree of the given graph.
 */
double stats_max_degree(
  graph_t *g /**< the graph to query */
);

/**
 * \return the average clustering coefficient of the given graph.
 */
double stats_avg_clustering(
  graph_t *g /**< the graph to query */
);

/**
 * Returns an approximation of the clustering coefficient by sampling a number
 * of triples (3 connected nodes) in the given graph. The returned clustering
 * coefficient is the ratio of the number of triangles (fully connected
 * triples) to the number of triples sampled.

 * Assumes that the stdlib random number generator has already been seeded.
 *
 *   Schank T & Wagner D 2005. Approximating Clustering
 *   Coefficient and Transitivity. Journal of Graph
 *   Algorithms and Applications, 9:2:265-275
 */
double stats_approx_clustering(
  graph_t *g,       /**< the graph to query              */
  uint32_t ntriples /**< the number of triples to sample */
);

/**
 * \return the characteristic path length of the given graph.
 */
double stats_avg_pathlength(
  graph_t *g /**< the graph to query */
);

/**
 * \return the small world index of the given graph. The values are compared
 * to the clustering coefficient and the path length of an Erdos-Renyi
 * randomly generated graph.
 */
double stats_smallworld_index(
  graph_t *g /**< the graph to query */
);

/**
 * \return the approximate average path length of an Erdos-Renyi random graph
 * which has the same order and density of the given graph.
 */
double stats_er_pathlength(
  graph_t *g /**< the graph to query */
);

/**
 * \return the approximate average clustering coefficient of an Erdos-Renyi
 * random graph which has the same order and density of the given graph.
 */
double stats_er_clustering(
  graph_t *g /**< the graph to query */
);

/**
 * \return the assortativity of the given graph.
 */
double stats_assortativity(
  graph_t *g /**< the graph to query */
);

/**
 * \return the number of components in the graph.
 */
uint32_t stats_num_components(
  graph_t  *g,      /**< the graph to query                               */
  uint32_t  sz,     /**< minimum component size to search for             */
  array_t  *sizes,  /**< if not NULL, the component sizes are stored here */
  uint32_t *cmpnums /**< if not NULL, component numbers for each node are
                         stored here                                      */
);

/**
 * \return the spatial span of the given component, that is, the maximum
 * distance between all pairs of nodes in the component, or a negative value
 * on failure.
 */
double stats_component_span(
  graph_t *g,  /**< the graph     */
  uint32_t cmp /**< the component */
);

/**
 * \return the number of connected nodes 
 * (nodes with a degree >= 1) in the graph.
 */
uint32_t stats_connected(
  graph_t *g /**< the graph to query*/
);

/**
 * \return the degree of the given node.
 */
uint32_t stats_degree(
  graph_t *g,   /**< the graph to query */
  uint32_t nidx /**< the node to query  */
);

/**
 * \return the clustering coefficient of the given node.
 */
double stats_clustering(
  graph_t *g,    /**< the graph to query */
  uint32_t nidx  /**< the node to query  */
);

/**
 * Calculates the characteristic path length of the given node using a breadth
 * first search through the graph from that node. The pathlens pointer may be
 * NULL, in which case the individual path lengths are not stored or returned.
 *
 * Definition of characteristic path length:
 *   Watts DJ & Strogatz Sh 1998. Collective dynamics 
 *   of small world networks. Nature, 393:440-442.
 *
 * \return the characteristic path length of the given node. 
 */
double stats_pathlength(
  graph_t  *g,        /**< the graph to query                       */
  uint32_t  nidx,     /**< the node to query                        */
  double   *pathlens  /**< place to store individual path lengths   */
);

/**
 * Calculates the global efficiency of the graph.
 *
 * Definition of efficiency:
 *
 *   Massimo Marchiori, Vito Latora 2001. Efficient behaviour of 
 *   small-world networks. Physical Review Letters 87(19):198701
 *
 * \return the global efficiency of the graph.
 */
double stats_global_efficiency(
  graph_t *g /**< the graph to query */
);

/**
 * \return the local efficiency for the given node.
 */
double stats_local_efficiency(
  graph_t *g,   /**< the graph to query */
  uint32_t nidx /**< the node to query  */
);

/**
 * \return the degree (or point) centrality for the given node.
 */
double stats_degree_centrality(
  graph_t *g,   /**< the graph to query */
  uint32_t nidx /**< the node to query  */
);

/**
 * \return the closeness centrality for the given node.
 */
double stats_closeness_centrality(
  graph_t *g,   /**< the graph to query */
  uint32_t nidx /**< the node to query  */
);

/**
 * \return the betweenness centrality for the given node.
 */
double stats_betweenness_centrality(
  graph_t *g,   /**< the graph to query */
  uint32_t nidx /**< the node to query  */
);

/**
 * Counts the number of shortest paths which exist between the given node and
 * all other nodes in the graph.  Optionally saves the per-node path count in
 * the provided numpaths array, which must be graph_num_nodes(g) in length. If
 * you don't care about the per-node counts, pass in NULL.
 *
 * \return the total number of paths from this node to all other nodes.
 */
double stats_numpaths(
  graph_t  *g,       /**< the graph to query         */
  uint32_t  nidx,    /**< the node to query          */
  double   *numpaths /**< array to store path counts */
);

/**
 * \return the pathsharing value for the edge which lies between the two given
 * nodes.
 */
double stats_edge_pathsharing(
  graph_t *g, /**< the graph    */
  uint32_t u, /**< one node     */
  uint32_t v  /**< another node */
);

/**
 * Calculates the modularity of the given graph; modularity gives an
 * indication of the extent to which the graph is made up of densely
 * connected communities. The communities in this case are defined
 * by the provided communities array; it is assumed that community 
 * IDs start from 0.
 *
 *   MEJ Newman 2006. Modularity and community structure
 *   in networks. PNAS vol. 103, no. 23, pp 8577-8582
 *
 * \return the average graph modularity.
 */
double stats_modularity(
  graph_t  *g,            /**< graph to query              */
  uint32_t  ncommunities, /**< number of communities       */
  uint32_t *communities   /**< community IDs for each node */
);

/**
 * \return the number of edges which lie between nodes with the same
 * label value. 
 */
double stats_num_intra_edges(
  graph_t *g,    /**< graph to query                       */
  double  *inter /**< if not NULL, number of inter-cluster
                      edges are stored here                */
);

/**
 * Copies the edge betweenness, for edges adjacent to the given node, into
 * the given betweenness pointer. The pointer must be large enough to store
 * (graph_num_neighbours(g,v)) values.
 *
 * If called before stats_avg_edge_betweenness has been called, this function
 * will call stats_avg_edge_betweenness.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t stats_edge_betweenness(
  graph_t *g,          /**< graph to query                     */
  uint32_t v,          /**< node to query                      */
  double  *betweenness /**< if not NULL, values for edges
                            adjacent to node v are stored here */
);

/**
 * \return the spatial distance between the two given nodes, according to the
 * coordinates in their label, if present. If the graph has no labels, returns
 * a negative value.
 */
double stats_edge_distance(
  graph_t *g, /**< the graph    */
  uint32_t u, /**< one node     */
  uint32_t v  /**< another node */
);

/**
 * \return the average spatial distances of edges of the given node.
 */ 
double stats_avg_edge_distance(
  graph_t *g, /**< the graph */
  uint32_t u  /**< the node  */
);

/**
 * Calculates the classification error for the given graph.
 *
 *   The criterion for deciding correct classification is as follows. We find
 *   the largest set of vertices that are grouped together by the algorithm in
 *   each of the four known communities. If the algorithm puts two or more of
 *   these sets in the same group, then all vertices in those sets are
 *   considered incorrectly classified. Otherwise, they are considered
 *   correctly classified. All other vertices not in the largest sets are
 *   considered incorrectly classified.
 *
 *     M. E. J. Newman 2004. Fast algorithm for
 *     detecting community structure in networks.
 *     Physical Review E, vol. 69, no. 6, pg. 066133.
 */
double stats_newman_error(
  graph_t *g
);

/**
 * Calculates the normalised mutual information on the given graph.
 *
 *   Danon L, Dutch J, Diaz-Guilera A, Arenas A. 2005. Comparing
 *   community structure identification. Journal of Statistical
 *   Mechanics: Theory and Experiment, vol. 2005, no. 9, pg. 09008.
 */
double stats_mutual_information(
  graph_t *g,    /**< the graph                         */
  uint8_t  disco /**< non-0: ignore disconnected nodes  */
);

/**
 * \return the number of nodes with the given label value.
 */
uint32_t stats_num_labelled_nodes(
  graph_t *g,       /**< the graph            */
  uint32_t labelval /**< label value to count */
);

#endif /* __STATS_H__ */
