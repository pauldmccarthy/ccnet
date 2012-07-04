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
  uint8_t sum;
 
} args_t ;

static struct argp_option options[] = {
  {"sum", 's', NULL, 0, "store sum of inputs, rather than average"},
  {0}
};

static char doc[] = "avgmat -- create an average matrix from "\
                    "a collection of input matrix files";

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {

    case 's': args->sum = 1; break; 

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

static uint8_t _add_input(mat_t *inmat, mat_t *outmat, uint8_t ninputs);

int main(int argc, char *argv[]) {

  uint16_t    i;
  mat_t      *inmat;
  mat_t      *outmat;
  args_t      args;
  struct argp argp = {options, _parse_opt, "OUTPUT [INPUT ...]", doc};

  inmat  = NULL;
  outmat = NULL;

  memset(&args, 0, sizeof(args));

  startup("avgmat", argc, argv, &argp, &args);

  if (args.ninputs == 0) {
    printf("at least one input is required\n");
    goto fail;
  }

  if (copyfile(args.inputs[0], args.output)) {
    printf("could not create output file from first input %s\n",
           args.inputs[0]);
    goto fail;
  }

  outmat = mat_open(args.output);
  if (outmat == NULL) goto fail;

  for (i = 1; i < args.ninputs; i++) {

    inmat = mat_open(args.inputs[i]);
    if (inmat == NULL) {
      printf("could not open input file %s\n", args.inputs[i]);
      goto fail;
    }

    if (_add_input(inmat, outmat, args.ninputs)) {
      printf("could not aggregate input file %s\n", args.inputs[i]);
      goto fail;
    }

    mat_close(inmat);
  }

  mat_close(outmat);

  return 0;

fail:

  if (inmat  != NULL) mat_close(inmat);
  if (outmat != NULL) mat_close(outmat);
  return 1;
}


uint8_t _add_input(mat_t *inmat, mat_t *outmat, uint8_t ninputs) {

  return 0;
}
