/**
 * Little module to attach an audit trail to a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef _GRAPH_TRAIL_H_
#define _GRAPH_TRAIL_H_

#include <stdint.h>

#include "graph/graph.h"

/**
 * Struct containing the audit trail.
 */
typedef struct __trail {

  char   **trail;   /**< list of messages   */
  uint16_t trailen; /**< number of messages */

} trail_t;

/**
 * Creates and attaches a trail_t struct to the given graph.
 */
uint8_t graph_trail_init(
  graph_t *g /**< graph to attach an audit trail to */
);

/**
 * \return 1 if the graph has an associated trail, 0 otherwise.
 */
uint8_t graph_trail_exists(
  graph_t *g /**< graph to check */
);

/**
 * Adds the given message to the trail.
 */
uint8_t graph_trail_add(
  graph_t *g,  /**< graph          */
  char    *msg /**< message to add */
);

/**
 * \return the number of messages in the trail.
 */
uint16_t graph_trail_num_msgs(
  graph_t *g /**< graph to query */
);

/**
 * \return the total length, combined, of all the messages in the trail.
 */
uint16_t graph_trail_total_len(
  graph_t *g /**< the graph */
);

/**
 * Copies all of the messages from the given data, adding them to the trail
 * for the graph.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_trail_import(
  graph_t *g,    /**< graph to import to         */
  uint8_t *data, /**< data to parse              */
  char    *delim /**< delimiter between messages */
);

/**
 * Copies all of the messages in the trail to the given destination pointer,
 * separating them with the given delimiter string. The destination pointer
 * must have enough space to store this many bytes:
 *
 *   graph_trail_total_len(g) + (graph_trail_num_msgs(g)-1)*strlen(delim) + 1
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_trail_export(
  graph_t *g,    /**< the graph           */
  uint8_t *dest, /**< place to put string */
  char    *delim /**< delimiter string    */
);

#endif
