/**
 * Generates a correlation matrix from an ANALYZE75 volume, saving it as a
 * symmetric MAT file.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <argp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "io/mat.h"
#include "io/analyze75.h"
#include "util/startup.h"
#include "timeseries/analyze_volume.h"

typedef enum {
  CORRTYPE_PEARSON,
  CORRTYPE_COHERENCE
} corrtype_t;

#define MAX_LABELS 50

typedef struct __args {

  char    *input;
  char    *output;
  char    *labelf;
  char    *maskf;
  double   lothres;
  double   hithres;
  uint8_t  corrtype;
  uint8_t  ninclabels;
  uint8_t  nexclabels;
  uint32_t inclabels[MAX_LABELS];
  uint32_t exclabels[MAX_LABELS];

} args_t;

static char doc[] =
"tsmat -- generate a correlation matrix from an ANALYZE75 volume";

static struct argp_option options[] = {
  {"labelf",  'f', "FILE",  0, "ANALYZE75 label file"},
  {"maskf",   'm', "FILE",  0, "ANALYZE75 mask file"},
  {"lothres", 'l', "FLOAT", 0, "low threshold"},
  {"hithres", 'h', "FLOAT", 0, "high threshold"},
  {"pcorr",   'p', NULL,    0, "use pearson correlation"},
  {"cohe",    'c', NULL,    0, "use coherence"},
  {"incl",    'i', "INT",   0, "include only voxels with this label"},
  {"excl",    'e', "INT",   0, "exclude voxels with this label"},
  {0}
};

/**
 * Populates the given mask array, which is used to determine voxel inclusion.
 * The mask array is a 1D array, containing a single value for every voxel in
 * the time series volume. A value of 1 in the mask indicates that the
 * corresponding voxel is to be included in the correlation matrix. Dimension
 * order in the mask array (from slowest changing to fastest changing) is
 * [x,y,z].
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _create_mask(
  analyze_volume_t *vol,  /**< volume containing time series  */
  uint8_t          *mask, /**< mask array (already allocated) */
  args_t           *args  /**< tsmat program arguments        */
);

/**
 * Checks the given time series data to see if it lies within the given
 * threshold range.
 *
 * \return 1 if the time series lies within the threshold
 * values and should be included, 0 otherwise.
 */
static uint8_t _threshold(
  double  *lothres, /**< pointer to low threshold value,
                         NULL if no low threshold        */
  double  *hithres, /**< pointer to high threshold value,
                         NULL if no high threshold       */
  double  *tsdata,  /**< time series to be checked       */
  uint32_t len      /**< length of time series           */
);

/**
 * \return 1 if the label should be included, 0 otherwise.
 */
static uint8_t _check_label(
  uint32_t *inclbls,  /**< include list                     */
  uint32_t *exclbls,  /**< exclude list                     */
  uint32_t  ninclbls, /**< number of labels in include list */
  uint32_t  nexclbls, /**< number of labels in exclude list */
  uint32_t  labelval  /**< label value to check             */
);

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {
    case 'f': args->labelf   = arg;                break;
    case 'm': args->maskf    = arg;                break;
    case 'l': args->lothres  = atof(arg);          break;
    case 'h': args->hithres  = atof(arg);          break;
    case 'p': args->corrtype = CORRTYPE_PEARSON;   break;
    case 'c': args->corrtype = CORRTYPE_COHERENCE; break;
      
    case 'i':
      if (args->ninclabels < MAX_LABELS) 
        args->inclabels[(args->ninclabels)++] = atoi(arg);
      break;
      
    case 'e':
      if (args->nexclabels < MAX_LABELS) 
        args->exclabels[(args->nexclabels)++] = atoi(arg);
      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
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

  uint8_t         *mask;
  analyze_volume_t vol;
  mat_t           *mat;
  args_t           args;
  struct argp      argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  mask = NULL;
  mat  = NULL;

  memset(&args, 0, sizeof(args));

  startup("tsmat", argc, argv, &argp, &args);

  if (analyze_open_volume(args.input, &vol)) {
    printf("error opening analyze volume from %s\n", args.input);
    goto fail;
  }

  mask = calloc(analyze_num_vals(vol.hdrs),1);
  if (mask == NULL) {
    printf("error allocating mask array\n");
  }

  if (_create_mask(&vol, mask, &args)) {
    printf("error creating mask array\n");
    goto fail;
  }


  return 0;
  
fail:
  return 1;
}

uint8_t _create_mask(analyze_volume_t *vol, uint8_t *mask, args_t *args) {

  return 0;
}

uint8_t _threshold(
  double *lothres, double *hithres, double *tsdata, uint32_t len) {

  uint64_t i;

  for (i = 0; i < len; i++) {

    if ((lothres != NULL) && (hithres != NULL)) {
      if ((tsdata[i] >= *lothres) && (tsdata[i] <= *hithres)) return 1;
    }
    else if (lothres != NULL) {
      if (tsdata[i] >= *lothres) return 1;
    }
    else if (hithres != NULL) {
      if (tsdata[i] <= *hithres) return 1;
    }
  }

  return 0;
}

uint8_t _check_label(
  uint32_t *inclbls,
  uint32_t *exclbls,
  uint32_t  ninclbls,
  uint32_t  nexclbls,
  uint32_t  lblval
) {

  uint64_t i;

  if (ninclbls == 0 && nexclbls == 0) return 1;

  for (i = 0; i < nexclbls; i++) {
    if (lblval == exclbls[i]) return 0;
  }

  if (ninclbls == 0) return 1;

  for (i = 0; i < ninclbls; i++) {
    if (lblval == inclbls[i]) return 1;
  }

  return 0;
}
