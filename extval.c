/**
 * Extracts the value for one or more voxels, from a 3D image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <argp.h>

#include "io/analyze75.h"
#include "util/startup.h"

static void _print_val(dsr_t *hdr, uint8_t *data, uint32_t *dims);

static uint32_t _unit_to_idx(dsr_t *hdr, uint8_t dim, double unit);

int main (int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *data;
  uint8_t  real;
  uint32_t nvoxels;
  uint64_t i;
  uint32_t idx[4];

  memset(idx, 0, sizeof(idx));

  startup("extval", argc, argv, NULL, NULL);

  if (argc < 6 || argc % 3) {
    printf("usage: extval file.img (r|v) x y z [x y z [...]]\n" 
           "  r: using real units\n"
           "  v: using voxel indices\n");
    goto fail;
  }

  if      (!strcmp(argv[2], "r")) real = 1;
  else if (!strcmp(argv[2], "v")) real = 0;
  else {
    printf("unknown index format (only 'r' or 'v' accepted):  %s\n", argv[2]);
    goto fail;
  }

  if (analyze_load(argv[1], &hdr, &data))  {
    printf("error loading image (%i)\n", errno);
    goto fail;
  }

  nvoxels = (argc - 3) / 3;

  for (i = 0; i < nvoxels; i++) {

    if (!real) {
      idx[0] = atoi(argv[3 + (i*3)    ]);
      idx[1] = atoi(argv[3 + (i*3) + 1]);
      idx[2] = atoi(argv[3 + (i*3) + 2]);
    }
    else {
      idx[0] = _unit_to_idx(&hdr, 0, atof(argv[3 + (i*3)    ]));
      idx[1] = _unit_to_idx(&hdr, 1, atof(argv[3 + (i*3) + 1]));
      idx[2] = _unit_to_idx(&hdr, 2, atof(argv[3 + (i*3) + 2]));
    }

    if (  idx[0] >= analyze_dim_size(&hdr, 0)
       || idx[1] >= analyze_dim_size(&hdr, 1)
       || idx[2] >= analyze_dim_size(&hdr, 2)) {

      printf("index [%u,%u,%u] out of bounds\n", idx[0], idx[1], idx[2]);
      continue;
    }

    _print_val(&hdr, data, idx);
  }

  return 0;

fail:
  return 1;
}


void _print_val(dsr_t *hdr, uint8_t *data, uint32_t *dims) {

  double   val;
  uint32_t idx;

  idx = analyze_get_index(hdr, dims);

  val = analyze_read_by_idx(hdr, data, idx);

  printf("%0.3f\n", val);
}

uint32_t _unit_to_idx(dsr_t *hdr, uint8_t dim, double unit) {

  float len;

  len = analyze_pixdim_size(hdr, dim);

  return (uint32_t)round(unit / len);
}
