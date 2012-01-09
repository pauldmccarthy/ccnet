/**
 * Standard generic stack implementation. Memory is automatically increased
 * when the stack reaches its capacity.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/stack.h"

/**
 * Minimum capacity of a stack.
 */
#define MIN_CAPACITY 4

/**
 * The amount by which a stack's capacity is increased
 * when it runs out of space.
 */
#define EXPAND_RATIO 1.5

/**
 * Increases the capacity of the stack.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _expand(
  cstack_t *stack, /**< the stack to expand                          */
  uint32_t  newcap /**< new capacity - pass in 0 to use EXPAND_RATIO */
);

uint8_t stack_create(cstack_t *stack, uint32_t elemsz, uint32_t capacity) {

  if (capacity < MIN_CAPACITY) capacity = MIN_CAPACITY;

  stack->capacity = capacity;
  stack->elemsz   = elemsz;
  stack->size     = 0;
  stack->data     = calloc(capacity, elemsz);
  
  if (stack->data == NULL) goto fail;

  return 0;
  
fail:
  return 1;
}

void stack_free(cstack_t *stack) {

  if (stack != NULL && stack->data != NULL)
    free(stack->data);
}

void * stack_pop(cstack_t *stack) {

  void *val;

  if (stack       == NULL) return NULL;
  if (stack->size == 0)    return NULL;
  
  val = stack_peek(stack);
  stack->size --;
  
  return val;
}

void * stack_peek(cstack_t *stack) {

  void    *val;
  uint32_t off;

  if (stack       == NULL) return NULL;
  if (stack->size == 0)    return NULL;

  off = (stack->size - 1) * (stack->elemsz);
  val = stack->data + off;

  return val;
}

uint8_t stack_push(cstack_t *stack, void *elem) {

  uint32_t off;

  if (stack       == NULL)            goto fail;
  if (stack->size == stack->capacity) {
    if (_expand(stack, 0)) goto fail;
  }

  off = (stack->size) * (stack->elemsz);

  memcpy(stack->data + off, elem, stack->elemsz);

  stack->size++;

  return 0;

fail:
  return 1;
}

uint8_t _expand(cstack_t *stack, uint32_t newcap) {

  uint8_t *newdata;

  if (newcap == 0) {
    newcap = (uint32_t)(EXPAND_RATIO * (stack->capacity));
  }

  if (newcap <= stack->capacity) goto fail;

  newdata = realloc(stack->data, newcap*(stack->elemsz));
  if (newdata == NULL) goto fail;

  stack->data     = newdata;
  stack->capacity = newcap;

  return 0;
fail:
  return 1;
}
