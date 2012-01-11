/**
 * Read in Radatools lol files, and convert to node_partition_t struct.
 * 
 * See http://deim.urv.cat/~sgomez/radatools.php
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef LOL_H
#define LOL_H

#include <stdint.h>
#include "util/array.h"
#include "graph/graph.h"

/**
 * Loads the given lol file, creating a node_partition_t struct.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t lol_load(
  char             *fname, /**< name of the lolfile                         */
  node_partition_t *lol    /**< pointer to an empty node_partition_t struct */
);

#endif
