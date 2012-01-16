/**
 * Implementation of Pearson's correlation coefficient between two time
 * series.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef __CORRELATION_H__
#define __CORRELATION_H__

#include <stdint.h>

double pearson(
  double  *x,
  double  *y,
  uint32_t len
);

#endif
