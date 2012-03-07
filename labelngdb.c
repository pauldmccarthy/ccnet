/**
 * Update the node labels in a ngdb file. The new label values are taken the
 * corresponding voxel value in a specified 3D ANALYZE75 image file.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <argp.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "io/analyze75.h"
#include "io/ngdb_graph.h"

typedef struct _args {
  char   *input;
  char   *output;
  char   *labelf;
  uint8_t real;
} args_t;

static char doc[] = "labelngdb -- update node labels in a ngdb file";

static struct argp_option options[] = {
  {"real", 'r', NULL, 0, "node labels are in real units (default: false)"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {
    
    case 'r': args->real = 1;         break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else if (state->arg_num == 2) args->labelf = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 3) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN; 
  }
  
  return 0;
}

int main(int argc, char *argv[]) {

  graph_t     g;
  dsr_t       hdr;
  uint8_t    *img;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT LABELFILE", doc};

  memset(&args, 0, sizeof(args));

  startup("labelngdb", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error openineg input file %s\n", args.input);
    goto fail;
  }

  if (analyze_load(args.labelf, &hdr, &img)) {
    printf("error opening label file %s\n", args.labelf);
    goto fail;
  }

  if (graph_relabel(&g, &hdr, img, args.real)) {
    printf("error relabelling graph\n");
    goto fail;
  }

  if (ngdb_write(&g, args.output)) {
    printf("error writing to output file %s\n", args.output);
    goto fail;
  }
  

  return 0;
  
fail:
  return 1;
}
