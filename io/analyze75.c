/**
 * Functions for reading/writing ANALYZE75 files.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "io/analyze75.h"
#include "io/nifti1.h"
#include "util/suffix.h"
#include "util/filesize.h"
#include "util/reverse.h"

/**
 * Reverses all header key fields.
 */
static void _reverse_header_key(header_key_t *hk);

/**
 * Reverses all image dimension fields.
 */
static void _reverse_image_dimension(image_dimension_t *id);

/**
 * Reverses all data history fields.
 */
static void _reverse_data_history(data_history_t *dh);


uint16_t analyze_datatype(dsr_t *hdr) {

  return hdr->dime.datatype;
}

uint8_t analyze_datatype_size(uint16_t datatype) {

  switch (datatype) {

    case DT_UNSIGNED_CHAR: return 1;
    case DT_SIGNED_SHORT:  return 2;
    case DT_SIGNED_INT:    return 4;
    case DT_FLOAT:         return 4;
    case DT_COMPLEX:       return 8;
    case DT_DOUBLE:        return 8;
    default:               return 0;
  }
}

uint8_t analyze_value_size(dsr_t *hdr) {

  return analyze_datatype_size(hdr->dime.datatype);
}

uint8_t analyze_num_dims(dsr_t *hdr) {

  /*first value in dim array is number of dimensions*/
  return hdr->dime.dim[0];
}

uint16_t analyze_dim_size(dsr_t *hdr, uint8_t dim) {

  uint8_t ndims;

  ndims = analyze_num_dims(hdr);

  return (dim > ndims) 
    ? 0 
    : hdr->dime.dim[dim+1];
}

float analyze_pixdim_size(dsr_t *hdr, uint8_t dim) {

  uint8_t ndims;

  ndims = analyze_num_dims(hdr);

  return (dim > ndims) 
    ? 0 
    : hdr->dime.pixdim[dim+1];
}


uint32_t analyze_dim_offset(dsr_t *hdr, uint8_t dim) {

  uint8_t  i;
  uint32_t off;
  uint32_t dimsz;

  for (off = 1, i = 0; i < dim; i++) {

    dimsz = analyze_dim_size(hdr, i);
    off = off * dimsz;
  }

  return off;
}

uint32_t analyze_get_offset(dsr_t *hdr, uint32_t *dims) {

  uint8_t  i;
  uint8_t  valsize;
  uint8_t  ndims;
  uint32_t dimoff;  
  uint32_t off;

  valsize = analyze_value_size(hdr);
  ndims   = analyze_num_dims(  hdr);

  for (off = 0, i = 0; i < ndims; i++) {

    dimoff = analyze_dim_offset(hdr, i);

    off += dimoff*dims[i];
  }

  return off*valsize;
}

uint32_t analyze_num_vals(dsr_t *hdr) {

  uint8_t  ndims;
  uint8_t  dim;
  uint32_t nvals;

  ndims = analyze_num_dims(hdr);

  for (nvals = 1, dim = 1; dim <= ndims; dim++)
    nvals *= hdr->dime.dim[dim];

  return nvals;
}

void analyze_reverse_hdr(dsr_t *hdr) {

  if (hdr == NULL) return;

  _reverse_header_key(     &(hdr->hk));
  _reverse_image_dimension(&(hdr->dime));
  _reverse_data_history(   &(hdr->hist));
}

uint8_t analyze_load_hdr(char *file, dsr_t *dsr) {

  uint16_t     i;
  int          sz;
  FILE        *f;
  uint8_t     *bytes;
  char        *afile;
  nifti1_hdr_t nhdr;

  f         = NULL;
  bytes     = NULL;
  afile     = NULL;

  memset(dsr,   0, sizeof(dsr_t));
  memset(&nhdr, 0, sizeof(nifti1_hdr_t));

  /*
   * 1. open file
   * 2. query file size
   * 3. malloc space to store contents of file
   * 4. read contents of file
   * 5. parse header
   */
  afile = set_suffix(file, "hdr");
  if (afile == NULL) goto fail;

  f = fopen(afile, "rb");
  if (f == NULL) goto fail;

  sz = filesize(f);
  if (sz != 348) {

    /* accept a NIFTI1 header */
    if (sz == 352) {
      
      if (nifti1_load_hdr(afile, &nhdr)) goto fail;
      if (nifti1_to_analyze(&nhdr, dsr)) goto fail;

      goto succ;
    }
    else goto fail;
  }

  bytes = malloc(sz);
  if (bytes == NULL) goto fail;

  if (fread(bytes, 1, sz, f) != sz) goto fail;

  memcpy(dsr, bytes, sz);

  if (dsr->dime.dim[0] > 7) {

    _reverse_header_key     (&(dsr->hk));
    _reverse_image_dimension(&(dsr->dime));
    _reverse_data_history   (&(dsr->hist));

    if (dsr->dime.dim[0]   >  7)   goto fail;
    if (dsr->hk.sizeof_hdr != 348) goto fail;

    dsr->rev = 1;
  }

  /*fix number of dimensions*/
  dsr->dime.dim[0] = 0;
  for (i = 1; i < 7; i++) {
    if (dsr->dime.dim[i] > 1) dsr->dime.dim[0]++;
    else                      break;
  }

succ:
  if (bytes != NULL) free(bytes);
  if (afile != NULL) free(afile);
  if (f     != NULL) fclose(f);
  return 0;

fail:
  if (f     != NULL) fclose(f);
  if (bytes != NULL) free(bytes);
  if (afile != NULL) free(afile);
  return 1;
}

