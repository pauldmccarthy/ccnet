/**
 * Functions for generating fastest-slowest dimension orderings. Used by
 * the dumpimg and dumpvolume programs. 
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/dimorder.h"


uint8_t dimorder_parse(
  dsr_t   *hdr,
  char    *order[],
  uint8_t *dims,
  uint8_t  orderlen)
{
  uint8_t i,j;
  uint8_t ndims;

  ndims = analyze_num_dims(hdr);

  if (orderlen > ndims) goto fail;

  for (i = 0; i < orderlen; i++) {

    if (strlen(order[i]) != 1) goto fail;
    if (!isdigit(order[i][0])) goto fail;

    dims[i] = atoi(order[i]);

    if (dims[i] >= ndims) goto fail;
  }

  /*
   * If the user didn't specify all dimensions in 
   * the ordering, complete the ordering by finding 
   * those dimensions not specified in the ordering, 
   * and adding them to the ordering, after the 
   * dimensions that were specified in the ordering. 
   * Ordering ordering ordering.
   */
  for (i = 0; orderlen < ndims && i < ndims; i++) {

    for (j = 0; j < orderlen; j++) if (dims[j] == i) break;
    if (j < orderlen) continue;

    dims[orderlen++] = i;
  }

  return 0;

fail: 
  return 1;
}


void dimorder_next(dsr_t *hdr, uint32_t *dims, uint8_t *dimorder) {

  uint32_t i;
  uint16_t ndims;
  uint32_t dimsz;

  ndims = analyze_num_dims(hdr);

  /*update dimension indices*/
  for (i = 0; i < ndims; i++) {
    
    dimsz = analyze_dim_size(hdr, dimorder[i]);
    dims[dimorder[i]] = (dims[dimorder[i]] + 1) % dimsz;
    
    if (dims[dimorder[i]] != 0) break;
  } 
}
