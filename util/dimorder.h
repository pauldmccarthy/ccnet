/**
 * Functions for generating fastest-slowest dimension orderings. Used by
 * the dumpimg and dumpvolume programs.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef __DIMORDER_H__
#define __DIMORDER_H__

#include <stdint.h>

#include "io/analyze75.h"

/**
 * This function is called to generate the dimension
 * ordering (the fastest-to-slowest changing dimension,
 * which determines the order in which values are 
 * printed),
 *
 * \return 0 on success, non-0 otherwise.
 */
uint8_t dimorder_parse(
  dsr_t   *hdr,     /**< image header                  */
  char    *order[], /**< orderings that were passed in */
  uint8_t *dims,    /**< space to store ordering       */
  uint8_t  orderlen /**< length of order (and dims)    */
);

/**
 * Gets the value specified by the given dimension indices, and updates
 * the indices to point to the next value, based on the given dimension order.
 *
 * \return the value at the specified dimension indices.
 */
double dimorder_getval(
  dsr_t    *hdr,     /**< image header      */
  uint8_t  *img,     /**< image data        */
  uint32_t *dims,    /**< dimension indices */
  uint8_t  *dimorder /**< dimension order   */
);

#endif
