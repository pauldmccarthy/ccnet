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
  {"node",       'n', NULL, 0, "print node statistics"},
  {"bigstats",   'b', NULL, 0, "zero big global stats"},
  {"pathlength", 'p', NULL, 0, "zero pathlength"},
  {"clustering", 'c', NULL, 0, "zero clustering"},
  {"efficiency", 'f', NULL, 0, "zero efficiency"},
  {"closeness",  'l', NULL, 0, "zero closeness"},
  {"edgedist",   'e', NULL, 0, "zero edgedist"},
  {0}
};


typedef struct _args {

  char   *input;
  uint8_t global;
  uint8_t node;
  uint8_t bigstats;
  uint8_t pathlength;
  uint8_t clustering;
  uint8_t efficiency;
  uint8_t closeness;
  uint8_t edgedist;

} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch (key) {

    case 'g': args->global     = 1; break;
    case 'n': args->node       = 1; break;
    case 'b': args->bigstats   = 1; break;
    case 'p': args->pathlength = 1; break;
    case 'c': args->clustering = 1; break;
    case 'f': args->efficiency = 1; break;
    case 'l': args->closeness  = 1; break;
    case 'e': args->edgedist   = 1; break;

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
  graph_t *g,
  args_t  *args
);

static void _print_node_stats_header(
  graph_t *g,
  args_t  *args
);

static void _print_node_stats(
  graph_t *g,
  args_t  *args,
  uint32_t n
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

  if (args.global) _print_global_stats(&g, &args);

  if (args.node) {
    
    _print_node_stats_header(&g, &args);
    
    nnodes = graph_num_nodes(&g);
    for (i = 0; i < nnodes; i++)
      _print_node_stats(&g, &args, i);
  }

  graph_free(&g);
  return 0;

fail:
  return 1;
}


void _print_global_stats(graph_t *g, args_t *args) {

  uint32_t nnodes;
  double   clustering;
  double   pathlength;
  double   swidx;
  double   globeff;
  double   assort;

  clustering = 0;
  pathlength = 0;
  swidx      = 0;
  globeff    = 0;
  assort     = 0;

  if (!args->bigstats) {
    clustering = stats_cache_graph_clustering( g);
    pathlength = stats_cache_graph_pathlength( g);
    swidx      = stats_smallworld_index(       g);
    globeff    = stats_cache_global_efficiency(g);
    assort     = stats_cache_assortativity(    g);
  }

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
  printf("# clustering         %0.6f\n", clustering);
  printf("# pathlength         %0.6f\n", pathlength);
  printf("# smallworld index   %0.6f\n", swidx);
  printf("# global efficiency  %0.6f\n", globeff);
  printf("# assortativity      %0.6f\n", assort);
}

void _print_node_stats_header(graph_t *g, args_t *args) {

  printf("node,");
  printf("x,");
  printf("y,");
  printf("z,");
  printf("label,");
  printf("degree,");
  printf("clustering,");
  printf("local efficiency,");
  printf("pathlength,");
  printf("closeness,");
  printf("edgedist,");
  printf("component\n");
}

void _print_node_stats(graph_t *g, args_t *args, uint32_t n) {

  graph_label_t *lbl;
  double         clust;
  double         leff;
  double         plen;
  double         edgedist;
  double         close;
  uint32_t       cmp;

  clust    = 0;
  leff     = 0;
  plen     = 0;
  edgedist = 0;
  close    = 0;

  lbl = graph_get_nodelabel(  g, n);

  if (!args->clustering) stats_cache_node_clustering(       g, n, &clust);
  if (!args->efficiency) stats_cache_node_local_efficiency( g, n, &leff);
  if (!args->pathlength) stats_cache_node_pathlength(       g, n, &plen);
  if (!args->edgedist)   close = stats_closeness_centrality(g, n);
  if (!args->edgedist)   stats_cache_node_edgedist(         g, n, &edgedist);

  stats_cache_node_component( g, n, &cmp);

  printf("%u,",    n);
  printf("%0.6f,", lbl->xval);
  printf("%0.6f,", lbl->yval);
  printf("%0.6f,", lbl->zval);
  printf("%u,",    lbl->labelval);
  printf("%u,",    graph_num_neighbours(g, n));
  printf("%0.6f,", clust);
  printf("%0.6f,", leff);
  printf("%0.6f,", plen);
  printf("%0.6f,", close);
  printf("%0.6f,", edgedist);
  printf("%u\n",   cmp);
}
