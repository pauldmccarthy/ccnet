/**
 * Implementation of Pearson's correlation coefficient between two time
 * series.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <math.h>
#include <stdint.h>

#include "timeseries/pearson.h"

double pearson(double *x, double *y, uint32_t len) {

  double   sumx;
  double   sumy;
  double   sumxy;
  double   sumx2;
  double   sumy2;
  double   numer;
  double   denom;
  uint64_t i;

  sumx = 0;
  sumy = 0;
  sumxy = 0;
  sumx2 = 0;
  sumy2 = 0;
  
  for (i = 0; i < len; i++) {
    
    sumx  += x[i];
    sumy  += y[i];
    sumxy += x[i] * y[i];
    sumx2 += x[i] * x[i];
    sumy2 += y[i] * y[i];
  }

  numer = (len * sumxy) - (sumx * sumy);
  denom = sqrt((len * sumx2) - sumx*sumx) *
          sqrt((len * sumy2) - sumy*sumy);

  if (isnan(denom)) return 0.0;
  
  return numer / denom;
}
