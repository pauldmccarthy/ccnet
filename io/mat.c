/**
 *   mat - a simple binary matrix file format.
 *
 * A simple file format for the storage and access of rectangular 2D double
 * matrices. See README.MAT for details.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <math.h>
#include <stdio.h>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include "io/mat.h"

#define MAT_FILE_ID 0x8493

#define MAT_HDR_SIZE 21

typedef enum __mat_mode {

  MAT_MODE_READ,
  MAT_MODE_CREATE
} mat_mode_t;

/**
 * Structure representing an open mat file.
 */
struct __mat {
  
  FILE      *hd;        /**< handle to the file         */
  uint64_t   numrows;   /**< number of rows             */
  uint64_t   numcols;   /**< number of columns          */
  uint16_t   flags;     /**< option flags               */
  uint8_t    labelsize; /**< label size                 */
  mat_mode_t mode;      /**< open mode (read or create) */
};

/**
 * Reads header information from the mat->hd file into the
 * provided mat_t struct.
 *
 * \return 0 on success,
 */
static uint8_t _mat_read_header(
  mat_t *mat /**< mat struct with an open file handle */
);

/**
 * Writes header information from the provided mat_t struct
 * into the mat->hd file.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mat_write_header(
  mat_t *mat /**< mat struct with an open file handle */
);

/**
 * Calculates the offset into the mat->hd file for the given row/column.
 *
 * \return the offset into the mat file.
 */
static uint64_t _mat_calc_offset(
  mat_t   *mat,
  uint64_t row,
  uint64_t col
);

/**
 * Seeks to the given row/column in the given mat->hd file.
 */
static uint8_t _mat_seek(
  mat_t   *mat, /**< mat struct with an open file */
  uint64_t row, /**< row to seek to               */
  uint64_t col  /**< column to seek to            */
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
  
  return (mat->flags >> MAT_IS_SYMMETRIC) & 1;
}

uint8_t mat_has_row_labels(mat_t *mat) {
  
  return (mat->flags >> MAT_HAS_ROW_LABELS) & 1;
}

uint8_t mat_has_col_labels(mat_t *mat) {

  return (mat->flags >> MAT_HAS_COL_LABELS) & 1;
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

  if (mat       == NULL)          goto fail;
  if (mat->mode != MAT_MODE_READ) goto fail;
  if (vals      == NULL)          goto fail;
  if (row       <  0)             goto fail;
  if (col       <  0)             goto fail;
  if (row       >= mat->numrows)  goto fail;
  if (col       >= mat->numcols)  goto fail;
  if (col + len >= mat->numcols)  goto fail;

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
  
  if (mat       == NULL)          goto fail;
  if (mat->mode != MAT_MODE_READ) goto fail;
  if (vals      == NULL)          goto fail;
  if (row       <  0)             goto fail;
  if (col       <  0)             goto fail;
  if (row       >= mat->numrows)  goto fail;
  if (col       >= mat->numcols)  goto fail;
  if (row + len >= mat->numrows)  goto fail;

  for (i = 0; i < len; i++, row++) {

    if (_mat_seek(mat, row, col))                       goto fail;
    if (fread(vals+i, sizeof(double), 1, mat->hd) != 1) goto fail;
  }

  return 0;
  
fail:
  return 1;
}

uint8_t mat_read_row_label(mat_t *mat, uint64_t row, void *data) {

  uint64_t off;

  if (!mat_has_row_labels(mat))       goto fail;
  if (row            <  0)            goto fail;
  if (row            >= mat->numrows) goto fail;
  if (mat->labelsize == 0)            goto fail;

  off = MAT_HDR_SIZE + ((mat->labelsize) * row);

  if (fread(data, mat->labelsize, 1, mat->hd) != 1) goto fail;
  
  return 0;
  
fail:
  return 1;
}

uint8_t mat_read_col_label(mat_t *mat, uint64_t col, void *data) {

  uint64_t off;
  uint64_t rlbloff;

  if (!mat_has_col_labels(mat))       goto fail;
  if (col            <  0)            goto fail;
  if (col            >= mat->numcols) goto fail;
  if (mat->labelsize == 0)            goto fail;


  if (mat_has_row_labels(mat)) rlbloff = (mat->labelsize) * (mat->numrows);
  else                         rlbloff = 0;

  off = MAT_HDR_SIZE + rlbloff + ((mat->labelsize) * col);

  if (fread(data, mat->labelsize, 1, mat->hd) != 1) goto fail;

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

  mat_t *mat;
  mat = NULL;

  mat = calloc(1, sizeof(mat_t));
  if (mat == NULL) goto fail;

  mat->numrows   = numrows;
  mat->numcols   = numcols;
  mat->flags     = flags;
  mat->labelsize = labelsize;
  mat->mode      = MAT_MODE_CREATE;

  if (numrows == 0)                                  goto fail;
  if (numcols == 0)                                  goto fail;
  if (mat_is_symmetric(mat) && (numrows != numcols)) goto fail;
  if (mat_has_row_labels(mat) && labelsize == 0)     goto fail;
  if (mat_has_col_labels(mat) && labelsize == 0)     goto fail;
  
  mat->hd = fopen(fname, "wb");
  if (mat->hd == NULL) goto fail;

  if (_mat_write_header(mat)) goto fail;

  return mat;

fail:
  if (mat != NULL) {
    if (mat->hd != NULL) fclose(mat->hd);
    free(mat);
  }
  return NULL;
}


uint8_t mat_write_elem(mat_t *mat, uint64_t row, uint64_t col, double val) {

  return mat_write_row_part(mat, row, col, 1, &val);
}

