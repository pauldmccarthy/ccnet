/**
 * Comparison functions for use with bsearch and qsort.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __COMPARE_H__
#define __COMPARE_H__

#include <stdlib.h>

/**
 * Convert two strings to numbers, then compare them numerically.
 *
 * \return >0 if (*a > *b), 0 if (*a == *b), <0 if (*a < *b).
 */
int compare_str_numeric(
  const void *a, /**< pointer to a string which contains a number */
  const void *b  /**< pointer to a string which contains a number */
);

/**
 * Compares two double values.
 *
 * \return >0 if (*a > *b), 0 if (*a == *b), <0 if (*a < *b).
 */
int compare_double(
  const void *a, /**< pointer to a double */
  const void *b  /**< pointer to a double */
);

/**
 * Compares two uint32_t values.
 *
 * \return >0 if (*a > *b), 0 if (*a == *b), <0 if (*a < *b).
 */
int compare_u32(
  const void *a, /**< pointer to a uint32_t */
  const void *b  /**< pointer to a uint32_t */
);

/**
 * Compares two uint32_t values. Not for use with regular bsearch - for use
 * with bsearch_insert. 
 *
 * \return 0 if (*a >= *b) and b is the last element in the array, or *a is
 * less than the next value in the array, <0 if *a < *b, >0 otherwise.
 */
int compare_u32_insert(
  const void *a, /**< pointer to a uint32_t */
  const void *b  /**< pointer to a uint32_t */
);

/**
 * Alternative version of bsearch which, when a value is not present in the
 * array, instead of returning NULL, returns a pointer to the location of the
 * array that the value should be inserted. Use the compare_*_insert functions
 * with this function.
 *
 * \return NULL if the value is less than the first element in the array,
 * otherwise a pointer to the location of the value, or the location that the
 * value should be inserted.
 */
void *bsearch_insert(
  const void *key,    /**< pointer to value to search for  */
  const void *base,   /**< pointer to start of the array   */
  size_t      nmemb,  /**< number of elements in the array */
  size_t      size,   /**< size of one element             */
  int       (*compar)(
    const void *,
    const void *)     /**< compare_*_insert function       */
);

#endif /*  __COMPARE_H__ */
