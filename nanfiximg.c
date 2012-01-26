/**
 * Replaces NaN values with zeros, in a 3D image.
 *
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

static uint8_t _nanfix(dsr_t *hdr, uint8_t *img); 

int main(int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *img;
  uint16_t datatype;

  startup("nanfiximg", argc, argv, NULL, NULL);

  if (argc != 3) {
    printf("usage: nanfiximg infile outfile\n");
    goto fail;
  }

  if (analyze_load(argv[1], &hdr, &img)) {
    printf("error loading %s\n", argv[1]);
    goto fail;
  }

  datatype = analyze_datatype(&hdr);

  if (datatype != DT_FLOAT && datatype != DT_DOUBLE) {
    printf("nan values don't occur for data type %u - "\
           "copying image anyway\n", datatype);
  }

  if (_nanfix(&hdr, img)) {
    printf("error fixing nan values\n");
    goto fail;
  }

  if (analyze_write_hdr(argv[2], &hdr)) {
    printf("error writing header %s\n", argv[2]);
    goto fail;
  }

  if (analyze_write_img(argv[2], &hdr, img)) {
    printf("error writing %s\n", argv[2]);
    goto fail;
  }

  free(img);
  return 0;

 fail:
  if (img != NULL) free(img);
  return 1; 
}

uint8_t _nanfix(dsr_t *hdr, uint8_t *img) {
  
  uint32_t i;
  double   val;
  uint32_t nvals;

  nvals   = analyze_num_vals(hdr);

  uint32_t nnan = 0;

  for (i = 0; i < nvals; i++) {

    val = analyze_read_by_idx(hdr, img, i);
    if (isnan(val)) {val = 0; nnan++;}

    analyze_write_by_idx(hdr, img, i, val);
  }

  printf("nnan: %u\n", nnan);

  return 0;
}
