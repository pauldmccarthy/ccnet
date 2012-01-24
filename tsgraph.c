/**
 * Generates a graph from a .mat file containing a correlation matrix. The
 * matrix file is assumed to be symmetric.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "io/mat.h"
#include "io/ngdb_graph.h"
#include "util/startup.h"
#include "graph/graph.h"

#define MAX_LABELS         50
#define NGDB_HDR_DATA_SIZE 512

typedef struct __args {

  char    *input;
  char    *output;
  char    *hdrmsg;
  uint8_t  absval;
  uint8_t  weighted;
  uint8_t  directed;
  double   threshold;
  uint8_t  ninclbls;
  uint8_t  nexclbls;
  
  double   inclbls[MAX_LABELS];
  double   exclbls[MAX_LABELS];  

} args_t;

static char doc[] =
  "tsgraph -- generate a graph from a .mat file";


static struct argp_option options[] = {
  {"hdrmsg",    'h', "MSG",   0,
   "message to save to .ngdb file header"},
  {"absval",    'a',  NULL,   0,
   "use absolute correlation value (default: false)"},
  {"weighted",  'w',  NULL,   0,
   "create weighted graph (default: false)"},
  {"directed",  'd',  NULL,   0,
   "create directed graph (default: false)"}, 
  {"threshold", 't', "FLOAT", 0,
   "discard correlation values below this (default: 0.9)"}, 
  {"incl",      'i', "FLOAT", 0,
   "include only rows/columns with this label"},
  {"excl",      'e', "FLOAT", 0,
   "exclude rows/columns with this label"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;
  args = state->input;

  switch (key) {
    case 'h': args->hdrmsg    = arg;       break;
    case 'a': args->absval    = 1;         break;
    case 'w': args->weighted  = 1;         break;
    case 't': args->threshold = atof(arg); break;
      
    case 'i':
      if (args->ninclbls < MAX_LABELS) 
        args->inclbls[(args->ninclbls)++] = atof(arg);
      break;
      
    case 'e':
      if (args->nexclbls < MAX_LABELS) 
        args->exclbls[(args->nexclbls)++] = atof(arg);
      break; 

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

/**
 * Compiles a list of row/column IDs (i.e. node IDs) from the given
 * include/exclude lists. The node IDs are stored in the given nodes
 * array, and the number of nodes returned. The nodes array must already
 * be allocated, and have the length of the matrix rows/columns.
 *
 * \return number of nodes on success, negative on failure.
 */
static int64_t _apply_label_mask(
  mat_t    *mat,      /**< the matrix file                           */
  double   *inclbls,  /**< include only rows/columns with this label */
  double   *exclbls,  /**< exclude rows/columns with this label      */
  uint8_t   ninclbls, /**< number of labels in include list          */
  uint8_t   nexclbls, /**< number of labels in exclude list          */
  uint32_t *nodes     /**< place to store row/column/node IDs        */
);

/**
 * Adds edges between the nodes in the graph, according to the correlation
 * values in the matrix file.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _connect_graph(
  mat_t    *mat,       /**< mat file                             */
  graph_t  *graph,     /**< initialised empty graph              */
  uint32_t *nodes,     /**< row/column/node ids to include       */
  uint32_t  nnodes,    /**< number of nodes                      */
  double    threshold, /**< ignore correlation values below this */
  uint8_t   absval     /**< use absolute correlation value       */
);


int main (int argc, char *argv[]) {

  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};
  mat_t      *mat;
  graph_t     graph;
  uint32_t   *nodes;
  int64_t     nnodes;

  nodes = NULL;

  memset(&args, 0, sizeof(args));
  args.threshold = 0.9;

  startup("tsgraph", argc, argv, &argp, &args);

  mat = mat_open(args.input);
  if (mat == NULL) {
    printf("error opening mat file %s\n", args.input);
    goto fail;
  }

  nodes = calloc(mat_num_rows(mat), sizeof(uint32_t));
  if (nodes == NULL) goto fail;
  
  nnodes = _apply_label_mask(
    mat,
    args.inclbls,
    args.exclbls,
    args.ninclbls,
    args.nexclbls,
    nodes);
  
  if (nnodes < 0) goto fail;

  if (graph_create(&graph, nnodes, args.directed)) {
    printf("error creating graph\n");
    goto fail;
  }

  if (_connect_graph(
        mat,
        &graph,
        nodes,
        nnodes,
        args.threshold,
        args.absval)) {
    printf("error connecting graph\n");
    goto fail;
  }

  if (ngdb_write(&graph, args.output)) {
    printf("error writing graph to %s\n", args.output);
    goto fail;
  }

  graph_free(&graph);
  free(nodes);
  mat_close(mat);
  return 0;

fail:
  mat_close(mat);
  if (nodes != NULL) free(nodes);
  return 1;
}

int64_t _apply_label_mask(
  mat_t    *mat,
  double   *inclbls,
  double   *exclbls,
  uint8_t   ninclbls,
  uint8_t   nexclbls,
  uint32_t *nodes) {

  return 0;
}

uint8_t _connect_graph(
  mat_t    *mat,
  graph_t  *graph,
  uint32_t *nodes,
  uint32_t  nnodes,
  double    threshold,
  uint8_t   absval
) {

  return 0;
}
