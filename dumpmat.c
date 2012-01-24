/**
 * Prints out the data in, or information about, a .mat file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "io/mat.h"
#include "util/startup.h"
#include "graph/graph.h"

typedef struct _args {

  char   *input;
  uint8_t meta;
  uint8_t stats;
  uint8_t labels;
  uint8_t data;

} args_t;

static char doc[] =
"dumpmat -- print the contents of a .mat file";

static struct argp_option options[] = {
  {"meta",   'm', NULL, 0, "print information about the file"},
  {"stats",  's', NULL, 0, "print basic data statistics"},
  {"labels", 'l', NULL, 0, "print row/column labels"},
  {"data",   'd', NULL, 0, "print the data in the file"},
  {0},
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch(key) {
    
    case 'm': args->meta   = 1; break;
    case 's': args->stats  = 1; break;
    case 'l': args->labels = 1; break;
    case 'd': args->data   = 1; break;
      
    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->input = arg;
      else argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

/**
 * Prints file header information.
 */
static void _print_meta(
  mat_t *mat /**< mat file to query */
);

/**
 * Calculates and prints basic statistics on the data.
 */
static void _print_stats(
  mat_t *mat /**< mat file to query */
);

/**
 * Prints row/column labels.
 */
static void _print_labels(
  mat_t *mat /**< mat file to query */
);

/**
 * Prints the matrix data.
 */
static void _print_data(
  mat_t *mat /**< mat file to query */
);


int main (int argc, char *argv[]) {

  struct argp argp = {options, _parse_opt, "INPUT", doc};
  args_t      args;
  mat_t      *mat;

  memset(&args, 0, sizeof(args));

  startup("dumpmat", argc, argv, &argp, &args);

  mat = mat_open(args.input);
  if (mat == NULL) {

    printf("error opening %s\n", args.input);
    goto fail;
  }

  if (args.meta)   _print_meta(  mat);
  if (args.stats)  _print_stats( mat);
  if (args.labels) _print_labels(mat);
  if (args.data)   _print_data(  mat);

  mat_close(mat);
  return 0;

fail:
  return 1;
}

static void _print_meta(mat_t *mat) {

  uint16_t hdrsize;
  char    *hdrdata;

  hdrdata = NULL;
  hdrsize = mat_hdr_data_size(mat);

  printf("rows:           %llu\n", mat_num_rows(    mat));
  printf("cols:           %llu\n", mat_num_cols(    mat));
  printf("hdr data size:  %u\n", hdrsize);
  printf("label size:     %u\n", mat_label_size(    mat));
  printf("symmetric:      %u\n", mat_is_symmetric(  mat));
  printf("has row labels: %u\n", mat_has_row_labels(mat));
  printf("has col labels: %u\n", mat_has_col_labels(mat));

  if (hdrsize > 0) {
    
    hdrdata = calloc(hdrsize+1, 1);
    if (hdrdata == NULL) goto fail;

    if (mat_read_hdr_data(mat, hdrdata)) {
      printf("error reading header data\n");
      goto fail;
    }

    hdrdata[hdrsize] = '\0';

    printf("hdr data:\n\n");
    printf("%s", hdrdata);
    printf("\n\n");

    free(hdrdata);
  }

  return;

fail:
  if (hdrdata != NULL) free(hdrdata);
}

static void _print_stats(mat_t *mat) {

}

static void _print_labels(mat_t *mat) {

  uint64_t      i;
  uint64_t      nrows;
  uint64_t      ncols;
  graph_label_t label;

  nrows = mat_num_rows(mat);
  ncols = mat_num_cols(mat);
  

  if (mat_has_row_labels(mat)) {
    for (i = 0; i < nrows; i++) {
      if (mat_read_row_label(mat, i, &label)) {
        printf("error reading row label %llu\n", i);
        break;
      }
      printf(
        "row %5llu: %0.3f %0.3f %0.3f %u\n",
        i, label.xval, label.yval, label.zval, label.labelval);
    }
  }

  if (mat_has_col_labels(mat)) {
    for (i = 0; i < ncols; i++) {
      if (mat_read_col_label(mat, i, &label)) {
        printf("error reading col label %llu\n", i);
        break;
      }
      printf(
        "col %5llu: %0.3f %0.3f %0.3f %u\n",
        i, label.xval, label.yval, label.zval, label.labelval);
    }
  }
}

static void _print_data(mat_t *mat) {

  uint64_t i;
  uint64_t j;
  uint64_t nrows;
  uint64_t ncols;
  double  *rowvals;

  
  rowvals = NULL;
  nrows   = mat_num_rows(mat);
  ncols   = mat_num_cols(mat);

  rowvals = malloc(ncols*sizeof(double));

  for (i = 0; i < nrows; i++) {

    if (mat_read_row(mat, i, rowvals)) {
      printf("error reading row %llu data\n", i);
      goto fail;
    }

    for (j = 0; j < ncols; j++) {
      
      printf("%0.3f", rowvals[j]);
      if (j < ncols - 1) printf(" ");
    }
    printf("\n");
  }

  free(rowvals);
  return;
  
fail:
  if (rowvals != NULL) free(rowvals);
}
