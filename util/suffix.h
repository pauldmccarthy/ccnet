/**
 * File name manipulation functions.
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
  char *name, /**< input file name       */
  char *pref  /**< place to store prefix */
);

/**
 * Extracts the suffix of the given name, storing it in the suffix pointer.
 */
void get_suffix(
  char *name, /**< input file name       */
  char *suf   /**< place to store suffix */
);

/**
 * Joins the given directory path and file name, returning a newly allocated
 * string.
 *
 * \return a string containing the path and name joined, NULL on failure.
 */
char * join_path(
  char *path, /**< directory path */
  char *name  /**< file name      */
);

#endif
