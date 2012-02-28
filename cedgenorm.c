/**
 * Normalise the edge weights of a graph so that they lie in a specified
 * range.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>

#include "util/startup.h"
#include "graph/graph.h"
#include "io/ngdb_graph.h"

static char doc [] = "cedgenorm - normalise edge weights of a ngdb "\
                     "graph file";

typedef struct _args {

  char  *input;
  char  *output;
  double newlo;
  double newhi;

} args_t;

static struct argp_option options[] = {
  {"lo", 'l', "FILE", 0, "new low (minimum) edge weight value"},
  {"hi", 'h', "FILE", 0, "new high (maximum) edge weight value "},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {

    case 'l': a->newlo = atof(arg); break;
    case 'h': a->newhi = atof(arg); break;

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

/**
 * Finds the minimum and maximum edge weight values in the given graph.
 */
static void _find_minmax(
  graph_t *g,   /**< the graph */
  double  *min, /**< place to store minimum edge weight */
  double  *max  /**< place to store maximum edge weight */
);

/**
 * Rescales all of the edge weights in the given graph from the old range
 * (oldlo,oldhi) to the new range (newlo,newhi).
 */
static void _normalise_edge_weights(
  graph_t *g,     /**< the graph         */
  double   oldlo, /**< old minimum value */
  double   oldhi, /**< old maximum value */
  double   newlo, /**< new minimum value */
  double   newhi  /**< new maximum value */
);

int main (int argc, char *argv[]) {

  graph_t     g;
  args_t      args;
  double      oldlo;
  double      oldhi;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));

  startup("cedgenorm", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error loading ngdb file %s\n", args.input);
    goto fail;
  }

  _find_minmax(           &g, &oldlo, &oldhi);
  _normalise_edge_weights(&g,  oldlo,  oldhi, args.newlo, args.newhi);

  if (ngdb_write(&g, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
   } 
  
  return 0;

fail:
  return 1;
}

void _find_minmax(graph_t *g, double *min, double *max) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  float    *wts;

  *min =  DBL_MAX;
  *max = -DBL_MAX;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    wts   = graph_get_weights   (g, i);

    for (j = 0; j < nnbrs; j++) {

      if (wts[j] < *min) *min = wts[j];
      if (wts[j] > *max) *max = wts[j];
    }
  }
}

void _normalise_edge_weights(
  graph_t *g, double oldlo, double oldhi, double newlo, double newhi) {


  uint64_t i;
  uint64_t j;
  uint32_t nnodes;
  uint32_t nnbrs;
  double   scale;
  float   *wts;

  scale  = (newhi - newlo) / (oldhi - oldlo);

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    
    nnbrs = graph_num_neighbours(g, i);
    wts   = graph_get_weights   (g, i);

    for (j = 0; j < nnbrs; j++)
      wts[j] = (wts[j] - oldlo) * scale + newlo;
  }
}
