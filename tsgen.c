/**
 * Program which Generates random time series data, and saves it to an
 * ANALYZE75 volume.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <math.h>
#include <argp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "io/analyze75.h"
#include "util/startup.h"

typedef struct _args {

  char    *output;
  uint16_t xn;
  uint16_t yn;
  uint16_t zn;
  uint16_t tn;
  double   xl;
  double   yl;
  double   zl;
  uint16_t ts;
  uint16_t dt;
  double   lo;
  double   hi;
  
} args_t;

static char doc[] =
  "tsgen -- generate random time series, save to ANALYZE75 format\v\
Supported data type formats:\n\
  2  - unsigned char (1 byte)\n\
  4  - signed short  (2 bytes)\n\
  8  - signed int    (4 bytes)\n\
  16 - float         (4 bytes)\n\
  64 - double        (8 bytes)\n\
";

static struct argp_option options[] = {
  {"xn", 'a', "INT",   0, "number of voxels along x axis"},
  {"yn", 'b', "INT",   0, "number of voxels along y axis"},
  {"zn", 'c', "INT",   0, "number of voxels along z axis"},
  {"tn", 'd', "INT",   0, "length of time series"},
  {"xl", 'e', "FLOAT", 0, "length of one voxel along x axis"},
  {"yl", 'f', "FLOAT", 0, "length of one voxel along y axis"},
  {"zl", 'g', "FLOAT", 0, "length of one voxel along z axis"},
  {"ts", 's', "INT",   0, "start number for first image"},
  {"dt", 't', "INT",   0, "data type"},
  {"lo", 'l', "FLOAT", 0, "minimum value"},
  {"hi", 'h', "FLOAT", 0, "maximum value"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;
  args = state->input;

  switch (key) {
    case 'a': args->xn = atoi(arg); break;
    case 'b': args->yn = atoi(arg); break;
    case 'c': args->zn = atoi(arg); break;
    case 'd': args->tn = atoi(arg); break;
    case 'e': args->xl = atof(arg); break;
    case 'f': args->yl = atof(arg); break;
    case 'g': args->zl = atof(arg); break;
    case 's': args->ts = atoi(arg); break;
    case 't': args->dt = atoi(arg); break;
    case 'l': args->lo = atof(arg); break;
    case 'h': args->hi = atof(arg); break;
      
    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break;      

    default:
      return ARGP_ERR_UNKNOWN; 
  }

  return 0;
}

/**
 * Creates a file name for the given image.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _file_name(
  uint16_t tn,  /**< total number of values in time series */
  uint16_t ti,  /**< number of current value               */
  char    *name /**< place to put file name                */
);

/**
 * Creates one image, according to the arguments.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _create_image(
  dsr_t    *hdr, /**< header to populate                                    */
  uint8_t **img, /**< pointer which will be updated to point to a new image */
  args_t   *args /**< program arguments                                     */
);

int main(int argc, char *argv[]) {

  uint32_t    i;
  dsr_t       hdr;
  uint8_t    *img;
  char        fname[20];
  args_t      args;
  struct argp argp = {options, _parse_opt, "OUTDIR", doc};

  memset(&args, 0, sizeof(args));

  startup("tsgen", argc, argv, &argp, &args);

  for (i = 0; i < args.tn; i++) {

    if (_file_name(args.tn+args.ts, i+args.ts, fname)) {
      printf("error generating file name (series too long?)\n");
      goto fail;
    }

    if (_create_image(&hdr, &img, &args)) {
      printf("error creating image (%s)\n", fname);
      goto fail;
    }

    if (analyze_write_hdr(fname, &hdr)) {
      printf("error writing header (%s)\n", fname);
      goto fail;
    }

    if (analyze_write_img(fname, &hdr, img)) {
      printf("error writing image (%s)\n", fname);
      goto fail;
    } 

    free(img);
    img = NULL;
    memset(&hdr, 0, sizeof(dsr_t));
  }

  return 0;

fail:
  return 1;
}

uint8_t _file_name(uint16_t tn, uint16_t ti, char *name) {

  uint8_t fmtlen;
  char    fmtstr[10];

  fmtlen = ((int)log10(tn)) + 1;

  if (fmtlen > 9) return 1;

  sprintf(fmtstr, "%%%02uu", fmtlen);
  sprintf(name,   fmtstr,    ti);

  return 0;
}

uint8_t _create_image(dsr_t *hdr, uint8_t **img, args_t *args) {

  return 0;
}
