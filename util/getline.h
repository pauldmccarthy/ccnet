/**
 * Replacement implementations of getline and getdelim in the event
 * that we are on OS X.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#ifndef __GETLINE_H__
#define __GETLINE_H__

#include <stdio.h>
#include <unistd.h>

ssize_t cnet_getline(char **lineptr, size_t *n, FILE *stream);
ssize_t cnet_getdelim(char **lineptr, size_t *n, int delim, FILE *stream);


#endif /* __GETLINE_H__ */
