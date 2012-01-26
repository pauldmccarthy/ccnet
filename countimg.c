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
#include "util/startup.h"

int main(int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *data;
  double   threshold;
  double   val;
  uint32_t count;
  uint32_t nvals;
  uint64_t i;

  count     = 0;
  threshold = 0;
  data      = NULL;

  startup("countimg", argc, argv, NULL, NULL);

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

  for (i = 0; i < nvals; i++) {

    val = analyze_read_by_idx(&hdr, data, i);
    
    if (val >= threshold) count++;
  }

  printf("%u / %u values above %0.3f\n", count, nvals, threshold);

  free(data);
  exit(0);

 fail:
  if (data != NULL) free(data);
  exit(1);
}
