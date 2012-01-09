/**
 * Provides a function, which takes a file name, 
 * and ensures that it has a given suffix.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdlib.h>
#include <string.h>

#include "suffix.h"

/**
 * It's ugly and dangerous, and there are a ton of 
 * situations it won't handle. Just letting you know.
 */
char * suffix(char *oldname, char *suffix) {

  int len;
  char *newname;

  if (oldname == NULL) return NULL;
  if (suffix  == NULL) return NULL;

  len = strlen(oldname);

  newname = malloc(len + 10);
  if (newname == NULL) goto fail;

  /*already has correct suffix*/
  if (!strcmp(oldname+(len-3), suffix)) {

    memcpy(newname, oldname, len+1);
  }

  /*has dot, but no suffix*/
  else if (oldname[len-1] == '.') {

    memcpy(newname, oldname, len+1);
    memcpy(newname+len, suffix, 3);
    newname[len+3] = '\0';
  }

  /*doesn't have suffix*/
  else if (oldname[len-4] != '.') {

    memcpy(newname, oldname, len+1);
    newname[len] = '.';
    memcpy(newname+len+1, suffix, 3);
    newname[len+4] = '\0';
  }

  /*has different suffix*/
  else {

    memcpy(newname, oldname, len);
    memcpy(newname+(len-3), suffix, 3);
    newname[len] = '\0';
  }

  return newname;

fail:
  return NULL;
}
