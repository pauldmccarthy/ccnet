/**
 * Average a collection of matrix files.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "util/startup.h"
#include "util/copyfile.h"
#include "io/mat.h"

#define MAX_INPUTS 50

typedef struct _args {
  
  char   *inputs[MAX_INPUTS];
  char   *output;
  uint8_t ninputs;
 
} args_t ;


static char doc[] = "avgmat -- create an average matrix from "\
                    "a collection of input matrix files";

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {

    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->output = arg;
      else {
        
        if (args->ninputs < MAX_INPUTS) args->inputs[args->ninputs++] = arg;
        else printf("too many inputs - ignoring %s\n", arg);
      }
        
      break;

    case ARGP_KEY_END:
      if (state->arg_num <= 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN; 
  }
  
  return 0;
}

static mat_t * _create_outmat(
  char    *outf,
  mat_t  **inmats,
  uint8_t  ninputs
);

static uint8_t _agg_matrix(
  mat_t  **inmats,
  mat_t   *outmat,
  uint8_t  ninputs
);

int main(int argc, char *argv[]) {

  uint16_t    i;
  mat_t      *inmats[MAX_INPUTS];
  mat_t      *outmat;
  args_t      args;
  struct argp argp = {NULL, _parse_opt, "OUTPUT [INPUT ...]", doc};
  
  outmat = NULL;

  memset(inmats, 0, sizeof(inmats));
  memset(&args,  0, sizeof(args));

  startup("avgmat", argc, argv, &argp, &args);

  if (args.ninputs == 0) {
    printf("at least one input is required\n");
    goto fail;
  }

  for (i = 0; i < args.ninputs; i++) {
    
    inmats[i] = mat_open(args.inputs[i]);
    
    if (inmats[i] == NULL) {
      printf("could not open input file %s\n", args.inputs[i]);
      goto fail;
    }    
  }

  outmat = _create_outmat(args.output, inmats, args.ninputs);
  if (outmat == NULL) {
    printf("could not create output matrix %s\n", args.output);
    goto fail;
  }

  if (_agg_matrix(inmats, outmat, args.ninputs)) {
    printf("could not aggregate matrix\n");
    goto fail;
  }

  for (i = 0; i < args.ninputs; i++) 
    mat_close(inmats[i]);

  mat_close(outmat);

  return 0;

fail:

  for (i = 0; i < args.ninputs; i++) {
      if (inmats[i] != NULL)
        mat_close(inmats[i]);
  }

  if (outmat != NULL) mat_close(outmat);
  return 1;
}

mat_t * _create_outmat(char *outf, mat_t **inmats, uint8_t ninputs) {

  mat_t *outmat;

  outmat = mat_create(
    outf,
    mat_num_rows(     inmats[0]),
    mat_num_cols(     inmats[0]),
    mat_get_flags(    inmats[0]),
    mat_hdr_data_size(inmats[0]),
    mat_label_size(   inmats[0]));
                      
  return outmat;
}


uint8_t _agg_matrix(mat_t **inmats, mat_t *outmat, uint8_t ninputs) {
  
  uint16_t i;
  uint64_t rowi;
  uint64_t coli;
  uint64_t nrows;
  uint64_t ncols;
  double  *inrowbuf;
  double  *outrowbuf;

  inrowbuf  = NULL;
  outrowbuf = NULL;

  nrows  = mat_num_rows(outmat);
  ncols  = mat_num_cols(outmat);
  
  inrowbuf = malloc(ncols*sizeof(double));
  if (inrowbuf == NULL) goto fail;
  
  outrowbuf = malloc(ncols*sizeof(double));
  if (outrowbuf == NULL) goto fail;

  for (rowi = 0; rowi < nrows; rowi++) {
    
    memset(outrowbuf, 0, ncols*sizeof(double));

    for (i = 0; i < ninputs; i++) {

      if (mat_read_row(inmats[i], rowi, inrowbuf))
        goto fail;

      for (coli = 0; coli < ncols; coli++)
        outrowbuf[coli] += inrowbuf[coli] / ninputs;
    }

    if (mat_write_row(outmat, rowi, outrowbuf))
      goto fail; 
  }

  free(inrowbuf);
  free(outrowbuf);
  return 0;
  
fail:

  if (inrowbuf  != NULL) free(inrowbuf);
  if (outrowbuf != NULL) free(outrowbuf);
  return 1;
}
