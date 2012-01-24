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
#include "graph/graph.h"
#include "timeseries/correlation.h"
#include "timeseries/analyze_volume.h"

typedef enum {
  CORRTYPE_PEARSON,
  CORRTYPE_COHERENCE
} corrtype_t;

#define MAX_LABELS      50
#define MAT_HDR_DATA_SZ 512

typedef struct __args {

  char    *input;
  char    *output;
  char    *hdrmsg;
  char    *labelf;
  char    *maskf;
  double  *lothres;
  double  *hithres;
  double   lothresval;
  double   hithresval;
  uint8_t  corrtype;
  uint8_t  ninclbls;
  uint8_t  nexclbls;
  
  double   inclbls[MAX_LABELS];
  double   exclbls[MAX_LABELS];

} args_t;

static char doc[] =
"tsmat -- generate a correlation matrix from an ANALYZE75 volume";

static struct argp_option options[] = {
  {"hdrmsg",  's', "MSG",   0, "message to save to .mat file header"},
  {"labelf",  'f', "FILE",  0,
   "ANALYZE75 label file (must have same datat type as volume files)"},
  {"maskf",   'm', "FILE",  0,
   "ANALYZE75 mask file (must have same data type as volume files)"},
  {"lothres", 'l', "FLOAT", 0, "low threshold"},
  {"hithres", 'h', "FLOAT", 0, "high threshold"},
  {"pcorr",   'p', NULL,    0, "use pearson correlation"},
  {"cohe",    'c', NULL,    0, "use coherence"},
  {"incl",    'i', "FLOAT", 0, "include only voxels with this label"},
  {"excl",    'e', "FLOAT", 0, "exclude voxels with this label"},
  {0}
};

/**
 * Figures out which voxels to include in the correlation matrix. Creates an
 * array to store the indices of these voxels, and points the given incvxls
 * pointer to this array. Voxel inclusion is determined by a combination of
 * the low/high thresholds, the include/exclude labels, and the mask file (in
 * that order), all specified in the program arguments.
 *
 * \return number of voxels that have been included on success, -1 on failure.
 */
static int64_t _create_mask(
  analyze_volume_t *vol,     /**< volume containing time series  */
  uint32_t        **incvxls, /**< place to store indices
                                  of voxels to include           */
  args_t           *args,    /**< tsmat program arguments        */
  dsr_t            *hdr,     /**< ANALYZE75 label header         */
  uint8_t          *img      /**< ANALYZE75 label data           */
);

/**
 * Updates the given mask array by excluding all voxels which do not lie
 * within the specified low/high threshold values.
 *
 * \return number of voxels that have been masked on success, -1 on failure. 
 */
static int64_t _apply_threshold_mask(
  analyze_volume_t *vol,     /**< time series volume              */
  uint8_t          *mask,    /**< mask array                      */
  double           *lothres, /**< pointer to low threshold value  */
  double           *hithres  /**< pointer to high threshold value */
);

/**
 * Updates the given mask array by excluding all voxels which either:
 *
 *   1. are contained within the exclbls list, or
 *   2. are not contained within the inclbls list.
 *
 * \return number of voxels that have been masked on success, -1 on failure.
 */
static int64_t _apply_label_mask(
  analyze_volume_t *vol,      /**< time series volume               */
  uint8_t          *mask,     /**< mask array                       */
  dsr_t            *hdr,      /**< ANALYZE75 label header           */
  uint8_t          *img,      /**< ANALYZE75 label data             */
  double           *inclbls,  /**< list of labels to include        */
  double           *exclbls,  /**< list of labels to exclude        */
  uint8_t           ninclbls, /**< number of values in include list */
  uint8_t           nexclbls  /**< number of values in exclude list */
);

/**
 * Updates the given mask array from the given mask file, which is
 * assumed to be an ANALYZE75 image file. Voxels from the volume are
 * masked if the corresponding mask voxel has a value of 0.
 *
 * \return number of voxels that have been masked on success, -1 on failure.
 */
static int64_t _apply_file_mask(
  analyze_volume_t *vol,  /**< volume containing time series  */
  uint8_t          *mask, /**< mask array (already allocated) */
  char             *maskf /**< mask file                      */  
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
  double   *inclbls,  /**< include list                     */
  double   *exclbls,  /**< exclude list                     */
  uint8_t   ninclbls, /**< number of labels in include list */
  uint8_t   nexclbls, /**< number of labels in exclude list */
  double    labelval  /**< label value to check             */
);

/**
 * Writes label data from the given label file to the matrix,
 * for all voxels in the incvxls list.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _write_labels(
  dsr_t    *hdr,     /**< label header              */
  uint8_t  *img,     /**< label data                */
  mat_t    *mat,     /**< matrix file               */
  uint32_t *incvxls, /**< voxels to include         */
  uint32_t  nincvxls /**< number of included voxels */
);

