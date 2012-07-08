/**
 * Extracts time series data from an ANALYZE75 image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <argp.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "util/startup.h"
#include "io/analyze75.h"
#include "io/ngdb_graph.h"
#include "timeseries/analyze_volume.h"
#include "graph/graph.h"

typedef struct _args {
  
  char    *input;
  char    *lblf;
  char    *maskf;
  char    *ngdbf;
  
  uint8_t  allnode;
  uint32_t nodeidx;
  uint8_t  bylbl;
  uint8_t  byidx;
  uint8_t  byreal;
  uint8_t  avg;
  
  uint32_t lblval;
  float    x;
  float    y;
  float    z;
 
} args_t ;

static struct argp_option options[] = {
  {"lblf",    'f', "FILE",   0, "ANALYZE75 label file"},
  {"maskf",   'm', "FILE",   0, "ANALYZE75 mask file"},
  {"ngdbf",   'g', "FILE",   0, "corresponding graph file"},
  {"allnode", 'o',  NULL,    0, "extract time series for all nodes in graph"},
  {"nodeidx", 'n', "INT",    0, "extract time series for the specified node"},
  {"bylbl",   'l',  NULL,    0, "extract time series by voxel/node label"},
  {"byidx",   'i',  NULL,    0, "extract time series by xyz indices"},
  {"byreal",  'r',  NULL,    0, "extract time series by real xyz coordinates"},
  {"avg",     'a',  NULL,    0, "print average of all specified time series"},
  {"lblval",  'v', "INT",    0, "extract time series with this label"},
  {"x",       'x', "FLOAT",  0, "x index/coordinate"},
  {"y",       'y', "FLOAT",  0, "y index/coordinate"},
  {"z",       'z', "FLOAT",  0, "z index/coordinate"},
  {0},
};

static char doc[] = "tsimg -- extract time series "\
                    "data from an ANALYZE75 volume ";

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {

    case 'f': args->lblf    = arg;         break;
    case 'm': args->maskf   = arg;         break;
    case 'g': args->ngdbf   = arg;         break;
    case 'o': args->allnode = 1;           break;
    case 'n': args->nodeidx = atoi(arg)+1; break;
    case 'l': args->bylbl   = 1;           break;
    case 'i': args->byidx   = 1;           break;
    case 'r': args->byreal  = 1;           break;
    case 'a': args->avg     = 1;           break;
    case 'v': args->lblval  = atoi(arg);   break;
    case 'x': args->x       = atof(arg);   break;
    case 'y': args->y       = atof(arg);   break;
    case 'z': args->z       = atof(arg);   break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->input = arg;
      else                     argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN; 
  }
  
  return 0;
}


static uint8_t _make_mask_mask(
  args_t           *args,
  analyze_volume_t *vol,
  uint8_t          *mask
);

static uint8_t _make_graph_mask(
  args_t           *args,
  analyze_volume_t *vol,
  uint8_t          *mask
);

static uint8_t _make_lbl_mask(
  args_t           *args,
  analyze_volume_t *vol,
  uint8_t          *mask
);

static uint8_t _make_idx_mask(
  args_t           *args,
  analyze_volume_t *vol,
  uint8_t          *mask  
);

static uint8_t _print_by_mask(
  analyze_volume_t *vol,
  uint8_t          *mask,
  uint8_t           avg
);

static void _print_ts(
  uint16_t len,
  double  *tsdata
);

int main(int argc, char *argv[]) {

  analyze_volume_t vol;
  args_t           args;
  uint8_t         *mask;
  struct argp      argp = {options, _parse_opt, "INPUT", doc};

  mask = NULL;

  memset(&args, 0, sizeof(args_t));

  startup("tsimg", argc, argv, &argp, &args);

  if (analyze_open_volume(args.input, &vol)) {
    printf("could not open ANALYZE75 volume %s\n", args.input);
    goto fail;
  }

  mask = calloc(analyze_num_vals(vol.hdrs), sizeof(uint8_t));
  if (mask == NULL) {
    printf("out of memory?!\n");
    goto fail;
  }

  if (args.maskf != NULL) {
    if (_make_mask_mask(&args, &vol, mask)) {
      printf("error making mask by mask file %s\n", args.maskf);
      goto fail;
    }
  }
  
  else if (args.lblf != NULL) {
    if (_make_lbl_mask(&args, &vol, mask)) {
      printf("error making mask by label file %s\n", args.lblf);
      goto fail;
    }
  }
  
  else if (args.ngdbf != NULL) {
    if (_make_graph_mask(&args, &vol, mask)) {
      printf("error making mask by graph %s\n", args.ngdbf);
      goto fail;
    }
  }

  else if (args.byidx || args.byreal) {

    if (_make_idx_mask(&args, &vol, mask)) {
      printf("error making mask by index/coordinates\n");
      goto fail;
    }
  }

  if (_print_by_mask(&vol, mask, args.avg)) {
    printf("error printing time series\n");
    goto fail;
  }

  free(mask);
  analyze_free_volume(&vol);
  return 0;

fail:
  return 1;
}

uint8_t _make_mask_mask(
  args_t *args, analyze_volume_t *vol, uint8_t *mask) {
  uint64_t i;
  uint32_t nvals;
  dsr_t    mskhdr;
  uint8_t *mskimg;
  double   mskval;
  
  mskimg = NULL;
  nvals  = analyze_num_vals(vol->hdrs);

  if (analyze_load(args->maskf, &mskhdr, &mskimg))   goto fail;
  if (analyze_hdr_compat_two(&mskhdr, vol->hdrs, 1)) goto fail;

  for (i = 0; i < nvals; i++) {

    mskval = analyze_read_by_idx(&mskhdr, mskimg, i);

    if (mskval == 0) continue;

    mask[i] = 1;
  }

  free(mskimg);
  return 0;
  
fail:

  if (mskimg != NULL) free(mskimg);
  return 1;
}

static uint8_t _make_graph_mask(
  args_t *args, analyze_volume_t *vol, uint8_t *mask) {

  uint64_t       i;
  uint32_t       nnodes;
  uint32_t       idx;
  uint32_t       dims[3];
  graph_label_t *lbl;
  graph_t        g;

  if (ngdb_read(args->ngdbf, &g)) goto fail;

  nnodes = graph_num_nodes(&g);

  for (i = 0; i < nnodes; i++) {

    lbl = graph_get_nodelabel(&g, i);

    if (args->byreal) {
      dims[0] = (uint32_t)round(lbl->xval / analyze_pixdim_size(vol->hdrs, 0));
      dims[1] = (uint32_t)round(lbl->yval / analyze_pixdim_size(vol->hdrs, 1));
      dims[2] = (uint32_t)round(lbl->zval / analyze_pixdim_size(vol->hdrs, 2));
    }
    else {
      dims[0] = (uint32_t)round(lbl->xval);
      dims[1] = (uint32_t)round(lbl->yval);
      dims[2] = (uint32_t)round(lbl->zval);
    }

    idx = analyze_get_index(vol->hdrs, dims);


    if (args->nodeidx) {
      if (args->nodeidx == i+1) {
      mask[idx] = 1;
      break;
    }}

    else if (args->allnode)
      mask[idx] = 1;
    
    else if (args->bylbl) {
      if (lbl->labelval == args->lblval)
        mask[idx] = 1;
    }
    
    else goto fail;
  }
  
  graph_free(&g);

  return 0;

fail:
  return 1;
}

static uint8_t _make_lbl_mask(
  args_t *args, analyze_volume_t *vol, uint8_t *mask) {

  uint64_t i;
  uint32_t nvals;
  dsr_t    lblhdr;
  uint8_t *lblimg;
  double   lblval;
  
  lblimg = NULL;
  nvals  = analyze_num_vals(vol->hdrs);

  if (analyze_load(args->lblf, &lblhdr, &lblimg))    goto fail;
  if (analyze_hdr_compat_two(&lblhdr, vol->hdrs, 1)) goto fail;

  for (i = 0; i < nvals; i++) {

    lblval = analyze_read_by_idx(&lblhdr, lblimg, i);

    if (lblval != args->lblval) continue;

    mask[i] = 1;
  }

  free(lblimg);
  return 0;
  
fail:

  if (lblimg != NULL) free(lblimg);
  return 1;
}

static uint8_t _make_idx_mask(
  args_t *args, analyze_volume_t *vol, uint8_t *mask) {

  uint32_t idx;
  uint32_t dims[3];

  if (args->byreal) {
    dims[0] = (uint32_t)round(args->x / analyze_pixdim_size(vol->hdrs, 0));
    dims[1] = (uint32_t)round(args->y / analyze_pixdim_size(vol->hdrs, 1));
    dims[2] = (uint32_t)round(args->z / analyze_pixdim_size(vol->hdrs, 2));
  }
  else if (args->byidx) {
    dims[0] = (uint32_t)round(args->x);
    dims[1] = (uint32_t)round(args->y);
    dims[2] = (uint32_t)round(args->z);
  }
  else goto fail;

  idx = analyze_get_index(vol->hdrs, dims);
  mask[idx] = 1;
  
  return 0;
  
fail:
  return 1;
}


uint8_t _print_by_mask(analyze_volume_t *vol, uint8_t *mask, uint8_t avg) {

  uint32_t nseries;
  uint32_t nvxls;
  uint64_t i;
  uint32_t j;
  double  *tsdata;
  double  *tsavg;

  tsdata  = NULL;
  tsavg   = NULL;
  nseries = 0;
  nvxls   = analyze_num_vals(vol->hdrs);

  tsdata = calloc(vol->nimgs, sizeof(double));
  if (tsdata == NULL) goto fail;
  
  tsavg = calloc(vol->nimgs, sizeof(double));
  if (tsavg == NULL) goto fail; 

  for (i = 0; i < nvxls; i++) {

    if (!mask[i]) continue;

    nseries ++;
    
    if (analyze_read_timeseries_by_idx(vol, i, tsdata)) goto fail;

    for (j = 0; j < vol->nimgs; j++) tsavg[j] += tsdata[j];

    if (!avg) _print_ts(vol->nimgs, tsdata);
  }

  for (j = 0; j < vol->nimgs; j++) tsavg[j] /= nseries;

  if (avg) _print_ts(vol->nimgs, tsavg);

  free(tsdata);
  free(tsavg);
  return 0;
fail:

  if (tsdata != NULL) free(tsdata);
  if (tsavg  != NULL) free(tsavg);
  return 1;
}

void _print_ts(uint16_t len, double *tsdata) {

  uint32_t i;
  for (i = 0; i < len; i++) printf("%0.4f ", tsdata[i]);
  printf("\n");
}
