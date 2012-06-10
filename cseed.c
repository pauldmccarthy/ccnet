/**
 * Extract a subgraph from a seed node, by breadth-first searching out from
 * the seed a specified depth.
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

static char doc[] = "cseed -- extract a subgraph from a specified seed node ";

static struct argp_option opts[] = {
  {"x",     'x', "FLOAT", 0, "X coordinate of seed node"},
  {"y",     'y', "FLOAT", 0, "Y coordinate of seed node"},
  {"z",     'z', "FLOAT", 0, "Z coordinate of seed node"},
  {"maxdeg",'m',  NULL,   0, "Use node with maximum degree as seed node"},
  {"nodeid",'n', "INT",   0, "ID of seed node"},
  {"depth", 'd', "INT",   0, "Depth to extract"},
  {0}
};

typedef struct _args {

  char    *input;
  char    *output;
  float    x;
  float    y;
  float    z;
  uint8_t  usecds;
  uint32_t n;
  uint8_t  usen;
  uint8_t  maxdeg;
  uint8_t  depth;
  
} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch (key) {

    case 'x': args->x      = atof(arg); args->usecds = 1; break;
    case 'y': args->y      = atof(arg); args->usecds = 1; break;
    case 'z': args->z      = atof(arg); args->usecds = 1; break;
    case 'n': args->n      = atoi(arg); args->usen   = 1; break;
    case 'd': args->depth  = atoi(arg);                   break;
    case 'm': args->maxdeg = 1;                           break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
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

static int64_t _get_seed_node(
  args_t  *args,
  graph_t *gin
);

int main (int argc, char *argv[]) {


  graph_t     gin;
  graph_t     gout;
  int64_t     seed;
  struct argp argp = {opts, _parse_opt, "INPUT OUTPUT", doc};
  args_t      args;  

  memset(&args, 0, sizeof(args_t));
  args.depth = 1;

  startup("cseed", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  seed = _get_seed_node(&args, &gin);

  if (seed < 0) {
    printf("Seed node incorrectly specified\n");
    goto fail;
  }

  if (graph_seed(&gin, &gout, seed, args.depth)) {
    printf("Error creating seed subgraph\n");
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

int64_t _get_seed_node(args_t *args, graph_t *gin) {

  uint64_t       i;
  uint32_t       nnodes;
  uint32_t       thisdeg;
  uint32_t       maxdeg;
  uint32_t       maxdegi;
  graph_label_t *lbl;

  nnodes  = graph_num_nodes(gin);
  thisdeg = 0;
  maxdeg  = 0;
  maxdegi = 0;
  
  /* node id used to specify seed */
  if (args->usen) return args->n;

  /* xyz coordinates used to specify seed */
  else if (args->usecds) {

    for (i = 0; i < nnodes; i++) {
      
      lbl = graph_get_nodelabel(gin, i);

      if ( lbl->xval == args->x
        && lbl->yval == args->y
        && lbl->zval == args->z) {
        return i;
      }
    }
    
    return -1;
  }

  /* node with maximum degree used as seed */
  else if (args->maxdeg) {
    
    for (i = 0; i < nnodes; i++) {
      
      thisdeg = graph_num_neighbours(gin, i);
      if (thisdeg > maxdeg) {
        maxdeg  = thisdeg;
        maxdegi = i;
      }
    }
    
    return maxdegi;
  }

  /*user did not specify seed node */
  else return -1;
}
