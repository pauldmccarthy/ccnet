/**
 * Read in an infomap .tree file, and convert to a node_partition_t struct.
 *
 * See http://www.tp.umu.se/~rosvall/code.html
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef INFOMAP_H
#define INFOMAP_H

#include <stdint.h>

#include "graph/graph.h"

/**
 * Loads the given .tree file, creating a node_partition_t struct.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t infomap_load(
  char             *fname,  /**< name of the lolfile                      */
  node_partition_t *infomap /**< pointer to empty node_partition_t struct */
);

#endif
