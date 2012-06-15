/**
 * Extract the 'components' from a graph, by iteratively extracting
 * a 'seeded subgraph', with the maximum degree node as the seed.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <argp.h>

#include "graph/graph.h"
#include "graph/graph_seed.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"

static char doc[] = "cseed -- extract a subgraph from a specified seed node";

static struct argp_option opts[] = {
  {"maxcmps", 'm',  "INT",    0, "maximum number of components to extract"},
  {"depth",   'd',  "INT",    0, "subgraph extraction depth"},
  {0}
};

typedef struct _args {

  char    *input;
  char    *outpref;
  uint8_t  depth;
  uint32_t maxcmps;
  
} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch (key) {

    case 'm': args->maxcmps = atoi(arg); break;
    case 'd': args->depth   = atoi(arg); break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input   = arg;
      else if (state->arg_num == 1) args->outpref = arg;
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

static uint32_t _get_seed_node(
  graph_t *g
);

int main (int argc, char *argv[]) {

  uint64_t    i;
  uint32_t    seed;
  graph_t     gin;
  graph_t     grem;
  graph_t     gout;
  struct argp argp = {opts, _parse_opt, "INPUT OUTPREF", doc};
  args_t      args;
  char        fname[1024];

  memset(&args, 0, sizeof(args_t));
  args.depth   = 1;
  args.maxcmps = 10;

  startup("callseed", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  for (i = 0; i < args.maxcmps; i++) {

    if (graph_num_nodes(&gin) == 0)
      break;

    seed = _get_seed_node(&gin);

    if (graph_seed(&gin,&gout, &seed, 1, args.depth, &grem)) {
      printf("Error creating seed subgraph\n");
      goto fail;
    }

    sprintf(fname, "%s_%02u.ngdb", args.outpref, (uint32_t)i);

    printf("Seeded subgraph %u (%u nodes): %s\n",
           (uint32_t)i, graph_num_nodes(&gout), fname);

    if (ngdb_write(&gout, fname)) {
      printf("Could not write to %s\n", fname);
      goto fail;
    }

    graph_free(&gin);
    graph_free(&gout);
    memcpy(&gin, &grem, sizeof(graph_t));
  }

  return 0;
  
fail:
  return 1;
}

uint32_t _get_seed_node(graph_t *g) {

  uint64_t       i;
  uint32_t       nnodes;
  uint32_t       thisdeg;
  uint32_t       maxdeg;
  uint32_t       maxdegi;

  nnodes  = graph_num_nodes(g);
  thisdeg = 0;
  maxdeg  = 0;
  maxdegi = 0;
  
  for (i = 0; i < nnodes; i++) {
     
    thisdeg = graph_num_neighbours(g, i);
    if (thisdeg > maxdeg) {
      maxdeg  = thisdeg;
      maxdegi = i;
    }
  }

  return maxdegi;
}
