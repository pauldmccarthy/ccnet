/**
 * Replaces one or more values in an ANALYZE75 image.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "io/analyze75.h"
#include "util/startup.h"

#define MAX_REPLACEMENTS 50

typedef struct _args {

  char    *input;
  char    *output;
  double   from[MAX_REPLACEMENTS];
  double   to  [MAX_REPLACEMENTS];
  uint16_t nreps;

} args_t;

static char *doc = "repimg -- replace values in an ANALYZE75 image";

static struct argp_option options[] = {
  {"rep", 'r', "FLOAT,FLOAT", 0, "replacement (from,to)"},
  {0}
};

static void _parse_rep(char *rep, args_t *args) {

  double  from;
  double  to;
  char   *toptr;

  if (args->nreps >= MAX_REPLACEMENTS);

  toptr = strchr(rep, ',');

  if (toptr         == NULL) return;
  if (strlen(toptr) == 0)    return;

  from = atof(rep);
  to   = atof(toptr+1);

  args->from[args->nreps] = from;
  args->to  [args->nreps] = to;
  args->nreps++;
}

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {
  
  args_t *args = state->input;

  switch (key) {

    case 'r': _parse_rep(arg, args); break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 2) argp_usage(state);
      break;

    default: return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

int main(int argc, char *argv[]) {

  dsr_t       hdr;
  uint8_t    *img;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  startup("repimg", argc, argv, &argp, &args);

  if (analyze_load(args.input, &hdr, &img)) {
    printf("error reading file %s\n", args.input);
    goto fail;
  }

  

  if (analyze_write_hdr(args.output, &hdr)) {
    printf("error writing header %s\n", args.output);
    goto fail;
  }

  if (analyze_write_img(args.output, &hdr, img)) {
    printf("error writing image %s\n", args.output);
    goto fail;
  }

  return 0;
  
fail:
  return 1;
}
