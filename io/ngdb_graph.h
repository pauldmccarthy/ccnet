/**
 * Reads/writes a graph_t struct from/to a NGDB file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __NGDB_GRAPH_H__
#define __NGDB_GRAPH_H__

#include <stdint.h>

#include "io/ngdb.h"
#include "graph/graph.h"

#define NGDB_HDR_DATA_SIZE 8192

/**
 * Loads the graph contained in the given 
 * ngdb file into the given graph struct.
 *
 * Assumes that the ngdb file has a node 
 * data section of 16 bytes, containing 
 * a graph_label_t struct (see graph.h).
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t ngdb_read(
  char    *f, /**< name of ngdb file to load        */
  graph_t *g  /**< pointer to a graph struct to use */
);

/**
 * Writes the given graph to the given file.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t ngdb_write(
  graph_t *g, /**< graph to write      */
  char    *f  /**< file to write it to */
);

#endif /* __NGDB_GRAPH_H__ */
