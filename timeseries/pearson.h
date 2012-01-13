/**
 * Implementation of Pearson's correlation coefficient between two time
 * series.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef __PEARSON_H__
#define __PEARSON_H__

#include <stdint.h>

double pearson(
  double  *x,
  double  *y,
  uint32_t len
);

#endif
