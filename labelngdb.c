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

/**
 * Updates the label value for the given node.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_labelval(
  graph_t *g,    /**< graph                    */
  dsr_t   *hdr,  /**< label header             */
  uint8_t *img,  /**< label image              */
  uint32_t nidx, /**< node id                  */
  uint8_t  real  /**< labels are in real units */
);

int main(int argc, char *argv[]) {

  graph_t     g;
  uint64_t    i;
  uint32_t    nnodes;
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

  if (analyze_num_dims(&hdr) != 3) {
    printf("label file does not have 3 dimensions\n");
    goto fail;
  }

  nnodes = graph_num_nodes(&g);

  for (i = 0; i < nnodes; i++) {
    if (_update_labelval(&g, &hdr, img, i, args.real)) {
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
  graph_t *g, dsr_t *hdr, uint8_t *img, uint32_t nidx, uint8_t real) {

  graph_label_t  lbl;
  graph_label_t *plbl;
  double         xl;
  double         yl;
  double         zl;
  uint32_t       dims[3];

  xl = analyze_pixdim_size(hdr, 0);
  yl = analyze_pixdim_size(hdr, 1);
  zl = analyze_pixdim_size(hdr, 2);

  plbl = graph_get_nodelabel(g, nidx);
  if (plbl == NULL) goto fail;

  memcpy(&lbl, plbl, sizeof(graph_label_t));

  if (real) {
    dims[0] = (uint32_t)(round(lbl.xval / xl));
    dims[1] = (uint32_t)(round(lbl.xval / yl));
    dims[2] = (uint32_t)(round(lbl.xval / zl));
  }
  else {
    dims[0] = (uint32_t)lbl.xval;
    dims[1] = (uint32_t)lbl.yval;
    dims[2] = (uint32_t)lbl.zval;
  }

  if (dims[0] >= analyze_dim_size(hdr, 0)) goto fail;
  if (dims[1] >= analyze_dim_size(hdr, 1)) goto fail;
  if (dims[2] >= analyze_dim_size(hdr, 2)) goto fail;

  lbl.labelval = analyze_read_val(hdr, img, dims);

  if (graph_set_nodelabel(g, nidx, &lbl)) goto fail;

  return 0;

fail:
  return 1;
}
