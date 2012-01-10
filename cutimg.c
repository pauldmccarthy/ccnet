/**
 * Program which cuts a single volume into a series of 
 * images. The volume is cut up along the last dimension.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "io/analyze75.h"
#include "util/suffix.h"
#include "util/filesize.h"

/**
 * Splits the given volume into a series of images.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _split(
  dsr_t   *hdr,    /**< volume header                 */
  uint8_t *img,    /**< volume data                   */
  char    *outdir, /**< output directory              */
  uint8_t  pref    /**< start number of output images */
);

/**
 * Generates a header for a single image file.
 */
static void _mk_hdr(
  dsr_t   *inhdr,  /**< volume header               */
  dsr_t   *outhdr, /**< place to store image header */
  uint8_t *img,    /**< image data                  */
  uint32_t imgsz   /**< image size in bytes         */
);

int main (int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *img;
  uint8_t  pref;

  pref = 1;

  img = NULL;

  if (argc != 3 && argc != 4) {
    printf("usage: cutimg input outdir [prefix]\n");
    goto fail;
  }

  if (argc == 4) pref = atoi(argv[3]);

  if (analyze_load(argv[1], &hdr, &img)) goto fail;

  if (_split(&hdr, img, argv[2], pref)) goto fail;

  free(img);
  return 0;

fail:
  if (img != NULL) free(img);
  return 1;
}

uint8_t _split(dsr_t *hdr, uint8_t *img, char *outdir, uint8_t pref) {

  dsr_t     newhdr;
  uint32_t  i;
  uint16_t  ndims;
  uint32_t  dimsz;
  uint8_t   valsize;
  uint32_t  cutsize;
  uint32_t  dimoff;
  FILE     *f;
  char     *filename;

  f        = NULL;
  filename = NULL;

  ndims   = analyze_num_dims(  hdr);
  valsize = analyze_value_size(hdr);
  dimsz   = analyze_dim_size(  hdr, ndims-1);
  dimoff  = analyze_dim_offset(hdr, ndims-1);

  cutsize = dimoff*valsize;

  filename = malloc(strlen(outdir) + 20);
  if (filename == NULL) goto fail;

  for (i = 0; i < dimsz; i++, pref++, f = NULL) {

    sprintf(filename, "%s/%03u.img", outdir, pref);
    f = fopen(filename, "wb");
    if (f == NULL) goto fail;

    if (fwrite(img + (i*cutsize), 1, cutsize, f) != cutsize) goto fail;
    fclose(f);

    _mk_hdr(hdr, &newhdr, img + (i*cutsize), cutsize);

    if (analyze_write_hdr(filename, &newhdr)) goto fail;
  }
  
  free(filename);
  return 0;

fail:
  if (f        != NULL) fclose(f);
  if (filename != NULL) free(filename);

  return 1;
}

void _mk_hdr(dsr_t *inhdr, dsr_t *outhdr, uint8_t *img, uint32_t imgsz) {

  uint8_t ndims;

  ndims = analyze_num_dims(inhdr);

  memcpy(outhdr, inhdr, sizeof(dsr_t));

  outhdr->dime.dim   [ndims] = 1;
  outhdr->dime.pixdim[ndims] = 0.0;
}
