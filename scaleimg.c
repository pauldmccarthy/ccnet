/**
 * Applies a multiplicative scaling factor to an ANALYZE75 image.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "io/analyze75.h"

static void _scaleimg(dsr_t *hdr, uint8_t *img, double scale);

int main(int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *img;
  double   scale;

  img  = NULL;

  if (argc != 4) {
    printf("usage: scaleimg input output scalefactor\n");
    goto fail;
  }

  scale = atof(argv[3]);

  if (analyze_load(argv[1], &hdr, &img)) {
    printf("error loading %s\n", argv[1]);
    goto fail;
  }

  _scaleimg(&hdr, img, scale);

  if (analyze_write_hdr(argv[3], &hdr))      goto fail;
  if (analyze_write_img(argv[3], &hdr, img)) goto fail;

  free(img);
  return 0;

 fail:
  if (img != NULL) free(img);
  return 1;
}

void _scaleimg(dsr_t *hdr, uint8_t *img, double scale) {

  uint32_t i;
  double   val;
  uint32_t nvals;
  uint8_t  valsize;

  valsize = analyze_value_size(hdr);
  nvals   = analyze_num_vals(  hdr);

  for (i = 0; i < nvals; i++) {

    val = analyze_read(hdr, img + (valsize*i));
    analyze_write(     hdr, img + (valsize*i), val*scale);
  }

  hdr->dime.cal_max *= scale;
  hdr->dime.cal_min *= scale;
  hdr->dime.glmax   *= scale;
  hdr->dime.glmin   *= scale;
}
