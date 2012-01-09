/**
 * Dynamically expanding data array.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stdint.h>

/**
 * Array handle. If the size field is not 0, data[size-1] is the
 * last value in the array.
 */
typedef struct _array {

  uint32_t  capacity; /**< current capacity                      */
  uint32_t  size;     /**< current size (number of values)       */
  uint32_t  datasz;   /**< size of one value in the data         */
  uint8_t  *data;     /**< the data                              */
  int     (*cmp)(     /**< search function for sorted searches   */
    const void *a,
    const void *b);
  int     (*cmpins)(  /**< search function for sorted insertions */
    const void *a,
    const void *b);

} array_t;

/**
 * Creates an array with the given initial capacity.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t array_create(
  array_t *array,   /**< handle to the array */
  uint32_t datasz,  /**< size of one value   */
  uint32_t capacity /**< initial capacity    */
);

/**
 * Sets the comparison functions for the given array. You could
 * just set them directly via array->cmp and array->cmpins.
 *
 * The cmp function is used by array_find for binary or linear
 * searches. If cmp is NULL, calls to the following functions
 * will fail:
 *  - array_find
 *  - array_remove_by_val
 *  - array_count
 *  - array_sort
 *  - array_insert_sorted
 *
 * The cmpins function is optionally used for sorted insertions,
 * via array_insert_sorted. If cmpins is not NULL, sorted
 * insertions will take O(log(n)) time. if cmpins is NULL, but
 * cmp is not NULL, sorted insertions will take O(n) time. If
 * both cmp and cmpins are NULL, calls to array_insert_sorted
 * will fail.
 */
void array_set_cmps(
  array_t *array,   /**< the array                            */
  int (*cmp)(       /**< function to use for searching        */
    const void *a,
    const void *b),
  int (*cmpins)(    /**< function to use for sorted insertion */
    const void *a,
    const void *b)
);

/**
 * Frees the memory used by the given array. Does not attempt to free the
 * array handle itself.
 */
void array_free(
  array_t *array /**< array to free */
);

/**
 * Discards the data in the array (simply sets the size to 0).
 */
void array_clear(
  array_t *array /**< array to clear */
);

/**
 * Resizes the given array so that it has the given capacity. If the array is
 * already large enough, does nothing.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t array_expand(
  array_t *array,   /**< array handle */
  uint32_t capacity /**< new capacity */
);

/**
 * Copies the value at the given index into the given pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t array_get(
  array_t *array, /**< array handle            */
  uint32_t idx,   /**< index                   */
  void    *data   /**< pointer to put value in */
);

/**
 * Get-direct - just returns a pointer to the data at the given index.
 *
 */
void *array_getd(
  array_t *array, /**< array handle */
  uint32_t idx    /**< index        */
);

/**
 * Set the value at the given index.  The index must be within the capacity of
 * the array.
 */
void array_set(
  array_t *array, /**< array handle */
  uint32_t idx,   /**< index        */
  void    *value  /**< new value    */
);

/**
 * Sets all values in the array to those stored in the given pointer.
 */
void array_set_all(
  array_t *array, /**< array handle          */
  void    *value  /**< pointer to new values */
);

/**
 * Finds a value in the array. If the array is sorted, set the is_sorted
 * paramater to non-0, and you'll get O(log(n)) performance; otherwise you'll
 * get O(n) performance. If the array->cmp function is NULL, calls to this
 * function  will fail.
 * 
 * \return the index, in the array, of the first value in the array which
 * matches the given value, -1 if the value is not present or the array's
 * comparison function has not been set.
 */
int64_t array_find(
  array_t *array,    /**< the array                        */
  void    *value,    /**< value to search for              */
  uint8_t  is_sorted /**< set to >0 if the array is sorted */
);

/**
 * \return the number of elements in the array which are equal to the given
 * value, according to the array->cmp comparison function.
 */
uint32_t array_count(
  array_t *array, /**< the array           */
  void    *value  /**< value to search for */
);

/**
 * Adds the given value to the end of the array. Increases the array capacity
 * if required.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t array_append(
  array_t *array, /**< array handle        */
  void    *value  /**< the value to append */
);

/**
 * Inserts the given value at the given index; all values following the
 * index are shifted forwards in the array.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t array_insert(
  array_t *array, /**< the array           */
  uint32_t idx,   /**< index               */
  void    *value  /**< the value to insert */
);

/**
 * Inserts the given value into the given array, maintaining ascending
 * order. The data in the given array must already be sorted. If idx is not
 * NULL, the insertion index is saved to it.
 *
 * \return 0 on success, 1 if unique is non-0, and the value was already
 * present, 2 on failure.

 */
uint8_t array_insert_sorted(
  array_t  *array,  /**< array handle              */
  void     *value,  /**< value to insert           */
  uint8_t   unique, /**< discard duplicate values? */
  uint32_t *idx     /**< place to put index        */
);

/**
 * Sorts the elements in the given array, using the comparison
 * function set via array_set_cmps. This is really just a wrapper
 * around qsort.
 */
void array_sort(
  array_t *array /**< array to sort */
);

/**
 * Removes the value at the given index from the array. All values following
 * the index are shifted backwards in the array.
 */
void array_remove_by_idx(
  array_t *array, /**< the array       */
  uint32_t idx    /**< index to remove */
);

/**
 * Removes the first element in the array which is equal to the given value.
 * All values following the removed element are shifted backwards in the
 * array. If the array is sorted, set the is_sorted field to >0 and you will
 * get O(log(n)) performance; otherwise you will get O(n) performance.
 *
 * \return the index of the removed value, -1 if the value was not present in
 * the array, or array->cmp is NULL.
 */
int64_t array_remove_by_val(
  array_t *array,    /**< the array                        */
  void    *val,      /**< value to remove                  */
  uint8_t  is_sorted /**< set to >0 if the array is sorted */
);

#endif /* __ARRAY_H__ */
