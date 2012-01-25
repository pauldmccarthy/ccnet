/**
 * Little module to attach an audit trail to a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "graph/graph_log.h"

/**
 * Frees the memory used by the given trailx.
 */
static void _log_free(
  void *log /**< pointer to an array_t struct containing messages */
);

uint8_t graph_log_init(graph_t *g) {

  array_t *log;

  log = calloc(sizeof(array_t), 1);
  if (log == NULL) goto fail;

  if (array_create(log, sizeof(char *), 10)) goto fail;

  g->ctx[     _GRAPH_LOG_CTX_LOC_] = log;
  g->ctx_free[_GRAPH_LOG_CTX_LOC_] = _log_free;

  return 0;
  
fail:
  if (log != NULL) free(log);
  return 1;
}

void _log_free(void *vlog) {

  array_t *log;
  char    *msg;
  uint64_t i;

  log = vlog;

  for (i = 0; i < log->size; i++) {

    msg = array_getd(log, i);
    if (msg != NULL) free(msg);
  }

  array_free(log);
}

uint8_t graph_log_exists(graph_t *g) {

  return g->ctx[_GRAPH_LOG_CTX_LOC_] != NULL ? 1 : 0;
}

uint16_t graph_log_num_msgs(graph_t *g) {

  array_t *log;

  log = g->ctx[_GRAPH_LOG_CTX_LOC_];
  
  if (log == NULL) return 0;

  return log->size;
  
  return 0;
}

uint8_t graph_log_add(graph_t *g, char *msg) {

  array_t *log;
  char    *msgcpy;

  log = g->ctx[_GRAPH_LOG_CTX_LOC_];
  if (log == NULL) return 0;
  

  

  return 0;
}

uint16_t graph_log_total_len(graph_t *g) {

  return 0;
}

uint8_t graph_log_import(graph_t *g, uint8_t *data, char *delim) {

  return 0;
}

uint8_t graph_log_export(graph_t *g, uint8_t *dest, char *delim) {

  return 0;
}


