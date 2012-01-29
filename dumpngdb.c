/**
 * Prints out the contents of a ngdb file.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <argp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/graph_log.h"
#include "io/ngdb_graph.h"
#include "util/startup.h"

typedef struct _args {
  char   *input;
  uint8_t meta;
  uint8_t labels;
  uint8_t graph;
} args_t;

static char doc[] =
  "dumpngdb -- print the contents of a .ngdb file";

static struct argp_option options[] = {
  {"meta",   'm', NULL, 0, "print information about the file"},
  {"labels", 'l', NULL, 0, "print node labels"},
  {"graph",  'g', NULL, 0, "print out nodes and neighbours"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch(key) {
    
    case 'm': args->meta   = 1; break;
    case 'l': args->labels = 1; break;
    case 'g': args->graph  = 1; break;
      
    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->input = arg;
      else argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break; 
      
    default:
      return ARGP_ERR_UNKNOWN;

  }
  
  return 0;
}

static void _meta(  graph_t *g);
static void _labels(graph_t *g);
static void _graph( graph_t *g);

int main(int argc, char *argv[]) {

  struct argp argp = {options, _parse_opt, "INPUT", doc};
  args_t      args; 
  graph_t     g;

  memset(&args, 0, sizeof(args));

  startup("dumpngdb", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error reading ngdb file %s\n", args.input);
    goto fail;
  }

  if (args.meta)   _meta(  &g);
  if (args.labels) _labels(&g);
  if (args.graph)  _graph( &g);

  graph_free(&g);
  return 0;
  
fail:
  return 1;
}


void _meta(graph_t *g) {

  uint32_t i;
  uint16_t nmsgs;
  char    *msg;

  nmsgs = graph_log_num_msgs(g);

  printf("num nodes: %u\n", graph_num_nodes(g));
  printf("num edges: %u\n", graph_num_edges(g));
  printf("directed:  %u\n", graph_is_directed(g));


  printf("log messages:\n");
  for (i = 0; i < nmsgs; i++) {
    
    msg = graph_log_get_msg(g, i);
    printf("  %3u: %s\n", i, msg);
  }

}

void _labels(graph_t *g) {

  graph_label_t *lbl;
  uint32_t       nnodes;
  uint64_t       i;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    
    lbl = graph_get_nodelabel(g, i);

    printf(
      "node %5llu: %0.3f %0.3f %0.3f %u\n",
      i, lbl->xval, lbl->yval, lbl->zval, lbl->labelval);
  }

}

void _graph(graph_t *g) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);

    printf("%5llu: ", i);

    for (j = 0; j < nnbrs; j++) {
      printf("%5u", nbrs[j]);
      if (j < nnbrs-1) printf(" ");
    }
    printf("\n");
  }
}
