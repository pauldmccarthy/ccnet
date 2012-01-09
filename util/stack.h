/**
 * A standard generic stack implementation.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __STACK_H__
#define __STACK_H__

#include <stdint.h>

/**
 * Struct representing a stack. The typedef is 'cstack_t' because, in OS X,
 * the file /usr/include/sys/_structs.h is implicitly included, and already
 * contains a 'stack_t'.
 */
typedef struct _stack {

  uint8_t *data;     /**< the stack                        */
  uint32_t size;     /**< current stack size               */
  uint32_t elemsz;   /**< size of one element in the stack */
  uint32_t capacity; /**< maximum stack capacity           */

} cstack_t;

/**
 * Initialises the given stack, allocating enough memory for the given
 * capacity.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t stack_create(
  cstack_t *stack,   /**< pointer to an unitialised stack  */
  uint32_t  elemsz,  /**< size of one element in the stack */
  uint32_t  capacity /**< initial stack capacity           */
);

/**
 * Frees memory used by the stack. Does not free the stack pointer.
 */ 
void stack_free(
  cstack_t *stack /**< stack to free */
);

/**
 * Removes and returns the top element from the stack.
 * 
 * \return the top element off the stack, NULL if the stack is empty.
 */
void * stack_pop(
  cstack_t *stack /**< the stack */
);

/**
w * \return the top element from the stack, but does not remove it.
 */
void * stack_peek(
  cstack_t *stack /**< the stack */
);

/**
 * Pushes the given element onto the top of the stack.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t stack_push(
  cstack_t *stack, /**< the stack   */
  void     *elem   /**< the element */
);

#endif /* __STACK_H__ */
