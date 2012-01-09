/**
 * Function to reverse an array.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef REVERSE_H
#define REVERSE_H

/**
 * Reverses the bytes in src, saving the result to dst. 
 * src and dst may be the same array, in which case the 
 * reversal occurs in place.
 */
void reverse(void *src, void *dst, int len);

#endif
