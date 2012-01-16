/**
 * Provides a function for ensuring that 
 * a given filename has a given suffix.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef SUFFIX_H
#define SUFFIX_H

/**
 * Ensures that the given file name ends with the given 
 * suffix. Space is malloced to store the new name, and
 * a pointer to it is returned (or NULL on failure).
 *
 * The caller is responsible for freeing the returned
 * pointer.
 */
char * set_suffix(
  char *oldname, /**< input file name */
  char *suffix   /**< desired suffix  */
);

/**
 * Extracts the prefix of the given name, storing it in the prefix pointer.
 */
void get_prefix(
  char *name,
  char *pref
);

#endif
