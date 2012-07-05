/**
 * Extracts time series data from an ANALYZE75 image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <argp.h>
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
  {"lblf",   'f', "FILE",   0, "ANALYZE75 label file"},
  {"bylbl",  'l',  NULL,    0, "extract time series by voxel label"},
  {"byidx",  'i',  NULL,    0, "extract time series by xyz indices"},
  {"byreal", 'r',  NULL,    0, "extract time series by real xyz coordinates"},
  {"avg",    'a',  NULL,    0, "print average of all specified time series"},
  {"lblval", 'v', "INT",    0, "extract time series with this label"},
  {"x",      'x', "FLOAT",  0, "x index/coordinate"},
  {"y",      'y', "FLOAT",  0, "y index/coordinate"},
  {"z",      'z', "FLOAT",  0, "z index/coordinate"},
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



int main(int argc, char *argv[]) {

  args_t      args;
  struct argp argp = {options, _parse_opt, "OUTPUT [INPUT ...]", doc};

  startup("tsimg", argc, argv, &argp, &args);


  return 0;
}
