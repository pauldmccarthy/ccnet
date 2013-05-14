/**
 * Read in simple text based graph files.
 *
 * An edge file is a plain text file which specifies the edges in an
 * unweighted, undirected graph. Each edge in the graph is specified on one
 * line of the file. An edge is specified by listing two numbers, which are
 * the (0-indexed) IDs of the endpoint nodes. An example edge file is:
 *
 * 0 1
 * 1 3
 * 3 4
 * 4 7
 *
 * Author: Paul McCarthy <pauldmccarthy@gmail.com>
 */
#ifndef EDGEFILE_H
#define EDGEFILE_H

#include <stdint.h>

#include "graph/graph.h"

/**
 * Create a graph from an edge file. Edge file specification is
 * in the header comments of this file.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t edgefile_read(
  graph_t *g,      /**< empty graph to create */
  uint32_t nnodes, /**< number of nodes       */
  char    *fname   /**< edge file to load     */
);


#endif
