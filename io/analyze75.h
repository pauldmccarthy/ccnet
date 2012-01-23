/**
 * ANALYZE75 file format definition, and functions for use.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef ANALYZE75_H
#define ANALYZE75_H

#include <stdint.h>

/*
 * ANALYZETM Header File Format
 *
 * (c) Copyright, 1986-1995
 * Biomedical Imaging Resource
 * Mayo Foundation
 */

#define DT_NONE          0
#define DT_UNKOWN        0 /* typo is intentional (DT_UNKNOWN
                              is defined in dirent.h) */
#define DT_BINARY        1
#define DT_UNSIGNED_CHAR 2
#define DT_SIGNED_SHORT  4
#define DT_SIGNED_INT    8
#define DT_FLOAT         16
#define DT_COMPLEX       32
#define DT_DOUBLE        64
#define DT_RGB           128
#define DT_ALL           255

typedef struct header_key {

  uint32_t sizeof_hdr;
  char     data_type[10];
  char     db_name[18];
  uint32_t extents;
  uint16_t session_error;
  char     regular;
  char     hkey_un0;

} header_key_t;

typedef struct image_dimension {

  uint16_t dim[8];
  uint16_t unused8;
  uint16_t unused9;
  uint16_t unused10;
  uint16_t unused11;
  uint16_t unused12;
  uint16_t unused13;
  uint16_t unused14;
  uint16_t datatype;
  uint16_t bitpix;
  uint16_t dim_un0;
  float    pixdim[8];
  float    vox_offset;
  float    funused1;
  float    funused2;
  float    funused3;
  float    cal_max;
  float    cal_min;
  float    compressed;
  float    verified;
  uint32_t glmax;
  uint32_t glmin;

} image_dimension_t;

typedef struct data_history {

  char     descrip[80];
  char     aux_file[24];
  char     orient;
  char     originator[10];
  char     generated[10];
  char     scannum[10];
  char     patient_id[10];
  char     exp_date[10];
  char     exp_time[10];
  char     hist_un0[3];
  uint32_t views;
  uint32_t vols_added;
  uint32_t start_field;
  uint32_t field_skip;
  uint32_t omax;
  uint32_t omin;
  uint32_t smax;
  uint32_t smin;

} data_history_t;

typedef struct dsr {

  header_key_t      hk;
  image_dimension_t dime;
  data_history_t    hist;

  /*
   * This field is set to 1 by analyze_load_hdr if the endianness
   * of the header file being read in does not match the endianness
   * of the system.
   */
  uint8_t           rev;

} dsr_t;

typedef struct _complex {

  float real;
  float imag;

} complex_t;


/**
 * Reverses all the fields in the header,
 */
void analyze_reverse_hdr(
  dsr_t *hdr /**< header to reverse */
);

/**
 * Checks the given headers to make sure that they all 
 * have the same dimensions, data type and endianness.
 *
 * \return 0 if all of the headers match, non-0 otherwise.
 */
uint8_t analyze_hdr_compat(
  uint16_t  nhdrs, /**< number of headers */
  dsr_t    *hdrs   /**< headers to check  */
);

/**
 * Convenience function with equivalent behaviour to
 * analyze_hdr_compat. Accepts a list of pointers,
 * rather than a list of structs.
 *
 * \return 0 if all of the headers match, non-0 otherwise.
 */
uint8_t analyze_hdr_compat_ptr(
  uint16_t nhdrs, /**< number of headers */
  dsr_t  **hdrs   /**< headers to check  */
);

/**
 * Reads an ANALYZE75 header from the given file.
 *
 * \return 0 on success, non-0 otherwise.
 */
uint8_t analyze_load_hdr(
  char    *filename, /**< name of header file to read    */
  dsr_t   *dsr       /**< struct to store header info in */
);

/**
 * Writes the given ANALYZE75 header to the specified file.
 *
 * \return 0 on success, non-0 otherwise.
 */
uint8_t analyze_write_hdr(
  char  *filename, /**< name of file to write to */
  dsr_t *hdr       /**< header to write          */
);

/**
 * Load both header and image files into memory.
 * The two files must have identical names, with
 * '.hdr' suffix for the header, and '.img' suffix
 * for the image file.
 *
 * The given file name may be either the header or 
 * image file, or may be the common name without
 * a suffix.
 *
 * If the given hdr pointer is NULL, the header
 * is not loaded. 
 *
 * If the given data pointer is  NULL, the image 
 * is not loaded, and no memory is allocated.
 *
 * \return 0 on success, non-0 otherwise.
 */
uint8_t analyze_load(
  char     *filename, /**< name of file to load                      */
  dsr_t    *hdr,      /**< pointer to header struct                  */
  uint8_t **data      /**< pointer which will be allocated for image */
);

/**
 * Writes the image data to the given file.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t analyze_write_img(
  char    *filename, /**< name of file to write to */
  dsr_t   *hdr,      /**< image header             */
  uint8_t *img       /**< image data               */
);