uint8_t analyze_write_img(char *filename, dsr_t *hdr, uint8_t *img) {

  FILE    *fd;
  uint32_t nvals;
  uint8_t  valsz;

  fd = NULL;

  filename = set_suffix(filename, "img");
  nvals    = analyze_num_vals(  hdr);
  valsz    = analyze_value_size(hdr);

  fd = fopen(filename, "wb");

  if (fwrite(img, 1, nvals*valsz, fd) != nvals*valsz) goto fail;

  fclose(fd);
  free(filename);
  return 0;
  
fail:

  if (filename != NULL) free(filename);
  if (fd       != NULL) fclose(fd);
  return 1;
}

uint8_t analyze_write_hdr(char *filename, dsr_t *hdr) {

  FILE *fd;
  dsr_t hdrcpy;

  fd = NULL;

  filename = set_suffix(filename, "hdr");
  if (filename == NULL) goto fail;

  memcpy(&hdrcpy, hdr, sizeof(dsr_t));

  if (hdrcpy.rev) analyze_reverse_hdr(&hdrcpy);

  fd = fopen(filename, "wb");
  if (fd == NULL) goto fail;

  if (fwrite(&hdrcpy, 1, 348, fd) != 348) goto fail;
  
  free(filename);
  fclose(fd);
  
  return 0;
  
fail:
  if (filename != NULL) free(filename);
  if (fd       != NULL) fclose(fd);
  return 1;
}

uint8_t analyze_hdr_compat(uint8_t nhdrs, dsr_t *hdrs) {

  uint8_t   i, j;
  uint8_t   ndims;
  uint16_t *dimszs;
  float    *pixdims;
  float     pixdim_diff;
  uint16_t  dtype;
  uint8_t   endi;

  dimszs  = NULL;
  pixdims = NULL;

  if (nhdrs == 0) return 0;

  /*
   * get number of dimensions, dimension sizes,data
   * type and endianness from first input image
   */
  ndims = analyze_num_dims(hdrs);
  dtype = analyze_datatype(hdrs);
  endi  = hdrs->rev;

  dimszs  = malloc(ndims*sizeof(uint16_t));
  if (dimszs  == NULL) goto fail;
  pixdims = malloc(ndims*sizeof(float));
  if (pixdims == NULL) goto fail;

  for (i = 0; i < ndims; i++) {
    dimszs [i] = analyze_dim_size(   hdrs, i);
    pixdims[i] = analyze_pixdim_size(hdrs, i);
  }

  /*
   * verify that all of the images 
   * have the same dimensions
   */
  for (i = 1; i < nhdrs; i++) {

    if (analyze_num_dims(hdrs+i) != ndims) goto fail;
    if (analyze_datatype(hdrs+i) != dtype) goto fail;
    if (hdrs[i].rev              != endi)  goto fail; 

    for (j = 0; j < ndims; j++) {

      pixdim_diff = fabs(analyze_pixdim_size(hdrs+i,j) - pixdims[j]);
      
      if (analyze_dim_size(hdrs+i, j) != dimszs [j]) goto fail;
      if (pixdim_diff                  > 0.00001)    goto fail;
    }
  }

  free(dimszs);
  free(pixdims);
  return 0;

fail:
  if (dimszs  != NULL) free(dimszs);
  if (pixdims != NULL) free(pixdims);
  return 1;
}