uint8_t mat_write_row(mat_t *mat, uint64_t row, double *vals) {
  
  return mat_write_row_part(mat, row, 0, mat->numcols, vals);
}

uint8_t mat_write_row_part(
  mat_t *mat, uint64_t row, uint64_t col, uint64_t len, double *vals) {

  uint64_t rowlen;
  uint64_t collen;

  if (mat       == NULL)            goto fail;
  if (mat->mode != MAT_MODE_CREATE) goto fail;
  if (vals      == NULL)            goto fail;
  if (row       <  0)               goto fail;
  if (col       <  0)               goto fail;
  if (row       >= mat->numrows)    goto fail;
  if (col       >= mat->numcols)    goto fail;
  if (col + len >  mat->numcols)    goto fail;

  if (!mat_is_symmetric(mat) || (col >= row)) {

    if (_mat_seek(mat,row,col))                            goto fail;
    if (fwrite(vals, sizeof(double), len, mat->hd) != len) goto fail;
  }

  else {

    rowlen = col + len - row;
    collen = row - col;

    if (mat_write_col_part(mat, col, row, collen, vals))      goto fail;
    if (_mat_seek(mat, row, row))                             goto fail;
    if (fwrite(vals+collen, sizeof(double), rowlen, mat->hd)) goto fail;
  }

  return 0;
  
fail:
  return 1;
}

uint8_t mat_write_col(mat_t *mat, uint64_t col, double *vals) {

  return mat_write_col_part(mat, 0, col, mat->numrows, vals);
}

uint8_t mat_write_col_part(
  mat_t *mat, uint64_t row, uint64_t col, uint64_t len, double *vals) {

  uint64_t i;

  if (mat       == NULL)            goto fail;
  if (mat->mode != MAT_MODE_CREATE) goto fail;
  if (vals      == NULL)            goto fail;
  if (row       <  0)               goto fail;
  if (col       <  0)               goto fail;
  if (row       >= mat->numrows)    goto fail;
  if (col       >= mat->numcols)    goto fail;
  if (row + len >  mat->numrows)    goto fail;

  for (i = 0; i < len; i++, row++) {

    if (_mat_seek(mat, row, col))                        goto fail;
    if (fwrite(vals+i, sizeof(double), 1, mat->hd) != 1) goto fail;
  }

  return 0;
fail:
  return 1;
}


uint8_t mat_write_row_label(mat_t *mat, uint64_t row, void *data) {

  uint64_t off;

  if (mat            == NULL)            goto fail;
  if (mat->mode      != MAT_MODE_CREATE) goto fail;
  if (row            == 0)               goto fail;
  if (row            >= mat->numrows)    goto fail;
  if (mat->labelsize == 0)               goto fail;
  if (!mat_has_row_labels(mat))          goto fail;

  off = MAT_HDR_SIZE + ((mat->labelsize) * row);

  if (fseek(mat->hd, off, SEEK_SET))                 goto fail;
  if (fwrite(data, mat->labelsize, 1, mat->hd) != 1) goto fail;

  return 0;
  
fail:
  return 1;
}

uint8_t mat_write_col_label(mat_t *mat, uint64_t col, void *data) {

  uint64_t off;

  if (mat            == NULL)            goto fail;
  if (mat->mode      != MAT_MODE_CREATE) goto fail;
  if (col            == 0)               goto fail;
  if (col            >= mat->numcols)    goto fail;
  if (mat->labelsize == 0)               goto fail;
  if (!mat_has_col_labels(mat))          goto fail;

  off = MAT_HDR_SIZE + ((mat->labelsize) * col);

  if (mat_has_row_labels(mat))
    off += (mat->labelsize)*(mat->numrows);

  if (fseek(mat->hd, off, SEEK_SET))                 goto fail;
  if (fwrite(data, mat->labelsize, 1, mat->hd) != 1) goto fail;  

  return 0;

fail:
  return 1;
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

uint64_t _mat_calc_offset(mat_t *mat, uint64_t row, uint64_t col) {

  uint64_t ncols;
  uint64_t nrows;
  uint64_t val_size;
  uint64_t rlbl_off;
  uint64_t clbl_off;
  uint64_t row_off;
  uint64_t col_off;

  nrows    = mat->numrows;
  ncols    = mat->numcols;
  val_size = sizeof(double);
  rlbl_off = 0;
  clbl_off = 0;
  row_off  = 0;
  col_off  = 0;

  if (mat_has_row_labels(mat))
    rlbl_off = (mat->labelsize) * nrows;
  if (mat_has_col_labels(mat))
    clbl_off = (mat->labelsize) * ncols;

  if (mat_is_symmetric(mat)) {
    row_off = ((ncols * row) - round(row*(row-1.0)/2.0)) * val_size;
    col_off = (col - row) * val_size;
  }
  else {
    row_off = ncols * val_size * row;
    col_off = col * val_size;
  }

  return MAT_HDR_SIZE + rlbl_off + clbl_off + row_off + col_off;
}

uint8_t _mat_seek(mat_t *mat, uint64_t row, uint64_t col) {

  uint64_t offset;

  if (mat == NULL)         goto fail;
  if (row <  0)            goto fail;
  if (col <  0)            goto fail;
  if (row >= mat->numrows) goto fail;
  if (col >= mat->numcols) goto fail;

  if (mat_is_symmetric(mat) && (col < row)) goto fail;

  offset = _mat_calc_offset(mat, row, col);

  if (fseek(mat->hd, offset, SEEK_SET)) goto fail;

  return 0;
  
fail:
  return 1;
}
