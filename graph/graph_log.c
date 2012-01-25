/**
 * Little module to attach an audit trail to a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "util/array.h"
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

    msg = *(char **)array_getd(log, i);
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

char * graph_log_get_msg(graph_t *g, uint16_t i) {

  array_t *log;

  log = g->ctx[_GRAPH_LOG_CTX_LOC_];
  
  if (log == NULL)      return NULL;
  if (i   >= log->size) return NULL;

  return *(char **)array_getd(log, i);
}

uint8_t graph_log_add(graph_t *g, char *msg) {

  array_t *log;
  char    *msgcpy;
  uint32_t len;

  msgcpy = NULL;
  len    = strlen(msg);
  log    = g->ctx[_GRAPH_LOG_CTX_LOC_];
  
  if (log == NULL) return 0;

  msgcpy = malloc(len+1);
  if (msgcpy == NULL) goto fail;
  
  strcpy(msgcpy, msg);

  if (array_append(log, &msgcpy)) goto fail;

  return 0;
  
fail:
  if (msgcpy!= NULL) free(msgcpy);
  return 1;
}

uint16_t graph_log_total_len(graph_t *g) {

  uint64_t i;
  uint16_t len;  
  array_t *log;
  char    *msg;

  len = 0;
  log = g->ctx[_GRAPH_LOG_CTX_LOC_];

  if (log == NULL) return 0;

  for (i = 0; i < log->size; i++) {
    
    msg = *(char **)array_getd(log, i);
    len += strlen(msg);
  }

  return len;
}

uint8_t graph_log_import(graph_t *g, char *data, char *delim) {

  int32_t  len;
  uint16_t dlen;
  array_t *log;
  char    *msg;
  char    *substr;
  uint16_t substrlen;

  msg  = NULL;
  log  = g->ctx[_GRAPH_LOG_CTX_LOC_];
  len  = strlen(data);
  dlen = strlen(delim);

  if (log == NULL) return 0;
  
  while (len > 0) {
    
    substr = strstr(data, delim);

    if (substr == NULL) substrlen = len;
    else                substrlen = substr - data;

    msg = calloc(substrlen+1, 1);
    if (msg == NULL) goto fail;

    memcpy(msg, data, substrlen);
    msg[substrlen] = '\0';

    if (array_append(log, &msg)) goto fail;

    data += (substrlen + dlen);
    len  -= (substrlen + dlen);
  }

  return 0;
  
fail:
  return 1;
}

void graph_log_export(graph_t *g, char *dest, char *delim) {

  uint32_t i;
  array_t *log;
  uint16_t dlen;
  uint16_t len;
  char    *msg;

  dlen = strlen(delim);
  log  = g->ctx[_GRAPH_LOG_CTX_LOC_];
  
  if (log == NULL) return;

  for (i = 0; i < log->size; i++) {
    
    msg = *(char **)array_getd(log, i);
    len = strlen(msg);

    memcpy(dest, msg, len);
    dest += len;
    
    if (i < log->size - 1) {
      memcpy(dest, delim, dlen);
      dest += dlen;
    }
  }

  dest[0] = '\0';
}
