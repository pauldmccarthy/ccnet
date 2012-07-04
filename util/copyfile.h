/**
 * Function which copies a file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef COPYFILE_H
#define COPYFILE_H

#include <stdint.h>

/**
 * Copies the file specified by the src to the a new file specified by dst.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t copyfile(
  char *src, /**< name of source file      */
  char *dst  /**< name of destination file */
);

#endif
