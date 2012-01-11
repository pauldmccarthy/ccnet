/**
 * Provides a function which builds a stack of node indices; each
 * entry in the stack is an array (pointer to an array_t struct) containing
 * indices of nodes which are of equivalent distance from the source node.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/bfs.h"

/**
 * Breadth first search callback function. Adds the
 * current level to the level stack.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state,  /**< bfs state              */
  void        *context /**< pointer to level stack */
);

uint8_t graph_level_stack(graph_t *g, uint32_t u, cstack_t *stack) {

  stack->data = NULL;

  if (stack_create(stack, sizeof(array_t), 20)) goto fail;

  if (bfs(g, &u, 1, NULL, stack, NULL, _bfs_cb, NULL)) goto fail;

  return 0;
  
fail:
  
  if (stack->data != NULL) stack_free(stack);
  return 1;
}

uint8_t _bfs_cb(bfs_state_t *state, void *context) {

  uint64_t i;
  uint32_t  ni;
  cstack_t *st;
  array_t   level;

  st = (cstack_t *)context;

  if (array_create(&level, sizeof(uint32_t), state->thislevel.size))
    goto fail;

  for (i = 0; i < state->thislevel.size; i++) {

    array_get(&(state->thislevel), i, &ni);
    array_set(&level,              i, &ni);
  }

  stack_push(st, &level);

  return 0;
  
fail:
  return 1;
}
