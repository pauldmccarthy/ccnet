/**
 * Breadth first search through a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/array.h"
#include "graph/graph.h"
#include "graph/bfs.h"
#include "graph/expand.h"

uint8_t bfs(
  graph_t    *g,
  uint32_t   *roots,
  uint32_t    nroots,
  uint8_t    *subgraphmask,
  void       *lvl_context,
  void       *edge_context,
  uint8_t   (*lvl_callback) (
    bfs_state_t *state,
    void        *context),
  uint8_t   (*edge_callback) (
    expand_state_t *state,
    void           *context))
{
  uint64_t    i;
  bfs_state_t state;
  array_t     tmp;       /* temp pointer used for swapping levels      */
  array_t     nextlevel; /* array to store nodes in next level         */
  uint8_t    *visited;   /* whether nodes have or haven't been visited */
  uint32_t    numnodes;  /* number of nodes in graph                   */

  visited              = NULL;
  nextlevel.data       = NULL;
  state.thislevel.data = NULL;

  numnodes = graph_num_nodes(g);

  if (array_create(&(state.thislevel), sizeof(uint32_t), numnodes/4))
    goto fail;
  if (array_create(&nextlevel,         sizeof(uint32_t), numnodes/4))
    goto fail;

  visited = calloc(numnodes, sizeof(uint8_t));
  if (visited == NULL) goto fail;

  if (subgraphmask != NULL) 
    memcpy(visited, subgraphmask, numnodes*sizeof(uint8_t));

  for (i = 0; i < nroots; i++) {
    array_append(&(state.thislevel), &(roots[i]));
    visited[roots[i]] = 1;
  }

  state.depth   = 0;
  state.visited = visited;

  do {

    array_clear(&nextlevel);
    if (state.depth > 0) 
      if (lvl_callback != NULL && lvl_callback(&state, lvl_context))
        break; 

    if (expand(
      g,
      &(state.thislevel),
      &nextlevel,
      visited,
      edge_context,
      edge_callback)) break;

    state.depth++;
   
    
    memcpy(&tmp,               &(state.thislevel), sizeof(array_t));
    memcpy(&(state.thislevel), &nextlevel,         sizeof(array_t));
    memcpy(&nextlevel,         &tmp,               sizeof(array_t));
    
  } while (state.thislevel.size != 0);

  array_free(&(state.thislevel));
  array_free(&nextlevel);
  free(visited);

  return 0;

fail:
  array_free(&(state.thislevel));
  array_free(&nextlevel);
  if (visited != NULL) free(visited);
  return 1;
}
