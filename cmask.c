/**
 * Include/exclude nodes in a ngdb graph file using values from an ANALYZE75
 * image.
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

#define MAX_MASKS 50

typedef struct _args {

  char    *input;
  char    *maskf;
  char    *output;
  char    *hdrmsg;
  uint8_t  include;
  uint16_t nmasks;
  double   maskvals[MAX_MASKS];
  uint8_t  real;

} args_t;

static struct argp_option options[] = {
  {"inc",     'i',  NULL,    0, "Include all nodes with any of the "\
                                "given values (default is to exclude)"},
  {"maskval", 'm', "DOUBLE", 0, "Mask value"},
  {"real",    'r',  NULL,    0, "Node coordinates are in real units"},
  {0}
};

static char doc[] = "cmask -- mask the nodes of a ngdb file using "\
                    "corresponding voxels from an ANALYZE75 image file\v"\
                    "Nodes can either be included, or excluded based on "\
                    "their voxel value.";

/**
 * Populates the given mask array, by including/excluding
 * nodes based on the corresponding value in the mask image.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mask_nodes(
  graph_t *g,         /**< graph containing nodes to mask     */
  dsr_t   *hdr,       /**< mask image header                  */
  uint8_t *img,       /**< mask image data                    */
  uint8_t *mask,      /**< mask array to populate             */
  uint8_t  real,      /**< node coordinates are in real units */
  uint8_t  include,   /**< include nodes based on voxel value,
                           instead of exclude                 */
  double  *maskvals,  /**< mask values                        */
  uint16_t nmasks     /**< number of mask values              */
);

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {

    case 'i': args->include = 1; break;
    case 'm':
      if (args->nmasks >= MAX_MASKS) {
        printf("Too many mask values - ignoring any more\n");
      }
      else {
        args->maskvals [args->nmasks++] = atof(arg);
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
        args.include,
        args.maskvals,
        args.nmasks)) {
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
  uint8_t  include,
  double  *maskvals,
  uint16_t nmasks) {

  uint64_t       i;
  uint32_t       j;
  uint32_t       nnodes;
  double         imgval;
  graph_label_t *lbl;
  uint32_t       imgi[4];
  double         xl;
  double         yl;
  double         zl;
  uint16_t       maskhits;

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
    maskhits = 0;

    for (j = 0; j < nmasks; j++) 
      if (imgval == maskvals[j]) maskhits++;

    if (include) {if (maskhits >  0) mask[i] = 1;}
    else         {if (maskhits == 0) mask[i] = 1;}
  }

  return 0;

fail:
  return 1;
}
