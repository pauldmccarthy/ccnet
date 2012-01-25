/**
 * Little module to attach an audit trail to a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef _GRAPH_LOG_H_
#define _GRAPH_LOG_H_

#include <stdint.h>

#include "graph/graph.h"

/**
 * Creates and attaches an array_t of char* pointers to the given graph.
 */
uint8_t graph_log_init(
  graph_t *g /**< graph to attach an audit trail to */
);

/**
 * \return 1 if the graph has an associated log, 0 otherwise.
 */
uint8_t graph_log_exists(
  graph_t *g /**< graph to check */
);

/**
 * Adds the given message to the log.
 */
uint8_t graph_log_add(
  graph_t *g,  /**< graph          */
  char    *msg /**< message to add */
);

/**
 * \return the number of messages in the log.
 */
uint16_t graph_log_num_msgs(
  graph_t *g /**< graph to query */
);

/**
 * \return the total length, combined, of all the messages in the log.
 */
uint16_t graph_log_total_len(
  graph_t *g /**< the graph */
);

/**
 * Copies all of the messages from the given data, adding them to the log
 * for the graph. The data are assumed to be '\0' terminated.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t graph_log_import(
  graph_t *g,    /**< graph to import to         */
  char    *data, /**< data to parse              */
  char    *delim /**< delimiter between messages */
);

/**
 * Copies all of the messages in the log to the given destination pointer,
 * separating them with the given delimiter string. The destination pointer
 * must have enough space to store this many bytes:
 *
 *   graph_log_total_len(g) + (graph_log_num_msgs(g)-1)*strlen(delim) + 1
 */
void graph_log_export(
  graph_t *g,    /**< the graph           */
  char    *dest, /**< place to put string */
  char    *delim /**< delimiter character */
);

#endif
