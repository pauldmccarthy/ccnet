/**
 * Program which shifts the voxels in a 3D ANALYZE75 file, allowing
 * wrap-around.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <argp.h>
#include <errno.h>

#include "io/analyze75.h"

#define MAX_SHIFTS 50

static char doc[] = "shiftimg -- shift data in ANALYZE75 image files\v\
Shifts are applied in the order that they are \
specified on the command line. Only the first 50 \
shift operations are applied; any more are ignored.";

static struct argp_option options[] = {
  {"wrap",   'w', NULL,  0, "Wrap values (default: false)"},
  {"xshift", 'x', "INT", 0, "X axis voxel offset"},
  {"yshift", 'y', "INT", 0, "Y axis voxel offset"},
  {"zshift", 'z', "INT", 0, "Z axis voxel offset"},
  {0}
};

struct arguments {

  char   *input;
  char   *output;
  uint8_t wrap;
  uint8_t nshifts;
  uint8_t shiftdim[MAX_SHIFTS];
  int16_t shiftval[MAX_SHIFTS];
};

/**
 * argp callback function; parses a single argument value.
 */
static error_t _parse_opt (
  int                key, 
  char              *arg, 
  struct argp_state *state
);

/**
 * \return the number of values contained in a single 'slice' across the given
 * dimension.
 */
static uint32_t _calc_slicesize(
  dsr_t  *hdr, /**< image header       */
  uint8_t dim  /**< dimension to slice */
);

/**
 * Shifts the image along the given dimension, the given number of voxels. 
 */
static uint8_t _shift(
  dsr_t   *hdr,      /**< image header                                     */
  uint8_t *oldimage, /**< image                                            */
  uint8_t *newimage, /**< place to store shifted image                     */
  uint8_t  dim,      /**< dimension to shift along                         */
  int16_t  shift,    /**< number of voxels to shift (positive or negative) */
  uint8_t  wrap      /**< whether to wrap around.                          */
);

/**
 * Reads a single slice from the given image, along the given dimension.
 */
static void _readslice(
  dsr_t   *hdr,     /**< image header                  */
  uint8_t *image,   /**< image data                    */
  uint8_t  dim,     /**< dimension to slice            */
  uint8_t  sliceno, /**< slice number (index into dim) */
  double  *slice,   /**< place to store slice          */
  uint32_t slicesz  /**< number of values in a slice   */
);

/**
 * Writes a single slice to the given image, along the given dimension.
 */
static void _writeslice(
  dsr_t   *hdr,     /**< image header              */
  uint8_t *image,   /**< image data                */
  uint8_t  dim,     /**< dimension to write to     */
  uint8_t  sliceno, /**< slice number              */
  double  *slice,   /**< slice data                */
  uint32_t slicesz  /**< number of values in slice */
);

int main (int argc, char *argv[]) {

  uint8_t i;

  dsr_t            inhdr;
  uint8_t         *inimg;
  uint8_t         *outimg;
  uint8_t         *tmpimg;
  uint32_t         nvals;
  uint8_t          valsz;
  FILE            *fimg;
  FILE            *fhdr;
  struct arguments arguments;
  struct argp      argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  fimg   = NULL;
  fhdr   = NULL;
  inimg  = NULL;
  outimg = NULL;
  tmpimg = NULL;

  arguments.wrap    = 0;
  arguments.nshifts = 0;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (analyze_load(arguments.input, &inhdr, &inimg)) {
    printf("error reading %s\n", arguments.input);
    goto fail;
  }

  nvals = analyze_num_vals(  &inhdr);
  valsz = analyze_value_size(&inhdr);

  outimg = calloc(1, nvals*valsz);
  if (outimg == NULL){ 
    printf("out of memory (%u)!\n", nvals*valsz);
    goto fail;
  }

  for (i = 0; i < arguments.nshifts; i++) {

    if (arguments.shiftval[i] == 0) continue;

    memset(outimg, 0, nvals*valsz);
    if (_shift(&inhdr,
                inimg,
                outimg,
                arguments.shiftdim[i],
                arguments.shiftval[i],
                arguments.wrap))
      goto fail;

    tmpimg = inimg;
    inimg  = outimg;
    outimg = tmpimg;
  }
  memcpy(outimg, inimg, nvals*valsz);

  if (analyze_write_hdr(arguments.output, &inhdr))         goto fail;
  if (analyze_write_img(arguments.output, &inhdr, outimg)) goto fail;

  fclose(fimg);
  fclose(fhdr);
  free(inimg);
  free(outimg);
  return 0;  
fail:
  printf("fail?\n");
  if (fimg   != NULL) fclose(fimg);
  if (fhdr   != NULL) fclose(fhdr);
  if (inimg  != NULL) free(inimg);
  if (outimg != NULL) free(outimg);
  return 1;
}

