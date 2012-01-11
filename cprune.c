/**
 * Program to remove disconnected nodes/components from a graph.
 *
 * This program creates a new graph from an input graph, removing any
 * disconnected nodes, or disconnected components, which are below a certain
 * size.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include "graph/graph.h"
#include "graph/graph_prune.h"
#include "io/ngdb_graph.h"
#include "util/startup.h"

static char doc[] = "cprune - remove disconnected nodes/components "\
                    "from a graph";

/**
 * input arguments
 */
typedef struct args {
  char    *input;
  char    *output;
  uint32_t size;
} args_t;

static struct argp_option options[] = {
  {"size", 's', "INT", 0, "remove components below this size"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {
    case 's': a->size = atoi(arg); break;
      
    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->input  = arg;
      else if (state->arg_num == 1) a->output = arg;
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

int main(int argc, char *argv[]) {

  graph_t gin;
  graph_t gout;

  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));
  args.size = 1; 
  startup("cprune", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  if (graph_prune(&gin, &gout, args.size)) {
    printf("Graph prune failed\n");
    goto fail;
  }

  if (ngdb_write(&gout, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}