/**
 * Calculates a correlation value between all pairs of time series,
 * storing the values in the given mat file, which is assumed to
 * have already been created.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mk_corr_matrix(
  analyze_volume_t *vol,      /**< time series volume           */
  mat_t            *mat,      /**< mat file ready for writing   */
  corrtype_t        corrtype, /**< correlation measure to use   */
  uint32_t         *incvxls,  /**< indices of voxels to include */
  uint32_t          nincvxls  /**< number of included voxels    */
);

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {
    case 'f': args->labelf   = arg;                break;
    case 'm': args->maskf    = arg;                break;
    case 's': args->hdrmsg   = arg;                break;
    case 'p': args->corrtype = CORRTYPE_PEARSON;   break;
    case 'c': args->corrtype = CORRTYPE_COHERENCE; break;
    case 'l':
      args->lothresval = atof(arg);
      args->lothres    = &(args->lothresval);
      break;
    case 'h':
      args->hithresval = atof(arg);
      args->hithres    = &(args->hithresval);
      break; 
      
    case 'i':
      if (args->ninclbls < MAX_LABELS) 
        args->inclbls[(args->ninclbls)++] = atof(arg);
      break;
      
    case 'e':
      if (args->nexclbls < MAX_LABELS) 
        args->exclbls[(args->nexclbls)++] = atof(arg);
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

  uint32_t         nincvxls;
  uint32_t        *incvxls;
  analyze_volume_t vol;
  mat_t           *mat;
  dsr_t            lblhdr;
  dsr_t           *hdrs[2];
  uint8_t         *lblimg;
  args_t           args;
  struct argp      argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  lblimg  = NULL;
  incvxls = NULL;
  mat     = NULL;

  memset(&args, 0, sizeof(args));

  startup("tsmat", argc, argv, &argp, &args);

  if (analyze_open_volume(args.input, &vol)) {
    printf("error opening analyze volume from %s\n", args.input);
    goto fail;
  }

  if (args.labelf != NULL) {
    
    if (analyze_load(args.labelf, &lblhdr, &lblimg)) {
      printf("error loading label file %s\n", args.labelf);
      goto fail;
    }

    hdrs[0] = vol.hdrs;
    hdrs[1] = &lblhdr;

    if (!analyze_hdr_compat_ptr(2, hdrs)) {
      printf(
        "label file %s does not match volume files in %s\n",
        args.labelf, args.input);
      goto fail;
    }
  }

  nincvxls = _create_mask(&vol, &incvxls, &args, &lblhdr, lblimg);
  
  if (nincvxls < 0) {
    printf("error masking voxels\n");
    goto fail;
  }

  mat = mat_create(
    args.output, nincvxls, nincvxls,
    (1 << MAT_IS_SYMMETRIC) | (1 << MAT_HAS_ROW_LABELS),
    MAT_HDR_DATA_SZ,
    sizeof(graph_label_t));

  if (mat == NULL) {
    printf("error creating mat file %s\n", args.output);
    goto fail;
  }

  if (_mk_corr_matrix(&vol, mat, args.corrtype, incvxls, nincvxls)) {
    printf("error creating correlation matrix\n");
    goto fail;
  }

  mat_close(mat);
  analyze_free_volume(&vol);
  free(lblimg);
  free(incvxls);

  return 0;
  
fail:

  if (mat != NULL) mat_close(mat);
  return 1;
}

int64_t _create_mask(
  analyze_volume_t *vol,
  uint32_t        **incvxls,
  args_t           *args,
  dsr_t            *hdr,
  uint8_t          *img) {

  uint64_t  i;
  uint64_t  j;
  uint8_t  *mask;
  uint32_t *lincvxls;
  uint32_t  nvals;
  uint32_t  nincvxls;
  int64_t   result;

  lincvxls = NULL;
  mask     = NULL;
  nvals    = analyze_num_vals(vol->hdrs);
  nincvxls = nvals;

  mask = malloc(nvals);
  if (mask == NULL) goto fail;

  /*all voxels are included initially*/
  memset(mask, 1, nvals);

  /*masking via low/high time series threshold*/
  if (args->lothres != NULL || args->hithres != NULL)  {
    
    result = _apply_threshold_mask(vol, mask, args->lothres, args->hithres);
    if (result < 0) goto fail;
    nincvxls -= result;
  }

  /*masking via label file*/
  if (args->labelf != NULL) {
    
    result = _apply_label_mask(
      vol,
      mask,
      hdr,
      img,
      args->inclbls,
      args->exclbls,
      args->ninclbls,
      args->nexclbls);
    
    if (result < 0) goto fail;
    nincvxls -= result;
  }

  /*masking via mask file*/
  if (args->maskf != NULL) {
    
    result = _apply_file_mask(vol, mask, args->maskf);
    
    if (result < 0) goto fail;
    nincvxls -= result;
  }

  /*store the indices of voxels to be included in the correlation matrix*/
  lincvxls = malloc(nincvxls * sizeof(uint32_t));
  if (lincvxls == NULL) goto fail;

  for (i = 0, j = 0; i < nvals; i++) {
    if (mask[i] != 0) lincvxls[j++] = i;
  }

  free(mask);
  *incvxls = lincvxls;
  return nincvxls;
  
fail:
  if (mask     != NULL) free(mask);
  if (lincvxls != NULL) free(lincvxls);
  *incvxls = NULL;
  return -1;
}

