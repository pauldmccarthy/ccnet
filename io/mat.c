/**
 *   mat - a simple binary matrix file format.
 *
 * A simple file format for the storage and access of rectangular 2D double
 * matrices. See README.MAT for details.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include "io/mat.h"

#define MAT_FILE_ID 0x8493

typedef enum __mat_mode {

  MAT_MODE_READ,
  MAT_MODE_CREATE
} mat_mode_t;

struct __mat {
  
  FILE      *hd;
  uint64_t   numrows;
  uint64_t   numcols;
  uint16_t   flags;
  uint8_t    labelsize;
  mat_mode_t mode;
};


static uint8_t _mat_read_header(
  mat_t *mat
);

static uint8_t _mat_write_header(
  mat_t *mat
);

static uint8_t _mat_seek(
  mat_t   *mat,
  uint64_t row,
  uint64_t col
);



mat_t * mat_open(char *fname) {

  mat_t *mat;

  mat = NULL;

  if (fname == NULL) goto fail;

  mat = malloc(sizeof(mat_t));
  if (mat == NULL) goto fail;

  mat->hd = fopen(fname, "rb");
  if (mat->hd == NULL) goto fail;

  if (_mat_read_header(mat)) goto fail;

  mat->mode = MAT_MODE_READ;

  return mat;

fail:

  if (mat != NULL) {
    if (mat->hd != NULL) fclose(mat->hd);
    free(mat);
  }
  return NULL;
}

uint8_t mat_close(mat_t *mat) {

  if (mat     == NULL) goto fail;
  if (mat->hd == NULL) goto fail;

  fclose(mat->hd);
  mat->hd = NULL;
  free(mat);

  return 0;
fail:
  return 1;
}

uint64_t mat_num_rows(mat_t *mat) {

  return mat->numrows;
}

uint64_t mat_num_cols(mat_t *mat) {

  return mat->numcols;
}

uint8_t mat_is_symmetric(mat_t *mat) {
  
  return (mat->flags) & MAT_IS_SYMMETRIC;
}

uint8_t mat_has_row_labels(mat_t *mat) {
  
  return (mat->flags) & MAT_HAS_ROW_LABELS;
}

uint8_t mat_has_col_labels(mat_t *mat) {

  return (mat->flags) & MAT_HAS_COL_LABELS;
}

double mat_read_elem(mat_t *mat, uint64_t row, uint64_t col) {

  double val;

  if (mat_read_row_part(mat, row, col, 1, &val)) return -DBL_MAX;
  else                                           return val;
}

uint8_t mat_read_row(mat_t *mat, uint64_t row, double *vals) {

  return mat_read_row_part(mat, row, 0, mat->numcols, vals);
}

uint8_t mat_read_row_part(
  mat_t   *mat,
  uint64_t row,
  uint64_t col,
  uint64_t len,
  double  *vals) {

  uint64_t rowlen;
  uint64_t collen;

  if (mat       == NULL)         goto fail;
  if (row       <  0)            goto fail;
  if (col       <  0)            goto fail;
  if (row       >= mat->numrows) goto fail;
  if (col       >= mat->numcols) goto fail;
  if (col + len >= mat->numcols) goto fail;

  if (!mat_is_symmetric(mat) || (col >= row)) {

    if (_mat_seek(mat, row, col))                         goto fail;
    if (fread(vals, sizeof(double), len, mat->hd) != len) goto fail;
  }
  
  else {

    collen = row - col;
    rowlen = col + len - row;

    /* translate reads for bottom left of matrix into top right */
    if (mat_read_col_part(mat, col, row, collen, vals))
      goto fail;

    /*read values from top right of matrix as normal */
    if (_mat_seek(mat, row, row)) goto fail;
    
    if (fread(vals+collen, sizeof(double), rowlen, mat->hd) != rowlen)
      goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t mat_read_col(mat_t *mat, uint64_t col, double *vals) {

  return mat_read_col_part(mat, 0, col, mat->numrows, vals);
}

uint8_t mat_read_col_part(
  mat_t   *mat,
  uint64_t row,
  uint64_t col,
  uint64_t len,
  double  *vals) {

  uint64_t i;
  
  if (mat       == NULL)         goto fail;
  if (row       <  0)            goto fail;
  if (col       <  0)            goto fail;
  if (row       >= mat->numrows) goto fail;
  if (col       >= mat->numcols) goto fail;
  if (row + len >= mat->numrows) goto fail;

  for (i = 0; i < len; i++, row++) {

    if (_mat_seek(mat, row, col))                       goto fail;
    if (fread(vals+i, sizeof(double), 1, mat->hd) != 1) goto fail;
  }

  return 0;
  
fail:
  return 1;
}





mat_t * mat_create(
  char    *fname,
  uint64_t numrows,
  uint64_t numcols,
  uint16_t flags,
  uint8_t  labelsize) {


  return NULL;
}









uint8_t _mat_read_header(mat_t *mat) {

  uint16_t id;

  rewind(mat->hd);

  if (fread(&id, sizeof(id), 1, mat->hd) != 1) goto fail;
  if (id != MAT_FILE_ID)                       goto fail;

  if (fread(&(mat->numrows),   sizeof(mat->numrows),   1, mat->hd) != 1)
    goto fail; 
  if (fread(&(mat->numcols),   sizeof(mat->numcols),   1, mat->hd) != 1)
    goto fail;
  if (fread(&(mat->flags),     sizeof(mat->flags),     1, mat->hd) != 1)
    goto fail; 
  if (fread(&(mat->labelsize), sizeof(mat->labelsize), 1, mat->hd) != 1)
    goto fail; 

  return 0;
  
fail:
  return 1;
}

uint8_t _mat_write_header(mat_t *mat) {

  uint16_t id;

  rewind(mat->hd);

  id = MAT_FILE_ID;

  if (fwrite(&(id),             sizeof(id),             1, mat->hd) != 1)
    goto fail;
  if (fwrite(&(mat->numrows),   sizeof(mat->numrows),   1, mat->hd) != 1)
    goto fail;
  if (fwrite(&(mat->numcols),   sizeof(mat->numcols),   1, mat->hd) != 1)
    goto fail; 
  if (fwrite(&(mat->flags),     sizeof(mat->flags),     1, mat->hd) != 1)
    goto fail; 
  if (fwrite(&(mat->labelsize), sizeof(mat->labelsize), 1, mat->hd) != 1)
    goto fail;


  return 0;
fail:
  return 1;
}

uint8_t _mat_seek(mat_t *mat, uint64_t row, uint64_t col) {

  return 0;
  
fail:
  return 1;
}
