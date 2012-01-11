/**
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "util/startup.h"
#include "graph/graph.h"
#include "io/ngdb_graph.h"
#include "io/dot.h"

static char doc[] = "cdot - convert a ngdb file to a dot file";

typedef struct args {
  
  char     *input;
  char     *output;
  char     *cmap;
  uint16_t  dotopts;

} args_t;

static struct argp_option options[] = {
  {"colormap",   'c', "FILE", 0, "file specifying label <-> color mappings"},
  {"randcolor",  'r', NULL,   0, "randomise per-label colours"},
  {"edgelabels", 'e', NULL,   0, "set edge weights as labels"},
  {"nodelval",   'n', NULL,   0, "include node labels in dot labels"},
  {"nodeid",     'i', NULL,   0, "include node IDs in dot labels"},
  {"nodepos",    'p', NULL,   0, "include node positions"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {

    case 'c': a->cmap     = arg;               break;
    case 'r': a->dotopts |= DOT_RAND_COLOUR;   break;
    case 'e': a->dotopts |= DOT_EDGE_LABELS;   break;
    case 'n': a->dotopts |= DOT_NODE_LABELVAL; break;
    case 'i': a->dotopts |= DOT_NODE_NODEID;   break;
    case 'p': a->dotopts |= DOT_NODE_POS;      break;

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

int main (int argc, char *argv[]) {

  FILE       *hd;
  graph_t     g;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));

  startup("cdot", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error loading ngdb file %s\n", args.input);
    goto fail;
  }

  hd = fopen(args.output, "wt");
  if (hd == NULL) goto fail;

  dot_write(hd, &g, args.cmap, args.dotopts);

  fclose(hd);

  return 0;

fail:
  return 1;
}