uint8_t analyze_load(
  char     *filename, 
  dsr_t    *hdr, 
  uint8_t **data)
{
  int      sz;
  FILE    *f;
  char    *afilename;
  uint8_t  valsize;
  uint32_t numvals;

  f                       = NULL;
  afilename               = NULL;
  if (data != NULL) *data = NULL;

  if (filename == NULL) goto fail;

  /*load header if requested*/
  if (hdr != NULL) {
    if (analyze_load_hdr(filename, hdr)) goto fail;
  }

  /*load image if requested*/
  if (data != NULL) {

    /*
     * 1. open file
     * 2. query file size
     * 3. check file size against header (if header was loaded)
     * 4. allocate space for data
     * 5. read data
     * 6. close file
     */

    afilename = set_suffix(filename, "img");
    if (afilename == NULL) goto fail;
    
    f = fopen(afilename, "rb");
    if (f == NULL) goto fail;

    sz = filesize(f);
    if (sz == -1) goto fail;

    if (hdr != NULL) {

      valsize = analyze_value_size(hdr);
      numvals = analyze_num_vals(  hdr);
      if (sz != valsize*numvals) goto fail;
    }

    *data = malloc(sz);
    if (*data == NULL) goto fail;

    if (fread(*data, 1, sz, f) != sz) goto fail;

    fclose(f);
  }

  free(afilename);
  return 0;

fail:

  if (f         != NULL)                  fclose(f);
  if (afilename != NULL)                  free(afilename);
  if (data      != NULL && *data != NULL) free(*data);
  return 1;
}

void _reverse_header_key(header_key_t *d) {

  reverse(&(d->sizeof_hdr), 
                &(d->sizeof_hdr), 
           sizeof(d->sizeof_hdr));
  reverse(&(d->extents), 
                &(d->extents), 
           sizeof(d->extents));
  reverse(&(d->session_error),
                &(d->session_error), 
           sizeof(d->session_error));
}

void _reverse_image_dimension(image_dimension_t *d) {

  uint8_t i;

  for (i = 0; i < 8; i++)
    reverse(&(d->dim[i]), &(d->dim[i]), sizeof(d->dim[i]));

  reverse(&(d->unused8),  &(d->unused8),  sizeof(d->unused8));
  reverse(&(d->unused9),  &(d->unused9),  sizeof(d->unused9));
  reverse(&(d->unused10), &(d->unused10), sizeof(d->unused10));
  reverse(&(d->unused11), &(d->unused11), sizeof(d->unused11));
  reverse(&(d->unused12), &(d->unused12), sizeof(d->unused12));
  reverse(&(d->unused13), &(d->unused13), sizeof(d->unused13));
  reverse(&(d->unused14), &(d->unused14), sizeof(d->unused14));
  reverse(&(d->datatype), &(d->datatype), sizeof(d->datatype));
  reverse(&(d->bitpix),   &(d->bitpix),   sizeof(d->bitpix));
  reverse(&(d->dim_un0),  &(d->dim_un0),  sizeof(d->dim_un0));

  for (i = 0; i < 8; i++) 
    reverse(&(d->pixdim[i]), &(d->pixdim[i]), sizeof(d->pixdim[i]));

  reverse(&(d->vox_offset), &(d->vox_offset), sizeof(d->vox_offset));
  reverse(&(d->funused1),   &(d->funused1),   sizeof(d->funused1));
  reverse(&(d->funused2),   &(d->funused2),   sizeof(d->funused2));
  reverse(&(d->funused3),   &(d->funused3),   sizeof(d->funused3));
  reverse(&(d->cal_max),    &(d->cal_max),    sizeof(d->cal_max));
  reverse(&(d->cal_min),    &(d->cal_min),    sizeof(d->cal_min));
  reverse(&(d->compressed), &(d->compressed), sizeof(d->compressed));
  reverse(&(d->verified),   &(d->verified),   sizeof(d->verified));
  reverse(&(d->glmax),      &(d->glmax),      sizeof(d->glmax));
  reverse(&(d->glmin),      &(d->glmin),      sizeof(d->glmin));
}

void _reverse_data_history(data_history_t *d) {

  reverse(&(d->views),       &(d->views),       sizeof(d->views));
  reverse(&(d->vols_added),  &(d->vols_added),  sizeof(d->vols_added));
  reverse(&(d->start_field), &(d->start_field), sizeof(d->start_field));
  reverse(&(d->field_skip),  &(d->field_skip),  sizeof(d->field_skip));
  reverse(&(d->omax),        &(d->omax),        sizeof(d->omax));
  reverse(&(d->omin),        &(d->omin),        sizeof(d->omin));
  reverse(&(d->smax),        &(d->smax),        sizeof(d->smax));
  reverse(&(d->smin),        &(d->smin),        sizeof(d->smin));
}

void analyze_sprint_val(dsr_t *hdr, char *str, double val) {
  switch (analyze_datatype(hdr)) {

    case DT_UNSIGNED_CHAR: sprintf(str, "%u",    (uint8_t) val); break;
    case DT_SIGNED_SHORT:  sprintf(str, "%i",    (int16_t) val); break;
    case DT_SIGNED_INT:    sprintf(str, "%i",    (uint32_t)val); break;
    case DT_FLOAT:         sprintf(str, "%0.3f", (float)   val); break;
    case DT_DOUBLE:        sprintf(str, "%0.3f",           val); break;
    default:                                                     break;
  }
}

