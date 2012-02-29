/**
 * Program which exports node attributes from a spatially
 * annotated graph as an ANALYZE75 image file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <argp.h>

#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"


typedef enum {

  N2I_DEGREE,
  N2I_DEGCENT,
  N2I_CMPNUM,

} img_val_t;


typedef struct args {

  char     *input;
  char     *output;
  uint16_t  xn;
  uint16_t  yn;
  uint16_t  zn;
  double    xl;
  double    yl;
  double    zl;
  uint8_t   real;
  uint8_t   rev;
  img_val_t value;

} args_t;

static struct argp_option options[] = {

  {"xn",      'x', "INT",   0, "X dimension size"},
  {"yn",      'y', "INT",   0, "Y dimension size"},
  {"zn",      'z', "INT",   0, "Z dimension size"},
  {"xl",      'a', "FLOAT", 0, "X voxel length"},
  {"yl",      'b', "FLOAT", 0, "Y voxel length"},
  {"zl",      'c', "FLOAT", 0, "Z voxel length"},
  {"real",    'e', NULL,    0, "node labels are in real units (default: false)"},
  {"rev",     'r', NULL,    0, "reverse endianness"},
  {"degree",  'd', NULL,    0, "output degree values"},
  {"degcent", 'g', NULL,    0, "output degree centrality values"},
  {"cmpnum",  'm', NULL,    0, "output component number"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch (key) {
    case 'x': a->xn    = atoi(arg);   break;
    case 'y': a->yn    = atoi(arg);   break;
    case 'z': a->zn    = atoi(arg);   break;
    case 'a': a->xl    = atof(arg);   break;
    case 'b': a->yl    = atof(arg);   break;
    case 'c': a->zl    = atof(arg);   break;
    case 'e': a->real  = 1;           break;
    case 'r': a->rev   = 1;           break;
    case 'd': a->value = N2I_DEGREE;  break;
    case 'g': a->value = N2I_DEGCENT; break;
    case 'm': a->value = N2I_CMPNUM;  break;
      
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

static char doc[] = "ngdb2img - convert a spatially annotated "\
                    "ngdb file to an ANALYZE75 image file";


static void _fill_hdr(
  dsr_t  *dsr,
  args_t *args);

static uint8_t _graph_to_img(
  graph_t  *g,
  dsr_t    *dsr,
  uint8_t  *img,
  uint8_t   real,
  img_val_t val
);


int main(int argc, char *argv[]) {

  graph_t     gin;
  dsr_t       hdr;
  uint8_t    *img;
  uint32_t    nbytes;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));
  
  args.xn = 64;
  args.yn = 64;
  args.zn = 16;
  args.xl = 2.328125;
  args.yl = 2.9375;
  args.zl = 9.25;

  startup("ngdb2img", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  stats_cache_init(&gin);

  _fill_hdr(&hdr, &args);

  nbytes = analyze_value_size(&hdr)*analyze_num_vals(& hdr);

  img = calloc(1, nbytes);
  if (img == NULL) {
    printf("calloc fail!?\n");
    goto fail;
  }

  if (_graph_to_img(&gin, &hdr, img, args.real, args.value)) {
    printf("Could not convert graph to image\n");
    goto fail;
  }

  if (analyze_write_hdr(args.output, &hdr)) {
    printf("Error writing header %s\n", args.output);
    goto fail;
  }

  if (analyze_write_img(args.output, &hdr, img)) {
    printf("Error writing image %s\n", args.output);
    goto fail;
  }

  free(img);
  return 0;

fail:
  return 1;
}


static void _fill_hdr(dsr_t *dsr, args_t *args) {

  memset(dsr, 0, sizeof(dsr_t));

  dsr->hk.sizeof_hdr = 348;

  dsr->dime.dim[0] = 3;
  dsr->dime.dim[1] = args->xn;
  dsr->dime.dim[2] = args->yn;
  dsr->dime.dim[3] = args->zn;
  dsr->dime.dim[4] = 1;
  dsr->dime.dim[5] = 1;
  dsr->dime.dim[6] = 1;
  dsr->dime.dim[7] = 1;

  dsr->dime.datatype  = DT_FLOAT;
  dsr->dime.bitpix    = 32;

  dsr->dime.pixdim[0] = 0.0;
  dsr->dime.pixdim[1] = args->xl;
  dsr->dime.pixdim[2] = args->yl;
  dsr->dime.pixdim[3] = args->zl;

  dsr->rev            = args->rev;
}

static uint8_t _graph_to_img(
  graph_t *g, dsr_t *hdr, uint8_t *img, uint8_t real, img_val_t valtype) {

  uint64_t       i;
  uint32_t       nnodes;
  graph_label_t *lbl;

  double xl;
  double yl;
  double zl;
  float  val;
  uint32_t uval;

  uint32_t imgi[4];

  memset(imgi, 0, sizeof(imgi));

  xl = analyze_pixdim_size(hdr, 0);
  yl = analyze_pixdim_size(hdr, 1);
  zl = analyze_pixdim_size(hdr, 2);

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    lbl = graph_get_nodelabel(    g, i);

    switch (valtype) {
      case N2I_DEGREE:  val = stats_degree           (g, i); break;
      case N2I_DEGCENT: val = stats_degree_centrality(g, i); break;
      case N2I_CMPNUM:
        stats_cache_node_component(g, i, &uval);
        val = uval;
        break;
      default: return 1;
    }

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

    analyze_write_val(hdr, img, imgi, val);
  }

  return 0;
}