int64_t _apply_threshold_mask(
  analyze_volume_t *vol,
  uint8_t          *mask,
  double           *lothres,
  double           *hithres) {

  double  *tsdata;
  uint32_t masked;
  uint64_t i;
  uint32_t nvals;

  tsdata = calloc(vol->nimgs, sizeof(double));
  if (tsdata == NULL) goto fail;

  nvals  = analyze_num_vals(vol->hdrs);
  masked = 0;

  for (i = 0; i < nvals; i++) {

    /*voxel has already been masked*/
    if (mask[i] == 0) continue;

    if (analyze_read_timeseries_by_idx(vol, i, tsdata))
      goto fail;

    if (!_threshold(lothres, hithres, tsdata, vol->nimgs)) {
      masked ++;
      mask[i] = 0;
    }
  }
  
  free(tsdata);
  return masked;

fail:
  if (tsdata != NULL) free(tsdata);
  return -1;
}

static int64_t _apply_label_mask(
  analyze_volume_t *vol,
  uint8_t          *mask,
  dsr_t            *hdr,
  uint8_t          *img,
  double           *inclbls,
  double           *exclbls,
  uint8_t           ninclbls,
  uint8_t           nexclbls
) {

  double   lblval;
  uint64_t i;
  uint32_t masked;
  uint32_t nvals;

  nvals  = analyze_num_vals(vol->hdrs);
  masked = 0;
  
  for (i = 0; i < nvals; i++) {

    if (mask[i] == 0) continue;

    lblval = analyze_read_by_idx(hdr, img, i);

    if (!_check_label(inclbls, exclbls, ninclbls, nexclbls, lblval)) {
      mask[i] = 0;
      masked ++;
    }
  }

  return masked;
}

int64_t _apply_file_mask(
  analyze_volume_t *vol,
  uint8_t          *mask,
  char             *maskf) {

  dsr_t   *hdrs[2];
  dsr_t    maskhdr;
  uint8_t *maskimg;
  double   maskval;  
  uint8_t  maskvalsz;
  uint64_t i;
  uint32_t masked;
  uint32_t nvals;

  maskimg = NULL;
  nvals   = analyze_num_vals(vol->hdrs);
  masked  = 0;

  if (analyze_load(maskf, &maskhdr, &maskimg)) goto fail;

  hdrs[0] = &maskhdr;
  hdrs[1] = vol->hdrs;

  if (analyze_hdr_compat_ptr(2, hdrs)) goto fail;

  maskvalsz = analyze_num_vals(&maskhdr);

  for (i = 0; i < nvals; i++) {

    if (mask[i] == 0) continue;

    maskval = analyze_read_by_idx(&maskhdr, maskimg, i);

    if (maskval == 0) {
      mask[i] = 0;
      masked ++;
    }
  }  

  free(maskimg);
  return masked;
  
fail:
  if (maskimg != NULL) free(maskimg);
  return -1;
}

uint8_t _threshold(
  double  *lothres,
  double  *hithres,
  double  *tsdata,
  uint32_t len) {

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
  double   *inclbls,
  double   *exclbls,
  uint8_t   ninclbls,
  uint8_t   nexclbls,
  double    lblval
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

static uint8_t _write_labels(
  dsr_t    *hdr,
  uint8_t  *img,
  mat_t    *mat,
  uint32_t *incvxls,
  uint32_t  nincvxls) {

  uint64_t      i;
  graph_label_t label;

  for (i = 0; i < nincvxls; i++) {

    if (mat_write_row_label(mat, i, &label)) goto fail;
  }

  return 0;

fail:

  return 1;
}

uint8_t _mk_corr_matrix(
  analyze_volume_t *vol,
  mat_t            *mat,
  corrtype_t        corrtype,
  uint32_t         *incvxls,
  uint32_t          nincvxls
) {

  uint64_t  row;
  uint64_t  col;
  uint32_t  len;
  double    corrval;
  double   *rowtsdata;
  double   *coltsdata;
  
  rowtsdata = NULL;
  coltsdata = NULL;

  len = vol->nimgs;

  rowtsdata = malloc(len*sizeof(double));
  if (rowtsdata == NULL) goto fail;
  coltsdata = malloc(len*sizeof(double));
  if (coltsdata == NULL) goto fail;

  for (row = 0; row < nincvxls; row++) {
    
    if (analyze_read_timeseries_by_idx(vol, incvxls[row], rowtsdata))
      goto fail;
    
    for (col = row; col < nincvxls; col++) {

      if (col == row) {
        if (mat_write_elem(mat, row, col, 0.0)) goto fail;
        continue;
      }

      if (analyze_read_timeseries_by_idx(vol, incvxls[col], coltsdata))
        goto fail;

      corrval = pearson(rowtsdata, coltsdata, len);

      if (mat_write_elem(mat, row, col, corrval))
        goto fail;
    }
  }

  free(rowtsdata);
  free(coltsdata);
  return 0;
  
fail:
  if (rowtsdata != NULL) free(rowtsdata);
  if (coltsdata != NULL) free(coltsdata);
  
  return 1;
}
