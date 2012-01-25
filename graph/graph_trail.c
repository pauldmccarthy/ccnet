/**
 * Little module to attach an audit trail to a graph_t struct.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>

#include "graph/graph.h"
#include "graph/graph_trail.h"


//static void _trail_free(void *trail);


uint8_t graph_trail_init(graph_t *g) {

  return  0;
}

uint8_t graph_trail_exists(graph_t *g) {

  return 0;
}

uint16_t graph_trail_num_msgs(graph_t *g) {
  
  return 0;
}

void _trail_free(void *trail) {

}

uint8_t graph_trail_add(graph_t *g, char *msg) {

  return 0;
}

uint16_t graph_trail_total_len(graph_t *g) {

  return 0;
}

uint8_t graph_trail_import(graph_t *g, uint8_t *data, char *delim) {

  return 0;
}

uint8_t graph_trail_export(graph_t *g, uint8_t *dest, char *delim) {

  return 0;
}