/**
 * \return the data type of the file.
 */
uint16_t analyze_datatype(
  dsr_t *hdr /**< file header */
);

/**
 * \return the size, in bytes, of one value of the given datatype.
 */
uint8_t analyze_datatype_size(
  uint16_t datatype /**< data type */
);

/**
 * \return the size, in bytes, of one value in the file.
 */
uint8_t analyze_value_size(
  dsr_t *hdr /**< file header */
);

/**
 * \return the number of dimensions in the file.
 */
uint8_t analyze_num_dims(
  dsr_t *hdr /**< file header */
);

/**
 * \return the size of the given dimension. 
 * Dimension numbers are 0-indexed, so to 
 * query the first dimension, pass in 0.
 */
uint16_t analyze_dim_size(
  dsr_t   *hdr, /**< file header        */
  uint8_t  dim  /**< dimension to query */
);

/**
 * \return the real world measurement 
 * of the given dimension.
 */
float analyze_pixdim_size(
  dsr_t   *hdr, /**< file header        */
  uint8_t  dim  /**< dimension to query */
);

/**
 * \return the offset of the given dimension.
 */
uint32_t analyze_dim_offset(
  dsr_t   *hdr, /**< file header        */
  uint8_t  dim  /**< dimension to query */
);

/**
 * \return the image offset for the given dimension indices.
 */
uint32_t analyze_get_offset(
  dsr_t    *hdr, /**< file header       */
  uint32_t *dims /**< dimension indices */
);

/**
 * Converts the given image value index into separate dimension indices.
 */
void analyze_get_indices(
  dsr_t    *hdr,   /**< image header                     */
  uint32_t  index, /**< value index to convert           */
  uint32_t *dims   /**< place to store dimension indices */
);

/**
 * \return the total number of values contained in the file.
 */
uint32_t analyze_num_vals(
  dsr_t *hdr /**< file header */
);

/**
 * Prints the given value to the given string.
 */
void analyze_sprint_val(
  dsr_t *hdr, /**< file header                  */
  char  *str, /**< place to put formatted value */
  double val  /**< value to print               */
);

/**
 * Reads an image value from the given location.
 */
double analyze_read_val(
  dsr_t    *hdr, /**< file header           */
  uint8_t  *img, /**< image data            */
  uint32_t *dims /**< location to read from */
);

/**
 * Reads a value from the data.
 */
double analyze_read(
  dsr_t   *hdr, /**< file header */
  uint8_t *data /**< image data  */
);

/**
 * Writes an image value to the given location.
 */
void analyze_write_val(
  dsr_t    *hdr,  /**< file header          */
  uint8_t  *img,  /**< image data           */
  uint32_t *dims, /**< location to write to */
  double    val   /**< value to write       */
);

/**
 * Writes the given value to the data.
 */
void analyze_write(
  dsr_t   *hdr,  /**< file header    */
  uint8_t *data, /**< image data     */
  double   val   /**< value to write */
);

/**
 * \return an unsigned char read from the data.
 */
double analyze_read_unsigned_char(
  dsr_t   *hdr,  /**< file header       */
  uint8_t *data  /**< data to read from */
);

/**
 * \return a signed (16 bit) integer read from the data.
 */
double analyze_read_signed_short(
  dsr_t   *hdr, /**< file header       */
  uint8_t *data /**< data to read from */
);

/**
 * \return a signed (32 bit) integer read from the data.
 */
double analyze_read_signed_int(
  dsr_t   *hdr, /**< file header       */
  uint8_t *data /**< data to read from */
);

/**
 * \return a float read from the data.
 */
double analyze_read_float(
  dsr_t   *hdr, /**< file header       */
  uint8_t *data /**< data to read from */
);

/**
 * \return a double read from the data.
 */
double analyze_read_double(
  dsr_t   *hdr, /**< file header       */
  uint8_t *data /**< data to read from */
);

/**
 * Writes an unsigned char to the data.
 */
void analyze_write_unsigned_char(
  dsr_t   *hdr,  /**< file header      */
  uint8_t *data, /**< data to write to */
  double   val   /**< value to write   */
);

/**
 * Writes a signed (16 bit) integer to the data.
 */
void analyze_write_signed_short(
  dsr_t   *hdr,  /**< file header      */
  uint8_t *data, /**< data to write to */
  double   val   /**< value to write   */
);

/**
 * Writes a signed (32 bit) integer to the data.
 */
void analyze_write_signed_int(
  dsr_t   *hdr,  /**< file header      */
  uint8_t *data, /**< data to write to */
  double   val   /**< value to write   */
);

/**
 * Writes a float to the data.
 */
void analyze_write_float(
  dsr_t   *hdr,  /**< file header      */
  uint8_t *data, /**< data to write to */
  double   val   /**< value to write   */
);

/**
 * Writes a double to the data.
 */
void analyze_write_double(
  dsr_t   *hdr,  /**< file header      */
  uint8_t *data, /**< data to write to */
  double   val   /**< value to write   */
);

#endif
