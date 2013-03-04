/**
 * Program to extract subgraphs by node coordinates.
 *
 * This program may be used to extract a subgraph from a parent graph. Nodes
 * to be included in the subgraph are selected by a coordinate range in any or
 * all of the x,y, and z dimensions.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include "stats/stats.h"
#include "graph/graph.h"
#include "graph/graph_log.h"
#include "graph/graph_mask.h"
#include "util/startup.h"
#include "util/array.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"

/**
 * Input arguments.
 */
typedef struct _args {
  char *input;   /**< name of input file            */
  char *output;  /**< name of output file           */
  char *hdrmsg;  /**< message to add to output file */
  float xlo;     /**< Low X coordinate (inclusive)  */ 
  float xhi;     /**< High X coordinate (inclusive) */ 
  float ylo;     /**< Low Y coordinate (inclusive)  */ 
  float yhi;     /**< High Y coordinate (inclusive) */ 
  float zlo;     /**< Low Z coordinate (inclusive)  */ 
  float zhi;     /**< High Z coordinate (inclusive) */ 

} args_t;

static char doc[]   = "cslice - extract a subgraph by node coordinates";

static struct argp_option options[] = {
  {"hdrmsg",    'h', "MSG",    0, "message to save to .ngdb file header"},  
  {"xlo",       'x', "FLOAT",  0, "Low X coordinate (inclusive)"},
  {"xhi",       'a', "FLOAT",  0, "High X coordinate (inclusive)"},
  {"ylo",       'y', "FLOAT",  0, "Low Y coordinate (inclusive)"},
  {"yhi",       'b', "FLOAT",  0, "HIgh Y coordinate (inclusive)"},
  {"zlo",       'z', "FLOAT",  0, "Low Z coordinate (inclusive)"},
  {"zhi",       'c', "FLOAT",  0, "High Z coordinate (inclusive)"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {
  
  args_t *args;

  args = state->input;

  switch(key) {

    case 'h': args->hdrmsg    = arg;       break;
    case 'x': args->xlo       = atof(arg); break;
    case 'a': args->xhi       = atof(arg); break;
    case 'y': args->ylo       = atof(arg); break;
    case 'b': args->yhi       = atof(arg); break;
    case 'z': args->zlo       = atof(arg); break;
    case 'c': args->zhi       = atof(arg); break;
      
    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 2) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;    
}

/**
 * Finds all nodes in the given graph which are within the specified
 * coordinate range along the specified dimension. Masks said nodes in the
 * given mask array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_nodes_by_coordinate(
  graph_t *g,   /**< input graph                   */
  float    xlo, /**< Low X coordinate (inclusive)  */
  float    xhi, /**< High X coordinate (inclusive) */
  float    ylo, /**< Low Y coordinate (inclusive)  */
  float    yhi, /**< High Y coordinate (inclusive) */
  float    zlo, /**< Low Z coordinate (inclusive)  */
  float    zhi, /**< High Z coordinate (inclusive) */
  uint8_t *mask /**< empty mask array to populate  */
);


int main (int argc, char *argv[]) {

  graph_t     gin;
  graph_t     gout;
  uint32_t    nginnodes;
  uint8_t    *mask;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};
  args_t      args;

  memset(&args, 0, sizeof(args_t));

  args.xlo = 0;
  args.xhi = 0xFFFF;
  args.ylo = 0;
  args.yhi = 0xFFFF;
  args.zlo = 0;
  args.zhi = 0xFFFF;

  startup("cslice", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  nginnodes = graph_num_nodes(&gin);

  mask = calloc(nginnodes, sizeof(uint8_t));
  if (mask == NULL) {
    printf("memory error!?\n");
    goto fail;
  }

  if (_find_nodes_by_coordinate(&gin,
                                args.xlo, args.xhi,
                                args.ylo, args.yhi,
                                args.zlo, args.zhi, mask)) {
    printf("Could not find nodes by component\n");
    goto fail;
  }
  
  if (graph_mask(&gin, &gout, mask)) {
    printf("could not mask graph\n");
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


uint8_t _find_nodes_by_coordinate(
  graph_t *g,
  float xlo, float xhi,
  float ylo, float yhi,
  float zlo, float zhi,
  uint8_t *mask) {

  uint64_t       i;
  uint32_t       nnodes;
  graph_label_t *lbl;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    lbl = graph_get_nodelabel(g, i);

    if (lbl->xval >= xlo && lbl->xval <= xhi) 
      if (lbl->yval >= ylo && lbl->yval <= yhi) 
        if (lbl->zval >= zlo && lbl->zval <= zhi)
          mask[i] = 1;
  }

  return 0;
}
