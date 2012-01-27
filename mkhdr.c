/**
 * Program to make an ANALYZE75 header file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <argp.h>

#include "io/analyze75.h"
#include "util/startup.h"

typedef struct args {

  char    *output;
  uint16_t xn;
  uint16_t yn;
  uint16_t zn;
  double   xl;
  double   yl;
  double   zl;
  uint8_t  dt;
  uint8_t  rev;

} args_t;


static struct argp_option options[] = {

  {"xn",  'x', "INT",   0, "X dimension size"},
  {"yn",  'y', "INT",   0, "Y dimension size"},
  {"zn",  'z', "INT",   0, "Z dimension size"},
  {"xl",  'a', "FLOAT", 0, "X voxel length"},
  {"yl",  'b', "FLOAT", 0, "Y voxel length"},
  {"zl",  'c', "FLOAT", 0, "Z voxel length"},
  {"dt",  'd', "INT",   0, "Data type"},
  {"rev", 'r', NULL,    0, "reverse endianness"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch (key) {
    case 'x': a->xn  = atoi(arg); break;
    case 'y': a->yn  = atoi(arg); break;
    case 'z': a->zn  = atoi(arg); break;
    case 'a': a->xl  = atof(arg); break;
    case 'b': a->yl  = atof(arg); break;
    case 'c': a->zl  = atof(arg); break;
    case 'd': a->dt  = atoi(arg); break;
    case 'r': a->rev = 1;         break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:  
      if (state->arg_num < 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static char doc[] = "mkhdr - make an ANALYZE75 header file";


static uint8_t _fill_hdr(
  dsr_t  *dsr,
  args_t *args
);

int main (int argc, char *argv[]) {

  FILE       *hd;
  args_t      args;
  dsr_t       hdr;
  struct argp argp = {options, _parse_opt, "OUTUPT", doc};

  hd = NULL;  

  memset(&args, 0, sizeof(args_t));
  startup("mkhdr", argc, argv, &argp, &args);

  if (_fill_hdr(&hdr, &args)) {
    printf("There's a problem with the input arguments\n");
    goto fail;
  }

  if (analyze_write_hdr(args.output, &hdr)) {
    printf("Error writing file: %s\n", args.output);
    goto fail;
  }

  fclose(hd);

  return 0;

fail:
  if (hd != NULL) fclose(hd);
  return 1;
}

static uint8_t _fill_hdr(dsr_t *hdr, args_t *args) {

  memset(hdr, 0, sizeof(dsr_t));

  hdr->hk.sizeof_hdr = 348;

  hdr->dime.dim[0] = 3;
  hdr->dime.dim[1] = args->xn;
  hdr->dime.dim[2] = args->yn;
  hdr->dime.dim[3] = args->zn;
  hdr->dime.dim[4] = 1;
  hdr->dime.dim[5] = 1;
  hdr->dime.dim[6] = 1;
  hdr->dime.dim[7] = 1;
  hdr->rev         = (args->rev > 0) ? 1 : 0;

  switch (args->dt) {
    case DT_UNSIGNED_CHAR: hdr->dime.bitpix = 8;  break;
    case DT_SIGNED_SHORT:  hdr->dime.bitpix = 16; break;
    case DT_SIGNED_INT:    hdr->dime.bitpix = 32; break;
    case DT_FLOAT:         hdr->dime.bitpix = 32; break;
    case DT_DOUBLE:        hdr->dime.bitpix = 64; break;
    default:               goto fail;
  }

  hdr->dime.datatype  = args->dt;

  hdr->dime.pixdim[0] = 0.0;
  hdr->dime.pixdim[1] = args->xl;
  hdr->dime.pixdim[2] = args->yl;
  hdr->dime.pixdim[3] = args->zl;

  return 0;

fail:
  return 1;
}
