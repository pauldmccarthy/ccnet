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

/**
 * Flag options.
 */
typedef enum {

  MAT_IS_SYMMETRIC   = 0,
  MAT_HAS_ROW_LABELS = 1,
  MAT_HAS_COL_LABELS = 2,


} mat_flags_t;


/**
 * Opens an existing mat file for reading.
 *
 * \return a newly allocated mat_t struct on success, NULL on failure.
 */
mat_t * mat_open(
  char *fname /**< name of file to open */
);

/**
 * Closes the given mat file.
 */
uint8_t mat_close(
  mat_t *mat /**< struct representing the mat file to close */
);

/**
 * \return the number of rows in the mat file.
 */
uint64_t mat_num_rows(
  mat_t *mat /**< mat file to query */
);

/**
 * \return the number of columns in the mat file.
 */
uint64_t mat_num_cols(
  mat_t *mat /**< mat file to query */
);

/**
 * \return the header data size in bytes.
 */
uint16_t mat_hdr_data_size(
  mat_t *mat /**< mat file to query */
);

/**
 * \return the row/column label size in bytes.
 */
uint16_t mat_label_size(
  mat_t *mat /**< mat file to query */
);

/**
 * \return non-0 if the given mat file is symmetric, 0 otherwise.
 */
uint8_t mat_is_symmetric(
  mat_t *mat /**< mat file to query */
);

/**
 * \return non-0 if the given mat file has row labels, 0 otherwise.
 */
uint8_t mat_has_row_labels(
  mat_t *mat /**< mat file to query */
);

/**
 * \return non-0 if the given mat file has column labels, 0 otherwise.
 */
uint8_t mat_has_col_labels(
  mat_t *mat /**< mat file to query */
);

/**
 * \return the element at the given row/column.
 */
double mat_read_elem(
  mat_t   *mat, /**< mat file to query */
  uint64_t row, /**< row to read       */
  uint64_t col  /**< column to read    */
);

/**
 * Copies the specified row into the given pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_row(
  mat_t   *mat, /**< mat file to query  */
  uint64_t row, /**< row to read        */
  double  *vals /**< space to store row */
);

/**
 * Copies the specified row section into the given pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_row_part(
  mat_t   *mat, /**< mat file to query          */
  uint64_t row, /**< row to read                */
  uint64_t col, /**< starting column            */
  uint64_t len, /**< number of values to read   */
  double  *vals /**< space to store row section */
);

/**
 * Copies the specified column into the given pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_col(
  mat_t   *mat, /**< mat file to query     */
  uint64_t col, /**< column to read        */
  double  *vals /**< space to store column */
);

/**
 * Copies the specified column section into the given pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_col_part(
  mat_t   *mat, /**< mat file to query             */
  uint64_t row, /**< starting row                  */
  uint64_t col, /**< column to read                */
  uint64_t len, /**< number of values to read      */
  double  *vals /**< space to store column section */
);

/**
 * Reads the label for the given row.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_row_label(
  mat_t   *mat, /**< mat file to query        */
  uint64_t row, /**< row to read              */
  void    *data /**< space to store row label */
);

/**
 * Reads the label for the given column.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_col_label(
  mat_t   *mat, /**< mat file to query           */
  uint64_t col, /**< column to read              */
  void    *data /**< space to store column label */
);

/**
 * Reads the header data from the given file.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_read_hdr_data(
  mat_t *mat,    /**< mat file to query          */
  void  *hdrdata /**< place to store header data */
);

/**
 * Creates a new mat file.
 *
 * \return a newly allocated mat_t struct on success, NULL on failure.
 */
mat_t * mat_create(
  char    *fname,    /**< name of file to create */
  uint64_t numrows,  /**< number of rows         */
  uint64_t numcols,  /**< number of rows         */
  uint16_t flags,    /**< file options           */
  uint16_t hdrsize,  /**< header data size       */
  uint8_t  labelsize /**< label size             */
);

/**
 * Writes the value to the specified row/column.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_elem(
  mat_t   *mat, /**< mat file to write to */
  uint64_t row, /**< row index            */
  uint64_t col, /**< column index         */
  double   val  /**< value to write       */
);

/**
 * Writes the data to the specified row.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_row(
  mat_t   *mat, /**< mat file to write to */
  uint64_t row, /**< row to write to      */
  double  *vals /**< data to write        */
);

/**
 * Writes the data to the specified row section.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_row_part(
  mat_t   *mat, /**< mat file to write to      */
  uint64_t row, /**< row to write to           */
  uint64_t col, /**< starting column           */
  uint64_t len, /**< number of values to write */
  double  *vals /**< data to write             */
);

/**
 * Writes the data to the specified column.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_col(
  mat_t   *mat, /**< mat file to write to */
  uint64_t col, /**< column to write to   */
  double  *vals /**< data to write        */
);

/**
 * Writes the data to the specified column section.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_col_part(
  mat_t   *mat, /**< mat file to write to      */
  uint64_t row, /**< starting row              */
  uint64_t col, /**< column to write to        */
  uint64_t len, /**< number of values to write */
  double  *vals /**< data to write             */
);

/**
 * Writes the label for the given row.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_row_label(
  mat_t   *mat, /**< mat file to write to */
  uint64_t row, /**< row to write to      */
  void    *data /**< data to write        */
);

/**
 * Writes the label for the given column.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_col_label(
  mat_t   *mat, /**< mat file to write to */
  uint64_t col, /**< column to write to   */
  void    *data /**< data to write        */
);

/**
 * Writes the given data to the header data section.
 * The header data section is then padded with 0s.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t mat_write_hdr_data(
  mat_t   *mat,     /**< mat file to query          */
  void    *hdrdata, /**< place to store header data */
  uint16_t len      /**< number of bytes to write   */
);

#endif
