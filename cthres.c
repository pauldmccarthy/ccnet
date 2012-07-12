/**
 * Threshold the edges of a weighted ngdb file.
 *
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <argp.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "graph/graph.h"
#include "graph/graph_threshold.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"

typedef struct _args {
  char   *input;
  char   *output;
  double  threshold;
  uint8_t absval;
  uint8_t reverse;
} args_t;

static char doc[] = "cthres -- threshold the edges of a weighted ngdb file";

static struct argp_option options[] = {
  {"threshold", 't', "DOUBLE", 0, "edge threshold value"},
  {"absval",    'a',  NULL,    0, "threshold at absolute value"},
  {"reverse",   'r',  NULL,    0, "remove edges below the "\
                                  "threshold, rather than above"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {
    
    case 't': args->threshold = atof(arg); break;
    case 'a': args->absval    = 1;         break;
    case 'r': args->reverse   = 1;         break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 2) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN; 
  }
  
  return 0;
}

int main(int argc, char *argv[]) {

  graph_t     gin;
  graph_t     gout;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args));

  startup("cthres", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("error openineg input file %s\n", args.input);
    goto fail;
  }

  if (graph_threshold_weight(
        &gin, &gout, args.threshold, args.absval, args.reverse)) {
    printf("error thresholding graph\n");
    goto fail;
  }

  if (ngdb_write(&gout, args.output)) {
    printf("error writing to output file %s\n", args.output);
    goto fail;
  }
  

  return 0;
  
fail:
  return 1;
}
