/**
 * Dynamically expanding array.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include <stdio.h>

#include "util/compare.h"
#include "util/array.h"

/**
 * Minimum capacity of an array.
 */
#define MIN_CAPACITY 2

/**
 * The amount by which an array's capacity is increased
 * when it runs out of space.
 */
#define EXPAND_RATIO 1.5

/**
 * Increases the capacity of the given array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _expand(
  array_t *array, /**< the array to expand                           */
  uint32_t newcap /**< new capacity - pass in 0 to use EXPAND_RATIO */
);

uint8_t array_create(array_t *array, uint32_t datasz, uint32_t capacity) {

  if (capacity < MIN_CAPACITY) capacity = MIN_CAPACITY;

  array->size     = 0;
  array->datasz   = datasz;
  array->capacity = capacity;
  array->cmp      = NULL;
  array->cmpins   = NULL;
  array->data     = calloc(capacity, datasz);
  
  if (array->data == NULL) goto fail;
  
  return 0;
fail:
  return 1;
}

void array_set_cmps(
  array_t *array,
  int    (*cmp)   (const void *a, const void *b),
  int    (*cmpins)(const void *a, const void *b)
) {

  if (array == NULL) return;
  
  array->cmp    = cmp;
  array->cmpins = cmpins;
}

void array_free(array_t *array) {

  if (array == NULL) return;
    
  if (array->data != NULL) free(array->data);
  array->capacity = 0;
  array->size     = 0;
  array->data     = NULL;
  array->cmp      = NULL;
  array->cmpins   = NULL;
}

void array_clear(array_t *array) {
  
  if (array != NULL) array->size = 0;
}

uint8_t array_expand(array_t *array, uint32_t capacity) {

  if (array           == NULL)     goto fail;
  if (array->capacity >= capacity) return 0;

  return _expand(array, capacity);
  
fail:
  return 1;
}

uint8_t array_get(array_t *array, uint32_t idx, void *data) {

  if (idx >= array->capacity) return 1;

  memcpy(data, (array->data)+(idx*(array->datasz)), array->datasz);

  return 0;
}

void * array_getd(array_t *array, uint32_t idx) {

  if (idx >= array->capacity) return NULL;

  return (array->data)+(idx*(array->datasz));
}

void array_set(array_t *array, uint32_t idx, void *value) {

  if (idx >= array->capacity) return;

  memcpy(array->data + (idx*(array->datasz)), value, array->datasz);

  if (idx >= array->size) array->size = idx+1;
}

void array_set_all(array_t *array, void *value) {

  if (array == NULL) return;
  if (value == NULL) return;

  memcpy(array->data, value, (array->size) * (array->datasz));
}

int64_t array_find(array_t *a, void *val, uint8_t is_sorted) {

  uint8_t *elem;
  size_t  sz;

  if (a      == NULL) return -1;
  if (val    == NULL) return -1;
  if (a->cmp == NULL) return -1;

  sz = a->size;

  if (is_sorted) elem = bsearch(val, a->data, a->size, a->datasz, a->cmp);
  else           elem = lfind(  val, a->data, &sz,     a->datasz, a->cmp);

  if (elem == NULL) return -1;

  return (elem - a->data) / a->datasz;
}

uint32_t array_count(array_t *array, void *value) {

  uint64_t i;
  uint32_t count;
  void    *value2;

  if (array      == NULL) return 0;
  if (array->cmp == NULL) return 0;

  count = 0;

  for (i = 0; i < array->size; i++) {
    
    value2 = array->data + i*(array->datasz);

    if (array->cmp(value, value2) == 0) count++;
  }

  return count;
}

uint8_t array_append(array_t *array, void *value) {

  if (array == NULL) goto fail;

  if (array->size == array->capacity) {
    if (_expand(array, 0)) goto fail;
  }

  array_set(array, array->size, value);

  return 0;
  
fail:
  return 1;
}

uint8_t array_insert(array_t *array, uint32_t idx, void *val) {

  uint8_t *src;
  uint8_t *dst;
  uint32_t sz;

  if (array == NULL) goto fail;
  if (val   == NULL) goto fail;

  if (array->size == array->capacity) {
    if (_expand(array, 0)) goto fail;
  }

  if (idx >= array->capacity) goto fail;

  if (idx < array->size) {

    sz  = (array->size -  idx)   * (array->datasz);
    src =  array->data +  idx    * (array->datasz);
    dst =  array->data + (idx+1) * (array->datasz);

    memmove(dst, src, sz);
    array->size ++;
  }

  array_set(array, idx, val);
  
  return 0;
  
fail:
  return 1;
}

uint8_t array_insert_sorted(
  array_t *a, void *val, uint8_t unique, uint32_t *idx) {

  int64_t  i;
  int      cmpval;
  uint8_t *elem;

  cmpval = 1;

  if ( a         == NULL) goto fail;
  if ( a->cmp    == NULL
    && a->cmpins == NULL) goto fail;

  if (idx != NULL) *idx = 0;

  /*log(n) time if an insertion comparison function has been set*/
  if (a->cmpins) {
    
    elem = bsearch_insert(val, a->data, a->size, a->datasz, a->cmpins);

    /*value belongs at the front of the array*/
    if (elem == NULL) {
      
      i      = 0;
      cmpval = -1;
    }

    else {
      
      i      = ((elem - a->data) / a->datasz) + 1;
      cmpval = a->cmp(val, elem);
    }
  }

  /*otherwise a linear search is required*/
  else {

    for (i = 0; i < a->size; i++) {
    
      elem   = a->data + i*(a->datasz);
      cmpval = a->cmp(val, elem);

      if (cmpval  < 0)           break;
      if (cmpval == 0 && unique) break;
    }
  }

  if (cmpval == 0 && unique) return 1;
  
  if (array_insert(a, i, val)) goto fail;

  if (idx != NULL) *idx = i; 
  
  return 0;
  
fail:
  return 2;
}

void array_sort(array_t *array) {

  if (array      == NULL) return;
  if (array->cmp == NULL) return;

  qsort(array->data, array->size, array->datasz, array->cmp);
}

void array_remove_by_idx(array_t *array, uint32_t idx) {

  uint8_t *src;
  uint8_t *dst;
  uint32_t sz;

  if (array == NULL)      return;
  if (idx >= array->size) return;

  sz  = (array->size -  idx)   * (array->datasz);
  src =  array->data + (idx+1) * (array->datasz);
  dst =  array->data +  idx    * (array->datasz);

  array->size --;
  memmove(dst, src, sz);
}

int64_t array_remove_by_val(array_t *array, void *val, uint8_t is_sorted) {

  uint32_t idx;

  idx = array_find(array, val, is_sorted);

  if (idx == -1) return -1;

  array_remove_by_idx(array, idx);
  
  return idx;
}

uint8_t _expand(array_t *array, uint32_t newcap) {

  uint8_t *newdata;

  if (newcap == 0) {
    newcap = (uint32_t)ceil(EXPAND_RATIO * (array->capacity));
  }

  /*this will never happen, but if it does, it will be bad*/
  if (newcap <= array->capacity) goto fail;

  newdata = realloc(array->data, newcap*(array->datasz));
  if (newdata == NULL) goto fail;

  array->data     = newdata;
  array->capacity = newcap;

  return 0;
fail:
  return 1;
}
