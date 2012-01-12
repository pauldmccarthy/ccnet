/**
 *   mat - a simple binary matrix file format.
 *
 * A simple file format for the storage and access of rectangular 2D double
 * matrices. See README.MAT for details.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef __MAT_H__
#define __MAT_H__

#include <stdint.h>

struct __mat;
typedef struct __mat mat_t;

typedef enum {

  MAT_HAS_ROW_LABELS = 0,
  MAT_HAS_COL_LABELS = 1,
  MAT_IS_SYMMETRIC   = 2,

} mat_flags_t;



mat_t * mat_open(
  char *fname
);

uint8_t mat_close(
  mat_t *mat
);

uint64_t mat_num_rows(
  mat_t *mat
);

uint64_t mat_num_cols(
  mat_t *mat
);

uint8_t mat_is_symmetric(
  mat_t *mat
);

uint8_t mat_has_row_labels(
  mat_t *mat
);

uint8_t mat_has_col_labels(
  mat_t *mat
);

double mat_read_elem(
  mat_t   *mat,
  uint64_t row,
  uint64_t col
);

uint8_t mat_read_row(
  mat_t   *mat,
  uint64_t row,
  double  *vals
);

uint8_t mat_read_row_part(
  mat_t   *mat,
  uint64_t row,
  uint64_t col,
  uint64_t len,
  double  *vals
);

uint8_t mat_read_col(
  mat_t   *mat,
  uint64_t col,
  double  *vals
);

uint8_t mat_read_col_part(
  mat_t   *mat,
  uint64_t row,
  uint64_t col,
  uint64_t len,
  double  *vals
);

uint8_t mat_read_row_label(
  mat_t   *mat,
  uint64_t row,
  void    *data
);

uint8_t mat_read_col_label(
  mat_t   *mat,
  uint64_t col,
  void    *data
);


mat_t * mat_create(
  char    *fname,
  uint64_t numrows,
  uint64_t numcols,
  uint16_t flags,
  uint8_t  labelsize
);

//mat_write_elem();
//mat_write_row();
//mat_write_col();
//mat_write_row_part();
//mat_write_col_part();
//mat_write_matrix();
//mat_write_row_label();
//mat_write_col_label();
//mat_write_row_labels();
//mat_write_col_labels();








#endif
