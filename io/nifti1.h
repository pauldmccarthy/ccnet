/**
 * Definition of the NIFTI-1 header format:
 *   http://nifti.nimh.nih.gov/nifti-1
 *
 * Based on the header file 'nifti1.h', written by Bob Cox, 
 * which  is in the public domain, and is available at:
 *   http://nifti.nimh.nih.gov/pub/dist/src/niftilib/nifti1.h
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef NIFTI1_H
#define NIFTI1_H

#include <stdint.h>

#include "io/analyze75.h"

typedef struct _nifti_hdr{

  uint32_t sizeof_hdr;
  char     data_type[10];
  char     db_name[18];
  uint32_t extents;
  uint16_t session_error;
  char     regular;
  char     dim_info;

  uint16_t dim[8];
  float    intent_p1;
  float    intent_p2;
  float    intent_p3;
  uint16_t intent_code;
  uint16_t datatype;
  uint16_t bitpix;
  uint16_t slice_start;
  float    pixdim[8];
  float    vox_offset;
  float    scl_slope;
  float    scl_inter;
  uint16_t slice_end;
  char     slice_code;
  char     xyzt_units;
  float    cal_max;
  float    cal_min;
  float    slice_duration;
  float    toffset;
  uint32_t glmax;
  uint32_t glmin;

  char     descrip[80];
  char     aux_file[24];
  uint16_t qform_code;
  uint16_t sform_code;
  float    quatern_b;
  float    quatern_c;
  float    quatern_d;
  float    qoffset_x;
  float    qoffset_y;
  float    qoffset_z;
  float    srow_x[4];
  float    srow_y[4];
  float    srow_z[4];
  char     intent_name[16];
  char     magic[4];
  uint8_t  extension[4];

  /*
   * This field is set to 1 by nifti1_load_hdr, if the endianness of
   * the header file that is being read does not match the endianness
   * of the system.
   */
  uint8_t  rev;

} nifti1_hdr_t;

/**
 * Reverses all of the fields in the given header.
 */
void nifti1_reverse_hdr(
  nifti1_hdr_t *hdr /**< header to reverse */
);

/**
 * Loads a NIFTI-1 header from the given 
 * file into the given struct pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t nifti1_load_hdr(
  char         *filename, /**< file to read header from   */
  nifti1_hdr_t *hdr       /**< header struct to copy into */
);

/**
 * Converts a NIFTI1 header to an ANALYZE75 header.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t nifti1_to_analyze(
  nifti1_hdr_t *nhdr, /**< pointer to NIFTi1 header to read     */
  dsr_t        *ahdr  /**< pointer to ANALYZE75 header to write */
);

#endif
