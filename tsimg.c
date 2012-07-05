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
#include "timeseries/analyze_volume.h"

typedef struct _args {
  
  char    *input;
  char    *lblf;
  char    *ngdbf;

  uint8_t  allnode;
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
  {"ngdbf",   'f', "FILE",   0, "corresponding graph file"},
  {"allnode", 'n',  NULL,    0, "extract time series for all nodes in graph"},
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

    case 'f': args->lblf    = arg;       break;
    case 'l': args->bylbl   = 1;         break;
    case 'i': args->byidx   = 1;         break;
    case 'r': args->byreal  = 1;         break;
    case 'a': args->avg     = 1;         break;
    case 'v': args->lblval  = atoi(arg); break;
    case 'x': args->x       = atof(arg); break;
    case 'y': args->y       = atof(arg); break;
    case 'z': args->z       = atof(arg); break;

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


static void _print_ts(
  uint16_t len,
  double  *tsdata
);

static void _make_lbl_mask(
  dsr_t   *lblhdr,
  uint8_t *lblimg,
  uint8_t  mask,
  double   lblval
);

static void _make_idx_mask(
  dsr_t   *lblhdr,
  uint8_t *lblimg,
  double   lblval,
  uint8_t  mask  
);


static uint8_t _print_by_mask(
  analyze_volume_t *vol,
  uint8_t          *mask,
  uint8_t           avg
);

int main(int argc, char *argv[]) {

  analyze_volume_t vol;
  args_t           args;
  struct argp      argp = {options, _parse_opt, "OUTPUT [INPUT ...]", doc};

  memset(&args, 0, sizeof(args_t));

  startup("tsimg", argc, argv, &argp, &args);

  if (analyze_open_volume(args.input, &vol)) {
    printf("could not open ANALYZE75 volume %s\n", args.input);
    goto fail;
  }

  if (args.bylbl) _print_by_label(&vol, &args);
  else            _print_one(     &vol, &args);


  return 0;

fail:
  return 1;

}

void _print_ts(uint16_t len, double *tsdata) {

  uint32_t i;
  for (i = 0; i < len; i++) printf("%0.4f ", tsdata[i]);
  printf("\n");
}

uint8_t _print_one(analyze_volume_t *vol, args_t *args) {

  double *tsdata;
  uint32_t x;
  uint32_t y;
  uint32_t z;

  x = 0;
  y = 0;
  z = 0;

  tsdata = malloc(vol->nimgs * sizeof(double));
  if (tsdata == NULL) goto fail;
  
  if (args->byidx) {
    
    x = (uint32_t)round(args->x);
    y = (uint32_t)round(args->y);
    z = (uint32_t)round(args->z);
  }
  
  else if (args->byreal) {
    
    x = (uint32_t)round(args->x / analyze_pixdim_size(vol->hdrs, 0));
    y = (uint32_t)round(args->y / analyze_pixdim_size(vol->hdrs, 1));
    z = (uint32_t)round(args->z / analyze_pixdim_size(vol->hdrs, 2));
  }

  else goto fail;

  if (analyze_read_timeseries(vol, x, y, z, tsdata)) goto fail;

  _print_ts(vol->nimgs, tsdata);

  free(tsdata);

  return 0;

fail:
   if (tsdata != NULL) free(tsdata);
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
