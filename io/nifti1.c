/**
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "io/analyze75.h"
#include "io/nifti1.h"
#include "util/reverse.h"
#include "util/suffix.h"
#include "util/filesize.h"

void nifti1_reverse_hdr(nifti1_hdr_t *hdr) {

  uint8_t i;

  reverse(&(hdr->sizeof_hdr), &(hdr->sizeof_hdr), sizeof(hdr->sizeof_hdr));
  reverse(&(hdr->extents),    &(hdr->extents),    sizeof(hdr->extents));
  reverse(&(hdr->session_error), 
          &(hdr->session_error), 
     sizeof(hdr->session_error));

  for (i = 0; i < 8; i++)
    reverse((hdr->dim)+i, (hdr->dim)+i, sizeof((hdr->dim)[i]));

  reverse(&(hdr->intent_p1),   &(hdr->intent_p1),   sizeof(hdr->intent_p1));
  reverse(&(hdr->intent_p2),   &(hdr->intent_p2),   sizeof(hdr->intent_p2));
  reverse(&(hdr->intent_p3),   &(hdr->intent_p3),   sizeof(hdr->intent_p3));
  reverse(&(hdr->intent_code), &(hdr->intent_code), sizeof(hdr->intent_code));
  reverse(&(hdr->datatype),    &(hdr->datatype),    sizeof(hdr->datatype));
  reverse(&(hdr->bitpix),      &(hdr->bitpix),      sizeof(hdr->bitpix));
  reverse(&(hdr->slice_start), &(hdr->slice_start), sizeof(hdr->slice_start));

  for (i = 0; i < 8; i++)
    reverse((hdr->pixdim)+i, (hdr->pixdim)+i, sizeof((hdr->pixdim)[i]));

  reverse(&(hdr->vox_offset),  &(hdr->vox_offset),  sizeof(hdr->vox_offset));
  reverse(&(hdr->scl_slope),   &(hdr->scl_slope),   sizeof(hdr->scl_slope));
  reverse(&(hdr->scl_inter),   &(hdr->scl_inter),   sizeof(hdr->scl_inter));
  reverse(&(hdr->slice_end),   &(hdr->slice_end),   sizeof(hdr->slice_end));
  reverse(&(hdr->slice_code),  &(hdr->slice_code),  sizeof(hdr->slice_code));
  reverse(&(hdr->xyzt_units),  &(hdr->xyzt_units),  sizeof(hdr->xyzt_units));
  reverse(&(hdr->cal_max),     &(hdr->cal_max),     sizeof(hdr->cal_max));
  reverse(&(hdr->cal_min),     &(hdr->cal_min),     sizeof(hdr->cal_min));
  reverse(&(hdr->slice_duration), 
          &(hdr->slice_duration), 
     sizeof(hdr->slice_duration));

  reverse(&(hdr->toffset),     &(hdr->toffset),     sizeof(hdr->toffset));
  reverse(&(hdr->glmax),       &(hdr->glmax),       sizeof(hdr->glmax));
  reverse(&(hdr->glmin),       &(hdr->glmin),       sizeof(hdr->glmin));

  reverse(&(hdr->qform_code),  &(hdr->qform_code),  sizeof(hdr->qform_code));
  reverse(&(hdr->sform_code),  &(hdr->sform_code),  sizeof(hdr->sform_code));
  reverse(&(hdr->quatern_b),   &(hdr->quatern_b),   sizeof(hdr->quatern_b));
  reverse(&(hdr->quatern_c),   &(hdr->quatern_c),   sizeof(hdr->quatern_c));
  reverse(&(hdr->quatern_d),   &(hdr->quatern_d),   sizeof(hdr->quatern_d));
  reverse(&(hdr->qoffset_x),   &(hdr->qoffset_x),   sizeof(hdr->qoffset_x));
  reverse(&(hdr->qoffset_y),   &(hdr->qoffset_y),   sizeof(hdr->qoffset_y));
  reverse(&(hdr->qoffset_z),   &(hdr->qoffset_z),   sizeof(hdr->qoffset_z));

  for (i = 0; i < 4; i++)
    reverse((hdr->srow_x)+i, (hdr->srow_x)+i, sizeof((hdr->srow_x)[i]));
    reverse((hdr->srow_y)+i, (hdr->srow_y)+i, sizeof((hdr->srow_y)[i]));
    reverse((hdr->srow_z)+i, (hdr->srow_z)+i, sizeof((hdr->srow_z)[i]));
}

uint8_t nifti1_load_hdr(char *filename, nifti1_hdr_t *hdr) {

  FILE    *f;
  int      sz;
  uint8_t *bytes;
  char    *afile;

  f     = NULL;
  bytes = NULL;
  afile = NULL;

  afile = set_suffix(filename, "hdr");
  if (afile == NULL) goto fail;

  f = fopen(afile, "rb");
  if (f == NULL) goto fail;

  sz = filesize(f);
  if (sz != 348 && sz != 352) goto fail;

  bytes = malloc(sz);
  if (bytes == NULL) goto fail;

  if (fread(bytes, 1, sz, f) != sz) goto fail;

  memcpy(hdr, bytes, sz);

  if (hdr->dim[0] > 7) {

    nifti1_reverse_hdr(hdr);

    if (hdr->dim[0]     >  7)   goto fail;
    if (hdr->sizeof_hdr != 348) goto fail;

    hdr->rev = 1;
  }

  fclose(f);
  free(bytes);
  free(afile);
  return 0;

fail:
  if (f     != NULL) fclose(f);
  if (bytes != NULL) free(bytes);
  if (afile != NULL) free(afile);
  return 1;
}

uint8_t nifti1_to_analyze(nifti1_hdr_t *nhdr, dsr_t *ahdr) {

  /*
   * straight header conversion is not 
   * possible if the NIFTI1 file is in a 
   * format not supported by ANALYZE75
   */
  if (nhdr->datatype > 256) return 1;

  memset(ahdr, 0, sizeof(dsr_t));

  ahdr->hk.sizeof_hdr = 348;
  ahdr->rev           = nhdr->rev;

  memcpy(ahdr->hk.data_type, nhdr->data_type, sizeof(ahdr->hk.data_type));
  memcpy(ahdr->hk.db_name,   nhdr->db_name,   sizeof(ahdr->hk.db_name));

  ahdr->hk.extents  = 16384;
  ahdr->hk.regular  = 'r';
  ahdr->hk.hkey_un0 = '0'; 

  memcpy(ahdr->dime.dim,    nhdr->dim,    sizeof(ahdr->dime.dim));
  memcpy(ahdr->dime.pixdim, nhdr->pixdim, sizeof(ahdr->dime.pixdim));
  
  ahdr->dime.datatype = nhdr->datatype;
  ahdr->dime.cal_min  = nhdr->cal_min;
  ahdr->dime.cal_max  = nhdr->cal_max;
  ahdr->dime.glmin    = nhdr->glmin;
  ahdr->dime.glmax    = nhdr->glmax;
  ahdr->dime.bitpix   = nhdr->bitpix;

  memcpy(ahdr->hist.descrip,  nhdr->descrip,  sizeof(ahdr->hist.descrip));
  memcpy(ahdr->hist.aux_file, nhdr->aux_file, sizeof(ahdr->hist.aux_file));

  return 0;
}