error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  struct arguments *arguments = state->input;

  switch (key) {

    case 'w':
      arguments->wrap = 1;
      break;

    case 'x':

      if (arguments->nshifts == MAX_SHIFTS) break;
      arguments->shiftdim[arguments->nshifts] = 0;
      arguments->shiftval[arguments->nshifts] = atoi(arg);
      arguments->nshifts ++;
      break;

    case 'y':
      if (arguments->nshifts == MAX_SHIFTS) break;
      arguments->shiftdim[arguments->nshifts] = 1;
      arguments->shiftval[arguments->nshifts] = atoi(arg);
      arguments->nshifts ++;
      break;

    case 'z':
      if (arguments->nshifts == MAX_SHIFTS) break;
      arguments->shiftdim[arguments->nshifts] = 2;
      arguments->shiftval[arguments->nshifts] = atoi(arg);
      arguments->nshifts ++;
      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) arguments->input  = arg;
      else if (state->arg_num == 1) arguments->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 2) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

uint32_t _calc_slicesize(dsr_t *hdr, uint8_t dim) {

  uint8_t   i;
  uint8_t   ndims;
  uint32_t  slicesize;

  ndims   = analyze_num_dims(hdr);

  slicesize = 1;
  for (i = 0; i < ndims; i++) {

    if (i == dim) continue;
    slicesize *= analyze_dim_size(hdr, i);
  }

  return slicesize;
}

uint8_t _shift(
  dsr_t   *hdr, 
  uint8_t *oldimage, 
  uint8_t *newimage, 
  uint8_t  dim,
  int16_t  shift, 
  uint8_t  wrap)
{
  int64_t   i;
  int64_t   newi;
  double   *slice;
  uint8_t   valsz;
  uint32_t  dimsz;
  uint32_t  slicesz;

  slice = NULL;

  dimsz   = analyze_dim_size(  hdr, dim);
  valsz   = analyze_value_size(hdr);
  slicesz = _calc_slicesize(   hdr, dim);

  slice = malloc(slicesz * sizeof(double));
  if (slice == NULL) goto fail;

  for (i = 0; i < dimsz; i++) {

    newi = (i+shift) % dimsz;

    if (newi < 0) newi = dimsz + newi;

    if (!wrap) {

      if (shift > 0 && newi < i) continue;
      if (shift < 0 && newi > i) continue;
    }

    _readslice( hdr, oldimage, dim,    i, slice, slicesz);
    _writeslice(hdr, newimage, dim, newi, slice, slicesz);
  }

  free(slice);
  return 0;
fail:

  if (slice  != NULL) free(slice);
  return 1;
}

void _readslice(
  dsr_t   *hdr,
  uint8_t *image,
  uint8_t  dim,
  uint8_t  sliceno,
  double  *slice,
  uint32_t slicesz) {
  
  uint32_t i;
  uint8_t  j;
  uint8_t  ndims;
  uint32_t dimidx[4];

  ndims = analyze_num_dims(hdr);

  memset(dimidx, 0, 4 * sizeof(uint32_t));
  dimidx[dim] = sliceno;

  for (i = 0; i < slicesz; i++) {

    slice[i] = analyze_read_val(hdr, image, dimidx);

    for (j = 0; j < ndims; j++) {

      if (j == dim) continue;

      dimidx[j] = (dimidx[j]+1) % analyze_dim_size(hdr, j);
      if (dimidx[j] > 0) break;
    }
  }
}

void _writeslice(
  dsr_t   *hdr,
  uint8_t *image,
  uint8_t  dim,
  uint8_t  sliceno,
  double  *slice,
  uint32_t slicesz)
{
  uint32_t i;
  uint8_t  j;
  uint8_t  ndims;
  uint8_t  valsz;
  uint32_t dimidx[4];

  ndims = analyze_num_dims(hdr);
  valsz = analyze_value_size(hdr);

  memset(dimidx, 0, 4 * sizeof(uint32_t));
  dimidx[dim] = sliceno;

  for (i = 0; i < slicesz; i++) {

    analyze_write_val(hdr, image, dimidx, slice[i]);

    for (j = 0; j < ndims; j++) {

      if (j == dim) continue;

      dimidx[j] = (dimidx[j]+1) % analyze_dim_size(hdr, j);
      if (dimidx[j] > 0) break;
    }
  }
}
