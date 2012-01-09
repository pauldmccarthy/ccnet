/**
 * Replacement implementations of getline and getdelim in the event
 * that we are on OS X.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "util/getline.h"

/**
 * amount to increase memory allocation by when we run out of space
 */
#define __GETDELIM_BUF_SZ__ 30

ssize_t cnet_getline(char **lineptr, size_t *n, FILE *stream) {
  return cnet_getdelim(lineptr, n, '\n', stream);
}

ssize_t cnet_getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {

  int c;
  int nchars;

  nchars   = 0;
  
  if (lineptr == NULL ||
      n       == NULL ||
      ferror(stream)) {
    errno = EINVAL;
    goto fail;
  }

  if (*lineptr != NULL &&
      *n       == 0) {
    errno = EINVAL;
    goto fail;
  }

  if (*lineptr == NULL) {
    
    *lineptr = malloc(__GETDELIM_BUF_SZ__);
    if (*lineptr == NULL) goto fail;
    
    *n = __GETDELIM_BUF_SZ__;
  }

  c = fgetc(stream);

  while (c != EOF && c != delim) {

    (*lineptr)[nchars++] = c;

    if (nchars >= (*n) - 2) {

      *lineptr = realloc(*lineptr, *n + __GETDELIM_BUF_SZ__);
      if (*lineptr == NULL) goto fail;
      
      *n += __GETDELIM_BUF_SZ__;
    }

    c = fgetc(stream);
  }

  if (c == EOF) goto fail;

  if (c == delim) (*lineptr)[nchars++] = delim;

  (*lineptr)[nchars] = '\0';

  return nchars;
  
fail:
  return -1;
}
