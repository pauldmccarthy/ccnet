/**
 * Program which removes edges from a graph, based on a given criteria.
 *
 * This program creates a new graph from an input graph, removing edges based
 * on one of the following criteria:
 *   - path-sharing
 *   - edge-betweenness
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include "graph/graph.h"
#include "graph/graph_threshold.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"

static char doc[] = "ctrim - remove edges from a graph "\
                    "based on a given criteria";

typedef enum {
  
  C_PATHSHARING     = 1,
  C_EDGEBETWEENNESS = 2,
  
} criteria_t;

/**
 * input arguments
 */
typedef struct args {
  
  char      *input;
  char      *output;
  criteria_t criteria;
  uint8_t    modularity;
  uint8_t    printmod;
  double     threshold;
  uint32_t   nedges;
  uint32_t   cmplimit;
  uint32_t   igndis;
  
} args_t;

static struct argp_option options[] = {
  {"nedges",     'n', "INT",    0, "number of edges to remove"},
  {"cmplimit",   'm', "INT",    0, "continue removing edges until the "\
                                   "graph splits into this many components"},
  {"modularity", 'o', NULL,     0, "output the graph with the maximum " \
                                   "modularity"},
  {"printmod",   'p', NULL ,    0, "print modularity and number of " \
                                   "components for each iteration"},
  {"criteria",   'c', "STRING", 0, "name of the criteria on which "\
                                   "to remove edges "},
  {"igndis",     'd', "INT",    OPTION_ARG_OPTIONAL,
                                   "do not class components this size or "\
                                   "smaller as components (default 1)"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {

    case 'n': a->nedges     = atoi(arg); break;
    case 'm': a->cmplimit   = atoi(arg); break;
    case 'o': a->modularity = 0xFF;      break;
    case 'p': a->printmod   = 0xFF;      break;
    case 'd':
      if (arg != NULL) a->igndis = atoi(arg);
      else             a->igndis = 1;
      break;
    case 'c':
      
      if (!strcmp(arg, "pathsharing") || !strcmp(arg, "ps"))
        a->criteria = C_PATHSHARING;
      else if (!strcmp(arg, "edgebetweenness") || !strcmp(arg,"eb"))
        a->criteria = C_EDGEBETWEENNESS;
      else argp_usage(state);
      
      break;
      
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

static uint8_t _trim(
  graph_t   *gin,
  graph_t   *gout,
  args_t    *a
);

int main(int argc, char *argv[]) {

  graph_t gin;
  graph_t gout;

  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));
  startup("ctrim", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {

    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  stats_cache_init(&gin);

  if (_trim(&gin, &gout, &args)) {
    printf("Could not trim graph\n");
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

static uint8_t _trim(graph_t *gin, graph_t *gout, args_t *a) {

  uint64_t  i;
  uint32_t  val;
  uint32_t  flags;
  uint32_t  oldcmp;
  void     *opt;
  mod_opt_t modopt;
  uint8_t (*tfunc)(
    graph_t  *gin, graph_t *gout, uint32_t cmplimit, 
    uint32_t igndis, void *opt,
    uint8_t (*init)(graph_t *g),
    uint8_t (*remove)(
      graph_t *g, double *space, array_t *edges, graph_edge_t *edge),
    uint8_t (*recalc)(graph_t *g, graph_edge_t *edge));

  flags = 0;

  if (a->modularity) {
    tfunc = &graph_threshold_modularity;
    val   = a->nedges;
    opt   = &modopt;
    if (val == 0) 
      val = graph_num_edges(gin);
  }
  else if (a->nedges   != 0) {
    tfunc = &graph_threshold_edges;
    val   = a->nedges;
    if (val == 0) 
      val = graph_num_edges(gin);
  }
  else if (a->cmplimit != 0) {
    tfunc = &graph_threshold_components;
    val   = a->cmplimit;
    flags = a->igndis;
  }
  else goto fail;

  switch (a->criteria) {

    case C_PATHSHARING:
      if (tfunc(gin,
                gout,
                val,
                flags,
                opt,
                &graph_init_pathsharing,
                &graph_remove_pathsharing,
                &graph_recalculate_pathsharing))
        goto fail;
      break;

    case C_EDGEBETWEENNESS:
      if (tfunc(gin,
                gout,
                val,
                flags,
                opt,
                &graph_init_edge_betweenness,
                &graph_remove_edge_betweenness,
                &graph_recalculate_edge_betweenness))
        goto fail;
      break;
  }

  if (a->modularity && a->printmod) {

    oldcmp = 0xFFFFFFFF;

    for (i = 0; i < modopt.nvals; i++) {

      if (modopt.ncmps[i] != oldcmp) {

        printf("%05lu, %04u, %0.6f\n", 
               i, modopt.ncmps[i], modopt.modularity[i]);
        oldcmp = modopt.ncmps[i];
      }
    }
  }

  return 0;
  
fail:
  return 1;
}
