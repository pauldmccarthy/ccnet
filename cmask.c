/**
 * Mask a ngdb graph file using values from an ANALYZE75 image.
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
#include "graph/graph_mask.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"

#define MAX_OPS 50

/**
 * The masking operators that are available.
 */
typedef enum {
  CMASK_EQ  = 'e', /**< Equal to                 */
  CMASK_NEQ = 'n', /**< Not equal to             */
  CMASK_GT  = 'g', /**< Greater than             */
  CMASK_GE  = 'a', /**< Greater than or equal to */
  CMASK_LT  = 'l', /**< Less than                */
  CMASK_LE  = 's'  /**< Less than or equal to    */
} op_t;

typedef struct _args {

  char    *input;
  char    *maskf;
  char    *output;
  char    *hdrmsg;
  uint16_t nops;
  op_t     ops[MAX_OPS];
  double   op_params[MAX_OPS];
  uint8_t  real;

} args_t;

static struct argp_option options[] = {
  {"eq",   CMASK_EQ,  "DOUBLE", 0, "Mask nodes with value == parameter"},
  {"neq",  CMASK_NEQ, "DOUBLE", 0, "Mask nodes with value != parameter"},
  {"gt",   CMASK_GT,  "DOUBLE", 0, "Mask nodes with value >  parameter"},
  {"ge",   CMASK_GE,  "DOUBLE", 0, "Mask nodes with value >= parameter"},
  {"lt",   CMASK_LT,  "DOUBLE", 0, "Mask nodes with value <  parameter"},
  {"le",   CMASK_LE,  "DOUBLE", 0, "Mask nodes with value <= parameter"},
  {"real", 'r',        NULL,    0, "Node coordinates are in real units"},
  {0}
};

static char doc[] = "cmask -- mask the nodes of a ngdb file using "\
                    "corresponding voxels from an ANALYZE75 image file\v"\
                    "All nodes with a corresponding image value that passes "\
                    "any of the given operators will be removed from the "\
                    "output graph.";

/**
 * Populates the given mask array, by applying the mask operator on values
 * from the mask image whcih correspond to nodes in the given graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mask_nodes(
  graph_t *g,         /**< graph containing nodes to mask     */
  dsr_t   *hdr,       /**< mask image header                  */
  uint8_t *img,       /**< mask image data                    */
  uint8_t *mask,      /**< mask array to populate             */
  uint8_t  real,      /**< node coordinates are in real units */
  op_t    *ops,       /**< mask operators                     */
  double  *op_params, /**< mask parameters                    */
  uint16_t nops       /**< number of operations               */
);

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {

    case CMASK_EQ:
    case CMASK_NEQ:
    case CMASK_GT:
    case CMASK_GE:
    case CMASK_LT:
    case CMASK_LE:
      if (args->nops >= MAX_OPS) {
        printf("Too many operations - ignoring any more\n");
      }
      else {
        args->ops      [args->nops]   = key;
        args->op_params[args->nops++] = atof(arg);
      }
      break;      

    case 'r':
      args->real = 1;
      break; 

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->maskf  = arg;
      else if (state->arg_num == 2) args->output = arg;
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

  graph_t     gin;
  graph_t     gout;
  uint32_t    nnodes;
  uint8_t    *mask;
  args_t      args;
  dsr_t       hdr;
  uint8_t    *img;
  struct argp argp = {options, _parse_opt, "INPUT MASKFILE OUTPUT", doc};

  mask = NULL;
  memset(&args, 0, sizeof(args));

  startup("cmask", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("error opening input file %s\n", args.input);
    goto fail;
  }

  nnodes = graph_num_nodes(&gin);
  mask = calloc(nnodes, sizeof(uint8_t));
  if (mask == NULL) {
    printf("memory error!?\n");
    goto fail;
  }

  if (analyze_load(args.maskf, &hdr, &img)) {
    printf("error opening image file %s\n", args.output);
    goto fail;
  }

  if (_mask_nodes(
        &gin,
        &hdr,
        img,
        mask,
        args.real,
        args.ops,
        args.op_params,
        args.nops)) {
    printf("error masking nodes\n");
    goto fail;
  }

  if (graph_mask(&gin, &gout, mask)) {
    printf("error masking graph\n");
    goto fail;
  }

  if (ngdb_write(&gout, args.output)) {
    printf("error writing to output file %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t _mask_nodes(
  graph_t *g,
  dsr_t   *hdr,
  uint8_t *img,
  uint8_t *mask,
  uint8_t  real,
  op_t    *ops,
  double  *op_params,
  uint16_t nops) {

  uint64_t       i;
  uint32_t       j;
  uint32_t       nnodes;
  double         imgval;
  graph_label_t *lbl;
  uint32_t       imgi[4];
  double         xl;
  double         yl;
  double         zl;

  memset(imgi, 0, sizeof(imgi));

  nnodes = graph_num_nodes(g);
  xl     = analyze_pixdim_size(hdr, 0);
  yl     = analyze_pixdim_size(hdr, 1);
  zl     = analyze_pixdim_size(hdr, 2);

  for (i = 0; i < nnodes; i++) {

    lbl = graph_get_nodelabel(g, i);
    if (lbl == NULL) goto fail;

    if (real) {
      imgi[0] = (uint32_t)(round(lbl->xval / xl));
      imgi[1] = (uint32_t)(round(lbl->yval / yl));
      imgi[2] = (uint32_t)(round(lbl->zval / zl));
    }
    else {
      imgi[0] = (uint32_t)lbl->xval;
      imgi[1] = (uint32_t)lbl->yval;
      imgi[2] = (uint32_t)lbl->zval;
    }

    imgval = analyze_read_val(hdr, img, imgi);

    for (j = 0; j < nops; j++) {

      if      (ops[j] == CMASK_EQ)  {if (imgval == op_params[j]) break;}
      else if (ops[j] == CMASK_NEQ) {if (imgval != op_params[j]) break;}
      else if (ops[j] == CMASK_GT)  {if (imgval >  op_params[j]) break;}
      else if (ops[j] == CMASK_GE)  {if (imgval >= op_params[j]) break;}
      else if (ops[j] == CMASK_LT)  {if (imgval <  op_params[j]) break;}
      else if (ops[j] == CMASK_LE)  {if (imgval <= op_params[j]) break;}
    }

    if (j == nops) mask[i] = 1;
  }

  return 0;

fail:
  return 1;
}
