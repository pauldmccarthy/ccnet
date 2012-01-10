/**
 * Function to reverse an array.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>

#include "reverse.h"

void reverse(void *src, void *dst, int len) {

  int i;
  uint8_t tmp;
  uint8_t *usrc = (uint8_t *)src;
  uint8_t *udst = (uint8_t *)dst;

  if (src == NULL) return;
  if (dst == NULL) return;
  if (len == 0)    return;

  for (i = 0; i < len/2; i++) {
  
    tmp           = usrc[i];
    udst[i]       = usrc[len-1-i];
    udst[len-1-i] = tmp;
  }
}
