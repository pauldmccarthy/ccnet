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
#include "graph/graph_log.h"
#include "io/ngdb_graph.h"
#include "util/startup.h"

static char doc[] = "cprune - remove disconnected nodes/components"\
                    "from a graph.\v"\
                    "Listen carefully.\n\n"\
                    "If size is > 0, the prop parameter is ignored, "\
                    "and the graph is pruned such that only those "\
                    "components which are larger than size remain. "\
                    "If size is 0 and prop is 0.0, the graph is pruned "\
                    "such that only the largest component remains - if "\
                    "multiple components have equal largest size, they "\
                    "are all retained.\n\n" \
                    "If size is 0 and prop is > 0.0, the graph is pruned "\
                    "such that only the largest component remains, but "\
                    "only if that component is (prop*100)% of the total "\
                    "network size. If this is not the case, the graph is "\
                    "pruned as if (size=1,prop=0.0) were passed in - i.e."\
                    "only disconnected nodes are pruned.\n\n" \
                    "If a size is not specified, or set to 0, the graph is "\
                    "pruned such that only the largest component remains.";

/**
 * input arguments
 */
typedef struct args {
  char    *input;
  char    *output;
  char    *hdrmsg;
  uint32_t size;
  double   prop;
} args_t;

static struct argp_option options[] = {
  {"size",   's', "INT",    0, "remove components below this size"},
  {"prop",   'p', "DOUBLE", 0, "minimum proportion of largest component"},
  {"hdrmsg", 'h', "MSG",    0, "message to save to .ngdb file header"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {
    case 's': a->size   = atoi(arg); break;
    case 'p': a->prop   = atof(arg); break;
    case 'h': a->hdrmsg = arg;       break;
      
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
  startup("cprune", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  if (graph_prune(&gin, &gout, args.size, args.prop)) {
    printf("Graph prune failed\n");
    goto fail;
  }

  if (graph_log_copy(&gin, &gout)) {
    printf("Error copying graph log\n");
    goto fail;
  }

  if (args.hdrmsg != NULL) {
    if (graph_log_add(&gout, args.hdrmsg)) {
      printf("Error adding header message\n");
      goto fail;
    }
  }

  if (ngdb_write(&gout, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}
