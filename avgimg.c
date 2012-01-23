/**
 * Program which can read in a number of ANALYZE75 image files,
 * and create a new image with voxel values that are the average
 * of the input images.
 *
 * All of the input images must have the same dimension sizes 
 * - the dim and pixdim fields in the headers must be identical. 
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

#include "io/analyze75.h"

/**
 * Loads all of the specified images into memory. Space is allocated
 * to store each header and each image. The function will fail if 
 * all of the image dimensions do not match.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _load_images(
  uint8_t    nfiles, /**< number of input images                        */
  char     **files,  /**< image file names                              */
  dsr_t    **hdrs,   /**< will be set to point to the allocated headers */
  uint8_t ***imgs    /**< will be set to point to the allocated images  */
);

/**
 * Creates an averaged image from the given input images.
 *
 * return 0 on success, non-0 on failure.
 */
static uint8_t _mk_avg_img(
  uint16_t  datatype, /**< datatype of average image          */
  uint8_t   nimgs,    /**< number of input images             */  
  dsr_t    *hdrs,     /**< input headers                      */
  uint8_t **imgs,     /**< input images                       */
  uint8_t **avgimg,   /**< pointer which will be set to point
                           to allocated space                 */
  dsr_t    *avghdr    /**< space to store the averaged header */
);

int main (int argc, char *argv[]) {

  uint8_t   nimgs;
  uint16_t  datatype;
  char     *outfile;
  dsr_t    *hdrs;
  uint8_t **imgs;
  uint8_t  *avgimg;
  dsr_t     avghdr;
  uint16_t  i;

  if (argc < 4) {
    printf("usage: avgimg format output input1 [input2 ...]\n");
    printf("Supported formats:\n\
  2  - unsigned char (1 byte)\n\
  4  - signed short  (2 bytes)\n\
  8  - signed int    (4 bytes)\n\
  16 - float         (4 bytes)\n\
  64 - double        (8 bytes)\n");
    return 1;
  } 

  hdrs     = NULL;
  imgs     = NULL;
  avgimg   = NULL;
  nimgs    = argc-3;
  datatype = atoi(argv[1]);
  outfile  = argv[2];

  /*load input images and headers in*/
  if (_load_images(nimgs, argv+3, &hdrs, &imgs)) {
    printf("error loading input images\n");
    goto fail;
  }

  /*check that the images can be averaged*/
  if (analyze_hdr_compat(nimgs, hdrs)) {
    printf("input images failed verification - "\
           "check they are the same dimensions \n");
    goto fail;
  }

  /*make average image*/
  if (_mk_avg_img(datatype, nimgs, hdrs, imgs, &avgimg, &avghdr)) {
    goto fail;
  }

  /*write average image out*/
  if (analyze_write_hdr(outfile, &avghdr))         goto fail;
  if (analyze_write_img(outfile, &avghdr, avgimg)) goto fail;
  
  free(hdrs);
  free(avgimg);
  for (i = 0; i < nimgs; i++) free(imgs[i]);
  free(imgs);
  return 0;

fail:

  printf("fail?\n");
  if (hdrs != NULL) free(hdrs);
  if (avgimg != NULL) free(avgimg);

  if (imgs != NULL) {
    for (i = 0; i < nimgs; i++) {
      if (imgs[i] != NULL) free(imgs[i]);
    }
    free(imgs);
  }
  return 1;
}

uint8_t _load_images(
  uint8_t    nfiles, 
  char     **files, 
  dsr_t    **hdrs, 
  uint8_t ***imgs)
{
  uint8_t i;

  dsr_t    *lhdrs;
  uint8_t **limgs;

  lhdrs  = NULL;
  limgs  = NULL;

  /*allocate space to store headers, and image pointers*/
  lhdrs = calloc(nfiles, sizeof(dsr_t));
  if (lhdrs == NULL) goto fail;

  limgs = calloc(nfiles, sizeof(uint8_t *));
  if (limgs == NULL) goto fail;

  /*load all of the files in*/
  for (i = 0; i < nfiles; i++) {
    if (analyze_load(files[i], lhdrs+i, limgs+i)) 
      goto fail;
  }

  *hdrs = lhdrs;
  *imgs = limgs;

  return 0;

fail:
  if (lhdrs != NULL) free(*hdrs); 
  if (limgs != NULL) {

    for (i = 0; i < nfiles; i++) {
      if (limgs[i] != NULL)
        free(limgs[i]);
    }

    free(limgs);
  }
  *hdrs = NULL;
  *imgs = NULL;
  return 1;
}

uint8_t _mk_avg_img(
  uint16_t  datatype,
  uint8_t   nimgs,  
  dsr_t    *hdrs,
  uint8_t **imgs,
  uint8_t  **avgimg, 
  dsr_t    *avghdr)
{
  uint64_t i;
  uint16_t j;
  double   val;
  uint32_t avgvalsz;
  uint32_t nvals;
  uint8_t *lavgimg;
  double   min;
  double   max;

  lavgimg = NULL;
  min     =  DBL_MAX;
  max     = -DBL_MAX;
  
  /*allocate space for average image*/
  nvals    = analyze_num_vals(     hdrs);
  avgvalsz = analyze_datatype_size(datatype);

  lavgimg = malloc(avgvalsz * nvals);
  if (lavgimg == NULL) {
    printf("out of memory?!\n");
    goto fail;
  }

  memcpy(avghdr, hdrs, sizeof(dsr_t));
  avghdr->dime.datatype = datatype;
  avghdr->dime.bitpix   = avgvalsz * 8;

  /*
   * Read in, average, and write out, one value at 
   * a time. Keep track of the minimum/maximum 
   * values to put into the header afterwards
   */
  for (i = 0; i < nvals; i++) {

    val = 0;

    for (j = 0; j < nimgs; j++)  {
      val += analyze_read_by_idx(hdrs+j, imgs[j], i);
    }

    val /= nimgs;

    if (val < min) min = val;
    if (val > max) max = val;

    analyze_write_by_idx(avghdr, lavgimg, i, val);
  }

  avghdr->dime.cal_max = (float)   max;
  avghdr->dime.cal_min = (float)   min;
  avghdr->dime.glmax   = (uint32_t)max;
  avghdr->dime.glmin   = (uint32_t)min;


  *avgimg = lavgimg;

  return 0;

fail:
  if (lavgimg != NULL) free(lavgimg);
  return 1;
}
