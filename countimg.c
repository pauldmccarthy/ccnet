/**
 * Prints a count of the number of voxels greater than or equal to a given
 * value in an ANALYZE75 image.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "io/analyze75.h"

static uint8_t _threshold(
  dsr_t *hdr, uint8_t *data, double thres);

int main(int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *data;
  uint8_t *ptr;
  double   threshold;
  uint32_t count;
  uint32_t nvals;
  uint8_t  valsz;
  uint64_t i;

  count     = 0;
  threshold = 0;
  data      = NULL;

  if (argc != 3) {
    printf("usage: countimg threshold file.img\n");
    goto fail;
  }

  threshold = atof(argv[1]);

  if (analyze_load(argv[2], &hdr, &data)) {
    printf("error loading image (%i)\n", errno);
    goto fail;
  }

  nvals = analyze_num_vals(&hdr);
  valsz = analyze_value_size(&hdr);
  ptr   = data;
  
  for (i = 0; i < nvals; i++) {

    if (_threshold(&hdr, ptr, threshold)) count++;
    ptr += valsz;
  }

  printf("%u / %u values above %0.3f\n", count, nvals, threshold);


  free(data);
  exit(0);

 fail:
  if (data != NULL) free(data);
  exit(1);
}

uint8_t _threshold(dsr_t *hdr, uint8_t *data, double thres) {

  double val;

  val = analyze_read(hdr, data);

  return (val >= thres) ? 1 : 0;
}
