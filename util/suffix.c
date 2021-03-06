/**
 * File name manipulation functions.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "suffix.h"

char * set_suffix(char *oldname, char *suffix) {

  int len;
  char *newname;
  char *oldprefix;
  char *oldsuffix;

  newname   = NULL;  
  oldprefix = NULL;
  oldsuffix = NULL;

  if (oldname == NULL) return NULL;
  if (suffix  == NULL) return NULL;

  len = strlen(oldname);

  oldprefix = malloc(len+1);
  if (oldprefix == NULL) goto fail;

  oldsuffix = malloc(len+1);
  if (oldsuffix == NULL) goto fail;

  get_prefix(oldname, oldprefix);
  get_suffix(oldname, oldsuffix);

  len = strlen(oldprefix) + strlen(suffix) + 2;

  newname = malloc(len);
  if (newname == NULL) goto fail;

  sprintf(newname, "%s.%s", oldprefix, suffix);

  free(oldprefix);
  free(oldsuffix);

  return newname;

fail:

  if (oldprefix != NULL) free(oldprefix);
  if (oldsuffix != NULL) free(oldsuffix);
  if (newname   != NULL) free(newname);
  return NULL;
}

void get_prefix(char *name, char *pref) {

  uint16_t len;
  char    *bn;
  char    *pref_end;

  if (name == NULL) return;
  if (pref == NULL) return;

  pref_end = NULL;
  bn       = basename(name);
  len      = strlen(name);

  pref_end = strrchr(bn, '.');
  
  if (pref_end == NULL) pref_end = name+len;

  for (; name < pref_end; name++,pref++) *pref = *name;

  *pref = '\0';
}

void get_suffix(char *name, char *suf) {
  
  int64_t  i;
  uint32_t len;
  uint32_t suf_len;
  int64_t  suf_start;

  if (name == NULL) return;
  if (suf  == NULL) return;

  suf_start = -1;
  len       = strlen(name);

  for (i = len-1; i >= 0; i--) {

    if (name[i] == '.') {
      suf_start = i+1;
      break;
    }
  }

  /*no suffix*/
  if (suf_start == -1 || suf_start == len) {
    suf[0] = '\0';
    return;
  }

  suf_len = len - suf_start;

  for (i = 0; i < suf_len; i++)
    suf[i] = name[suf_start + i];
  
  suf[suf_len] = '\0';
}


char * join_path(char *path, char *name) {

  uint32_t len;
  char    *joined;

  joined = NULL;

  len = strlen(path) + strlen(name) + 2;

  joined = malloc(len);
  if (joined == NULL) goto fail;

  sprintf(joined, "%s/%s", path, name);

  return joined;

fail:
  if (joined != NULL) free(joined);
  return NULL;
}


void dirname(char *path, char *dir) {

  char *bn;

  bn = strrchr(path, '/');

  if (bn == NULL) dir[0] = '\0';
  
  else {
    
    strncpy(dir, path, bn - path + 1);
    dir[bn - path + 1] = '\0';
  }
}

char * basename(char *path) {

  char *bn;

  bn = strrchr(path, '/');


  if (bn == NULL) return path;
  else            return bn + 1;
}