double analyze_read_val(dsr_t *hdr, uint8_t *img, uint32_t *dims) {

  uint16_t i;
  uint8_t  ndims;
  uint16_t dimsz;
  uint32_t off;

  if (hdr  == NULL) goto fail;
  if (img  == NULL) goto fail;
  if (dims == NULL) goto fail;
  
  ndims   = analyze_num_dims(hdr);
 
  /*check that dimension indices are valid*/
  for (i = 0; i < ndims; i++) {

    dimsz = analyze_dim_size(hdr, i);
    if (dims[i] < 0 || dims[i] >= dimsz) goto fail;
  }

  off = analyze_get_offset(hdr, dims);

  return analyze_read(hdr, img + off);

fail:
  return DBL_MAX;
}

double analyze_read(dsr_t *hdr, uint8_t *data) {

  double val;

  switch (hdr->dime.datatype) {

    case DT_UNSIGNED_CHAR:
      val = analyze_read_unsigned_char(hdr, data);
      break;

    case DT_SIGNED_SHORT:
      val = analyze_read_signed_short(hdr, data);
      break;

    case DT_SIGNED_INT:
      val = analyze_read_signed_int(hdr, data);
      break;

    case DT_FLOAT:
      val = analyze_read_float(hdr, data);
      break;

    case DT_DOUBLE:
      val = analyze_read_double(hdr, data);
      break;

    default:
      val = DBL_MAX;
  }

  return val;
}

void analyze_write_val(dsr_t *hdr, uint8_t *img, uint32_t *dims, double val) {

  uint16_t i;
  uint8_t  ndims;
  uint16_t dimsz;
  uint32_t off;

  if (hdr  == NULL) goto fail;
  if (img  == NULL) goto fail;
  if (dims == NULL) goto fail;

  ndims = analyze_num_dims(  hdr);
  off   = analyze_get_offset(hdr, dims);

  /*check that dimension indices are valid*/
  for (i = 0; i < ndims; i++) {

    dimsz = analyze_dim_size(hdr, i);
    if (dims[i] < 0 || dims[i] >= dimsz) goto fail;
  }

  analyze_write(hdr, img + off, val);

fail:
  return;
}

void analyze_write(dsr_t *hdr, uint8_t *data, double val) {
  
  switch (hdr->dime.datatype) {

    case DT_UNSIGNED_CHAR:
      analyze_write_unsigned_char(hdr, data, val);
      break;

    case DT_SIGNED_SHORT:
      analyze_write_signed_short(hdr, data, val);
      break;

    case DT_SIGNED_INT:
      analyze_write_signed_int(hdr, data, val);
      break;

    case DT_FLOAT:
      analyze_write_float(hdr, data, val);
      break;

    case DT_DOUBLE:
      analyze_write_double(hdr, data, val);
      break;
  }
}

double analyze_read_unsigned_char(dsr_t *hdr, uint8_t *data) {

  return data[0];
}

double analyze_read_signed_short(dsr_t *hdr, uint8_t *data) {

  int16_t val;

  if (hdr->rev) reverse(data, &val, 2);
  else          val = *(int16_t *)data;

  return val;
}

double analyze_read_signed_int(dsr_t *hdr, uint8_t *data) {

  int32_t val;
  
  if (hdr->rev) reverse(data, &val, 4);
  else val = *(int32_t *)data;
  
  return val;
}

double analyze_read_float(dsr_t *hdr, uint8_t *data) {

  float val;

  if (hdr->rev) reverse(data, &val, 4);
  else          val = *(float *)data;

  return val; 
}

double analyze_read_double(dsr_t *hdr, uint8_t *data) {

  double val;
  
  if (hdr->rev) reverse(data, &val, 8);
  else val = *(double *)data;

  return val;
}

void analyze_write_unsigned_char(dsr_t *hdr, uint8_t *data, double val) {

  data[0] = (uint8_t)round(val);
}

void analyze_write_signed_short(dsr_t *hdr, uint8_t *data, double val) {

  int16_t sval;
  sval = (int16_t)round(val);

  if (hdr->rev) reverse(&sval, data, 2);
  else          *((int16_t *)data) = sval;
}

void analyze_write_signed_int(dsr_t *hdr, uint8_t *data, double val) {

  int32_t ival;
  ival = (int32_t)round(val);

  if (hdr->rev) reverse(&ival, data, 4);
  else          *((int32_t *)data) = ival;
}

void analyze_write_float(dsr_t *hdr, uint8_t *data, double val) {

  float fval;
  fval = (float)val;

  if (hdr->rev) reverse(&fval, data, 4);
  else          *((float *)data) = fval;
}

void analyze_write_double(dsr_t *hdr, uint8_t *data, double val) {

  if (hdr->rev) reverse(&val, data, 8);
  else          *((double *)data) = val;
}
