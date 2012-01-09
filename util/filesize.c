/**
 * Provides the filesize function, which returns the 
 * size of a file given its open file descriptor.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <sys/stat.h>

#include "filesize.h"

int filesize(FILE *fd) {

  int fn;
  struct stat st;

  fn = fileno(fd);

  if (fn             == -1) return -1;
  if (fstat(fn, &st) == -1) return -1;

  return st.st_size;
}
