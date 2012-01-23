/**
 * Program which can convert an ANALYZE75 image to a different data type.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>

#include <argp.h>
#include <errno.h>

#include "io/analyze75.h"

/**
 * argp callback function; parses a single argument value.
 */
static error_t _parse_opt (
  int                key, 
  char              *arg, 
  struct argp_state *state
);

/**
 * \return 0 if the given format code is valid, non-0 otherwise.
 */
static uint8_t _check_format(uint16_t fmt);

static uint8_t _clone_img(
  uint8_t  *oldimg, /**< old image data                */
  uint8_t **newimg, /**< pointer which will be updated
                         to point to new image data    */
  dsr_t    *oldhdr, /**< old header                    */
  dsr_t    *newhdr, /**< new header                    */
  uint16_t  newfmt  /**< new data type                 */
);

static char doc[] = "cnvimg -- convert ANALYZE75 image files\v\
Supported formats:\n\
  2  - unsigned char (1 byte)\n\
  4  - signed short  (2 bytes)\n\
  8  - signed int    (4 bytes)\n\
  16 - float         (4 bytes)\n\
  64 - double        (8 bytes)\n\
";

static struct argp_option options[] = {
  {"format", 'f', "INT",  0, "output image format (default: 16 - DT_FLOAT)"},
  {0}
};

struct arguments {
  char    *input;
  char    *output;
  uint16_t format;
};

int main (int argc, char *argv[]) {

  struct arguments args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};
  
  dsr_t    oldhdr;
  dsr_t    newhdr;
  uint8_t *oldimg;
  uint8_t *newimg;  

  oldimg      = NULL;
  newimg      = NULL;
  args.format = 16;

  argp_parse(&argp, argc, argv, 0, 0, &args);

  /*
   * 1. read header and image
   * 2. clone header with new data type
   * 3. write new header out
   * 4. write new image out
   */

  if (analyze_load(args.input, &oldhdr, &oldimg)) {
    printf("error reading header (%i)\n", errno);
    goto fail;
  }

  if (_clone_img(oldimg,&newimg, &oldhdr, &newhdr, args.format)) {
    printf("error creating new image\n");
    goto fail;
  }

  if (analyze_write_hdr(args.output, &newhdr)) {
    printf("error writing new header\n");
    goto fail;
  }

  if (analyze_write_img(args.output, &newhdr, newimg)) {
    printf("error writing new image\n");
    goto fail;
  }

  free(oldimg);
  free(newimg);
  return 0;

fail:
  if (oldimg != NULL) free(oldimg);
  if (newimg != NULL) free(newimg);
  return 1;
}

error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  struct arguments *arguments = state->input;

  switch (key) {

    case 'f':
      arguments->format = atoi(arg);
      if (_check_format(arguments->format)) argp_usage(state);
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

uint8_t _check_format(uint16_t fmt) {

  switch (fmt) {
    case DT_UNSIGNED_CHAR: return 0;
    case DT_SIGNED_SHORT:  return 0;
    case DT_SIGNED_INT:    return 0;
    case DT_FLOAT:         return 0;
    case DT_DOUBLE:        return 0;
    default:               return 1;
  }
}

uint8_t _clone_img(
  uint8_t  *oldimg,
  uint8_t **newimg,
  dsr_t    *oldhdr,
  dsr_t    *newhdr,
  uint16_t  newfmt)
{
  uint64_t i;
  uint32_t nvals;
  uint8_t  newvalsz;
  uint8_t *lnewimg;
  double   val;

  nvals    = analyze_num_vals(  oldhdr);
  newvalsz = analyze_datatype_size(newfmt);
  lnewimg  = malloc(nvals*newvalsz);
  if (lnewimg == NULL) goto fail;  

  memcpy(newhdr, oldhdr, sizeof(dsr_t));
  newhdr->dime.datatype = newfmt;
  newhdr->dime.bitpix   = newvalsz * 8;

  for (i = 0; i < nvals; i++) {
    val = analyze_read_by_idx(oldhdr, oldimg,  i);
    analyze_write_by_idx(     newhdr, lnewimg, i, val);
  }

  *newimg = lnewimg;
  return 0;

fail:
  if (lnewimg != NULL) free(lnewimg);
  return 1;
}
