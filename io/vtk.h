/**
 * Output a graph in VTK format. You can either use vtk_print_graph, which
 * will print out a complete VTK file, or use a combination of the other
 * functions.
 * 
 * If you choose the latter option, you must call vtk_print_hdr first to print
 * a file header. If you don't, the file will be invalid, and it will be your
 * own fault. You may then optionally print out the nodes and edges, and then,
 * associated node scalars. If you want node scalars, you must print nodes
 * before the scalars.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __VTK_H__
#define __VTK_H__

#include <stdio.h>
#include <stdint.h>

#include "graph/graph.h"

/**
 * Writes the given graph as a VTK POLYDATA type, to the given file handle.
 * This function will output a complete, valid VTK file, with node scalar
 * data, by calling the other functions defined in this file. If you want to
 * do something more complex, use the other functions directly. 
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t vtk_print_graph(
  FILE    *fout,         /**< output handle to write to            */
  graph_t *graph,        /**< the graph to print                   */
  uint8_t  nscalars,     /**< number of scalar arrays              */
  char   **scalar_names, /**< names of node scalar data - 
                              must have length nscalars            */
  double **scalars       /**< list of scalar arrays - must have 
                              length nscalars, with each entry 
                              having length graph_num_nodes(graph) */
);

/**
 * Print the file header and polygon points.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t vtk_print_hdr(
  FILE    *f, /**< file handle */
  graph_t *g  /**< the graph   */
);

/**
 * Print the graph nodes.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t vtk_print_nodes(
  FILE    *f, /**< file handle */
  graph_t *g  /**< the graph   */
);

/**
 * Print the graph edges.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t vtk_print_edges(
  FILE    *f, /**< file handle */
  graph_t *g  /**< the graph   */
);

/**
 * Print the given node scalar data.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t vtk_print_node_scalar(
  FILE    *f,     /**< file handle                                       */
  graph_t *g,     /**< the graph                                         */
  uint8_t  first, /**< pass in non-0 if this is the first scalar to be
                       printed, 0 otherwise. If you don't, it's your
                       own fault                                         */
  char    *name,  /**< scalar name                                       */
  double  *data   /**< scalar data, must be graph_num_nodes(g) in length */
);


#endif /* __VTK_H__ */
