/**
 * Update the node labels in a ngdb file. The new label values are taken the
 * corresponding voxel value in a specified 3D ANALYZE75 image file.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "io/analyze75.h"
#include "io/ngdb_graph.h"

typedef struct _args {
  char *input;
  char *output;
  char *labelf;
} args_t;

static char doc[] = "labelngdb -- update node labels in a ngdb file";

static struct argp_option options[] = {
  {"output", 'o', "FILE", 0, "output file (default: overwrite input file)"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {
    case 'o': args->output = arg; break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->labelf = arg;
      else                          argp_usage(state);
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
 * Updates the label value for the given node.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_labelval(
  graph_t *g,   /**< graph        */
  dsr_t   *hdr, /**< label header */
  uint8_t *img, /**< label image  */
  uint32_t nidx /**< node id      */
);

int main(int argc, char *argv[]) {

  graph_t     g;
  uint64_t    i;
  uint32_t    nnodes;
  dsr_t       hdr;
  uint8_t    *img;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT LABELFILE", doc};

  memset(&args, 0, sizeof(args));

  startup("labelngdb", argc, argv, &argp, &args);

  if (args.output == NULL) args.output = args.input;

  if (ngdb_read(args.input, &g)) {
    printf("error openineg input file %s\n", args.input);
    goto fail;
  }

  if (analyze_load(args.labelf, &hdr, &img)) {
    printf("error opening label file %s\n", args.labelf);
    goto fail;
  }

  if (analyze_num_dims(&hdr) != 3) {
    printf("label file does not have 3 dimensions\n");
    goto fail;
  }

  nnodes = graph_num_nodes(&g);

  for (i = 0; i < nnodes; i++) {
    if (_update_labelval(&g, &hdr, img, i)) {
      printf("error updating label value for node %llu\n", i);
      goto fail;
    }
  }

  if (ngdb_write(&g, args.output)) {
    printf("error writing to output file %s\n", args.output);
    goto fail;
  }
  

  return 0;
  
fail:
  return 1;
}


uint8_t _update_labelval(
  graph_t *g, dsr_t *hdr, uint8_t *img, uint32_t nidx) {

  graph_label_t  lbl;
  graph_label_t *plbl;
  uint32_t       dims[3];

  plbl = graph_get_nodelabel(g, nidx);
  if (plbl == NULL) goto fail;

  memcpy(&lbl, plbl, sizeof(graph_label_t));

  dims[0] = lbl.xval;
  dims[1] = lbl.yval;
  dims[2] = lbl.zval;

  if (dims[0] >= analyze_dim_size(hdr, 0)) goto fail;
  if (dims[1] >= analyze_dim_size(hdr, 1)) goto fail;
  if (dims[2] >= analyze_dim_size(hdr, 2)) goto fail;

  lbl.labelval = analyze_read_val(hdr, img, dims);

  if (graph_set_nodelabel(g, nidx, &lbl)) goto fail;

  return 0;

fail:
  return 1;
}
