/**
 * Print graph statistics in a standard format. Cut down version of cnet.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"

static char doc[] = "ccnet -- calculate and print standard statistics "\
                    "over ngdb graph files in table format";

static struct argp_option opts[] = {
  {"global",     'g', NULL, 0, "print global statistics"},
  {"xyz",        'x', NULL, 0, "print node coordinates"},
  {"degree",     'd', NULL, 0, "print degree"},  
  {"clustering", 'c', NULL, 0, "print clustering coefficient"},
  {"pathlength", 'p', NULL, 0, "print characteristic path length"},
  {"localeff",   'e', NULL, 0, "print local efficiency"},
  {0}
};


typedef struct _args {

  char   *input;
  uint8_t global;
  uint8_t xyz;
  uint8_t degree;
  uint8_t clustering;
  uint8_t pathlength;
  uint8_t localeff;

} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch (key) {

    case 'g': args->global     = 1; break;
    case 'x': args->xyz        = 1; break;
    case 'd': args->degree     = 1; break;
    case 'c': args->clustering = 1; break;
    case 'p': args->pathlength = 1; break;
    case 'e': args->localeff   = 1; break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else                          argp_usage(state);
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1) argp_usage(state);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}


static void _print_global_stats(
  graph_t *g
);

static void _print_node_stats_header(
  graph_t *g,
  args_t  *args
);

static void _print_node_stats(
  graph_t *g,
  args_t  *args,
  uint32_t nidx
);


int main(int argc, char *argv[]) {

  graph_t     g;
  uint64_t    i;  
  uint32_t    nnodes;
  args_t      args;
  struct argp argp = {opts, _parse_opt, "INPUT", doc};

  memset(&args, 0, sizeof(args));

  startup("ccnet", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error opening graph file %s\n", args.input);
    goto fail;
  }

  if (stats_cache_init(&g)) {
    printf("error initialising stats cache\n");
    goto fail;
  }

  if (args.global) _print_global_stats(&g);

  _print_node_stats_header(&g, &args);

  nnodes = graph_num_nodes(&g);

  for (i = 0; i < nnodes; i++) _print_node_stats(&g, &args, i);

  graph_free(&g);
  return 0;

fail:
  return 1;
}


void _print_global_stats(graph_t *g) {

  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  printf("# nodes              %u\n",    graph_num_nodes(               g));
  printf("# edges              %u\n",    graph_num_edges(               g));
  printf("# density            %0.6f\n", stats_density(                 g));
  printf("# degree             %0.3f\n", stats_avg_degree(              g));
  printf("# max degree         %0.0f\n", stats_cache_max_degree(        g));
  printf("# components         %0.0f\n", stats_cache_num_components(    g));
  printf("# largest component  %0.0f\n", stats_cache_largest_component( g));
  printf("# connected          %0.0f\n", stats_cache_connected(         g));
  printf("# disconnected       %0.0f\n", nnodes - stats_cache_connected(g));  
  printf("# clustering         %0.6f\n", stats_cache_graph_clustering(  g));
  printf("# pathlength         %0.6f\n", stats_cache_graph_pathlength(  g));
  printf("# smallworld index   %0.6f\n", stats_smallworld_index(        g));
  printf("# global efficiency  %0.6f\n", stats_cache_global_efficiency( g));
  printf("# assortativity      %0.6f\n", stats_cache_assortativity(     g));
  
  
  

  /*
   * nodes
   * edges
   * density
   * clustering coefficient  *
   * path length             *
   * components              *
   * largest component       *
   * disconnected            *
   * sw index
   * global efficiency       *
   * assortativity           *
   * max degree
   */
}

void _print_node_stats_header(graph_t *g, args_t *args) {

}

void _print_node_stats(graph_t *g, args_t *args, uint32_t nidx) {

}
