/**
 * Program which crops an ANALYZE75 image.
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

static char doc[] = "cropimg -- extract part of a ANALYZE75 3D image file";

static struct argp_option options[] = {
  {"xlo", 'a', "INT", 0, "low X voxel limit"},
  {"xhi", 'b', "INT", 0, "high X voxel limit"},
  {"ylo", 'c', "INT", 0, "low Y voxel limit"},
  {"yhi", 'd', "INT", 0, "high Y voxel limit"},
  {"zlo", 'e', "INT", 0, "low Z voxel limit"},
  {"zhi", 'f', "INT", 0, "high Z voxel limit"},
  {0}
};

typedef struct _arguments {
  char    *input;
  char    *output;
  uint16_t xlo;
  uint16_t xhi;
  uint16_t ylo;
  uint16_t yhi;
  uint16_t zlo;
  uint16_t zhi;
} args_t;

/**
 * argp callback function; parses a single argument value.
 */
static error_t _parse_opt (
  int                key, 
  char              *arg, 
  struct argp_state *state
);

static void _crop_hdr(
  dsr_t  *inhdr,
  dsr_t  *outhdr,
  args_t *args
);

static void _crop_img(
  dsr_t   *inhdr,
  dsr_t   *outhdr,
  uint8_t *inimg,
  uint8_t *outimg,
  args_t  *args
);

int main (int argc, char *argv[]) {

  dsr_t       inhdr;
  dsr_t       outhdr;
  uint8_t    *inimg;
  uint8_t    *outimg;
  uint32_t    nbytes;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  inimg  = NULL;
  outimg = NULL;

  args.xlo = 0xFFFF;
  args.xhi = 0xFFFF;
  args.ylo = 0xFFFF;
  args.yhi = 0xFFFF;
  args.zlo = 0xFFFF;
  args.zhi = 0xFFFF;
  argp_parse(&argp, argc, argv, 0, 0, &args);

  if (analyze_load(args.input, &inhdr, &inimg)) {
    printf("error loading %s\n", args.input);
    goto fail;
  }

  if (args.xlo == 0xFFFF) args.xlo = 0;
  if (args.ylo == 0xFFFF) args.ylo = 0;
  if (args.zlo == 0xFFFF) args.zlo = 0;
  if (args.xhi == 0xFFFF) args.xhi = analyze_dim_size(&inhdr, 0);
  if (args.yhi == 0xFFFF) args.yhi = analyze_dim_size(&inhdr, 0);
  if (args.zhi == 0xFFFF) args.zhi = analyze_dim_size(&inhdr, 0);

  _crop_hdr(&inhdr, &outhdr, &args);

  nbytes = analyze_num_vals(&outhdr) * analyze_value_size(&outhdr);
  outimg = malloc(nbytes);
  if (outimg == NULL) goto fail;

  _crop_img(&inhdr, &outhdr, inimg, outimg, &args);

  if (analyze_write_hdr(args.output, &outhdr)) {
    printf("error writing header %s\n", args.output);
    goto fail;
  }

  if (analyze_write_img(args.output, &outhdr, outimg)) {
    printf("error writing image %s\n", args.output);
    goto fail;
  }

  free(inimg);
  free(outimg);

  return 0;
  
fail:
  if (inimg  != NULL) free(inimg);
  if (outimg != NULL) free(outimg);
  return 1;
}

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch(key) {

    case 'a': args->xlo = atoi(arg); break;
    case 'b': args->xhi = atoi(arg); break;
    case 'c': args->ylo = atoi(arg); break;
    case 'd': args->yhi = atoi(arg); break;
    case 'e': args->zlo = atoi(arg); break;
    case 'f': args->zhi = atoi(arg); break;
      
    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else                          argp_usage(state);
      break;
      
    case ARGP_KEY_END:
      if (state->arg_num < 2) argp_usage(state);
      break;
      
    default: return ARGP_ERR_UNKNOWN;

  }

  return 0;
}

static void _crop_hdr(dsr_t *inhdr, dsr_t *outhdr, args_t *args) {

  memcpy(outhdr, inhdr, sizeof(dsr_t));

  outhdr->dime.dim[1] = args->xhi - args->xlo;
  outhdr->dime.dim[2] = args->yhi - args->ylo;
  outhdr->dime.dim[3] = args->zhi - args->zlo;
}

static void _crop_img(
  dsr_t   *inhdr,
  dsr_t   *outhdr,
  uint8_t *inimg,
  uint8_t *outimg,
  args_t  *args
) {

  uint32_t xsz;
  uint32_t ysz;
  uint32_t zsz;
  uint32_t ini[4];
  uint32_t outi[4];
  double   val;

  memset(ini,  0, sizeof(ini));
  memset(outi, 0, sizeof(outi));

  xsz = analyze_dim_size(outhdr, 0);
  ysz = analyze_dim_size(outhdr, 1);
  zsz = analyze_dim_size(outhdr, 2);

  for (outi[0] = 0; outi[0] < xsz; outi[0]++) {
    for (outi[1] = 0; outi[1] < ysz; outi[1]++) {
      for (outi[2] = 0; outi[2] < zsz; outi[2]++) {

        ini[0] = outi[0] + args->xlo;
        ini[1] = outi[1] + args->ylo;
        ini[2] = outi[2] + args->zlo;

        val = analyze_read_val(inhdr,  inimg,  ini);
        analyze_write_val(     outhdr, outimg, outi, val);
      }
    }
  }
}
