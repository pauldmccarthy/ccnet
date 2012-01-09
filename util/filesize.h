/**
 * Provides a function which, given an open 
 * file descriptor, returns the file size.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef FILESIZE_H
#define FILESIZE_H

#include <stdio.h>

/**
 * \return the size of the given file.
 */
int filesize(
  FILE *fd /**< Open file descriptor */
);

#endif
