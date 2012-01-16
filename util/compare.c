/**
 * Comparison functions for use with bsearch and qsort.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>

#include "util/compare.h"

/**
 * Generic compare function used by compare_*_insert functions, for managing
 * state during a bsearch_insert search.
 * 
 * Inspiration provided by:
 *   http://blog.dt.in.th/2010/01/where-to-insert-bsearch
 */ 
static int _compare_insert(
  const void  *a,
  const void  *b,
  const size_t sz,
  int (*cmp)(const void *, const void *)
);

int compare_str_numeric(const void *a, const void *b) {

  char  *sa;
  char  *sb;
  double da;
  double db;

  sa = (char *)a;
  sb = (char *)b;

  da = atof(sa);
  db = atof(sb);
  
  return compare_double(&da, &db);
}

int compare_double(const void *a, const void *b) {
  
  double da;
  double db;

  da = *(double *)a;
  db = *(double *)b;

  if (da >  db) return 1;
  if (da == db) return 0;
  return -1;
}

int compare_u32(const void *a, const void *b) {

  uint32_t ia;
  uint32_t ib;

  ia = *(uint32_t *)a;
  ib = *(uint32_t *)b;

  if (ia >  ib) return 1;
  if (ia == ib) return 0;
  return -1;
}

int compare_u32_insert(const void *a, const void *b) {

  return _compare_insert(a, b, sizeof(uint32_t), compare_u32);
}

int _compare_insert(
  const void  *a,
  const void  *b,
  const size_t sz,
  int (*cmp)(const void *, const void *)) {

  static const void *last;
  const char        *cb;
  int                abcmp;

  if (a == NULL) {
    last = b;
    return 0;
  }

  cb    = b;
  abcmp = cmp(a, b);

  if (abcmp >= 0) {

    if (b == last)         return 0;
    if (cmp(a, cb+sz) < 0) return 0;
  }

  else if (abcmp < 0)
    return -1;

  return 1;
}

void *bsearch_insert(
  const void *key,
  const void *base,
  size_t      nmemb,
  size_t      size,
  int       (*compar)(const void *, const void *)) {

  const char *cbase;
  const void *last;

  cbase = base;
  last  = cbase + ((nmemb-1)*size);

  _compare_insert(NULL, last, 0, NULL);

  return bsearch(key, base, nmemb, size, compar);
}
