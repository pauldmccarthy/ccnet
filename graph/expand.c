/**
 * Provides the expand function, which can be used in breadth first searches
 * of a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <stdlib.h>

#include "util/array.h"
#include "graph/graph.h"
#include "graph/expand.h"

uint8_t expand(
  graph_t *g, 
  array_t *thislevel,
  array_t *nextlevel,
  uint8_t *visited,
  void    *context,
  uint8_t  (*callback) (
    expand_state_t *state,
    void           *context
  ))
{
  uint32_t       i;
  uint32_t       j;
  uint32_t       ni;
  uint32_t       nneighbours;
  uint32_t      *neighbours;
  expand_state_t state;

  neighbours = NULL;

  for (i = 0; i < thislevel->size; i++) {

    array_get(thislevel, i, &ni);

    nneighbours = graph_num_neighbours(g, ni);
    neighbours  = graph_get_neighbours(g, ni);

    for (j = 0; j < nneighbours; j++) {

      if (callback != NULL) {

        array_get(thislevel, i, &(state.parent));
        state.child   = neighbours[j];
        state.visited = visited[neighbours[j]];
        if (callback(&state, context)) {
          
          i = thislevel->size;
          return 1;
        }
      }

      if (!visited[neighbours[j]]) {
        array_append(nextlevel, &(neighbours[j]));
        visited[neighbours[j]] = 1;
      }
    }
  }

  return 0;
}
