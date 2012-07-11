/**
 * Reads an ANALYZE75 header file, and 
 * prints its contents to standard output.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "io/analyze75.h"
#include "util/startup.h"

static void print_datatype(uint16_t datatype);

static void dump_short(          dsr_t             *ds, uint8_t *img);
static void dump_overview(       dsr_t             *ds, uint8_t *img);
static void dump_header_key(     header_key_t      *hk);
static void dump_image_dimension(image_dimension_t *id);
static void dump_data_history(   data_history_t    *dh);

/**
 * Searches through the image data, and finds the minimum/maximum/meanx values.
 * This is done because the min/max data in the headers is not always correct.
 */
static void image_stats(
  dsr_t   *hdr, /**< image header                     */
  uint8_t *img, /**< image data                       */
  double  *min, /**< place to store the minimum value */
  double  *max, /**< place to store the maximum value */
  double  *mean /**< place to store the image mean    */
);

int main(int argc, char *argv[]) {

  dsr_t    dsr;
  uint8_t *img;
  uint8_t  shout;

  shout = 0;

  startup("dumphdr", argc, argv, NULL, NULL);

  if (argc != 2 && argc != 3) {
    printf("usage: dumphdr file.hdr [-s]\n");
    printf("  -s: short output\n");
    return 1;
  }

  if (argc == 3) {

    if (!strcmp(argv[2], "-s")) shout = 1;
    else {
      printf("unrecognised option: %s\n", argv[2]);
      return 1;
    }
  }

  if (analyze_load(argv[1], &dsr, &img) != 0) {
    printf("error reading header (%i)\n", errno);
    return 1;
  }

  if (shout) dump_short(&dsr, img);
  
  else{
    dump_overview(       &(dsr), img);
    dump_header_key(     &(dsr.hk));
    dump_image_dimension(&(dsr.dime));
    dump_data_history(   &(dsr.hist));
  }

  free(img);
  return 0;
}

void print_datatype(uint16_t datatype) {

  switch (datatype) {

    case DT_NONE:          printf("DT_NONE/DT_UNKNOWN\n"); break;
    case DT_BINARY:        printf("DT_BINARY\n");          break;
    case DT_UNSIGNED_CHAR: printf("DT_UNSIGNED_CHAR\n");   break;
    case DT_SIGNED_SHORT:  printf("DT_SIGNED_SHORT\n");    break;
    case DT_SIGNED_INT:    printf("DT_SIGNED_INT\n");      break;
    case DT_FLOAT:         printf("DT_FLOAT\n");           break;
    case DT_COMPLEX:       printf("DT_COMPLEX\n");         break;
    case DT_DOUBLE:        printf("DT_DOUBLE\n");          break;
    case DT_RGB:           printf("DT_RGB\n");             break;
    case DT_ALL:           printf("DT_ALL\n");             break;
    default:               printf("unknown type\n");       break;
  }
}

void dump_short(dsr_t *dsr, uint8_t *img) {

  uint32_t i;
  uint16_t dtype;
  uint8_t  dtypesz;
  uint8_t  ndims;
  uint32_t nvals;
  double   min;
  double   max;
  double   mean; 
  
  dtype   = analyze_datatype(  dsr);
  dtypesz = analyze_value_size(dsr);
  ndims   = analyze_num_dims(  dsr);
  nvals   = analyze_num_vals(  dsr);

  printf("data type:   ");
  print_datatype(dtype);
  printf("value size:  %u\n", dtypesz);
  printf("num values:  %u\n", nvals);

  printf("dimensions:  ");
  for (i = 0; i < ndims; i++) printf("%u ", analyze_dim_size(dsr, i));
  printf("\n");
  
  printf("voxel sizes: ");
  for (i = 0; i < ndims; i++) printf("%0.6f ", analyze_pixdim_size(dsr, i));
  printf("\n");

  image_stats(dsr, img, &min, &max, &mean);
  printf("minimum:     %0.6f\n", min);
  printf("maximum:     %0.6f\n", max);
  printf("mean:        %0.6f\n", mean);
  printf("endianness:  %u\n",    dsr->rev);
}

void dump_overview(dsr_t *dsr, uint8_t *img) {

  uint8_t i;
  uint8_t valsize;
  uint8_t numdims;
  uint8_t dimsize;
  int     dimoff;
  int     numvals;
  double  min;
  double  max;
  double  mean;

  valsize = analyze_value_size(dsr);
  numdims = analyze_num_dims(  dsr);
  numvals = analyze_num_vals(  dsr);

  printf("value size (bytes):   %u\n", valsize);
  printf("number of dimensions: %u\n", numdims);
  printf("number of values:     %i\n", numvals);

  for (i = 0; i < numdims; i++) {
    dimsize = analyze_dim_size(dsr, i);
    printf("dimension %u size: %u\n", i, dimsize);
  }
  
  for (i = 0; i < numdims; i++) {
    dimoff = analyze_dim_offset(dsr, i);
    printf("dimension %u offset: %u\n", i, dimoff);
  }

  image_stats(dsr, img, &min, &max, &mean);
  printf("data minimum: %0.6f\n", min);
  printf("data maximum: %0.6f\n", max);
  printf("data mean:    %0.6f\n", mean);
  printf("endianness:   %u\n",    dsr->rev);
}

void dump_header_key(header_key_t *hk) {

  char *tmp;

  printf("header_key\n");

  tmp = malloc(20);
  if (tmp == NULL) {
    printf("  malloc error\n");
    return;
  }
  tmp[19] = '\0';

  printf("  sizeof_hdr:    %u\n", hk->sizeof_hdr);

  memcpy(tmp, hk->data_type, sizeof(hk->data_type));
  printf("  data_type:     %s\n", tmp);

  memcpy(tmp, hk->db_name, sizeof(hk->db_name));
  printf("  db_name:       %s\n", tmp);

  printf("  extents:       %u\n", hk->extents);
  printf("  session_error: %u\n", hk->session_error);
  printf("  regular:       %u\n", hk->regular);
  printf("  hkey_un0:      %u\n", hk->hkey_un0);
  
  free(tmp);
}

void dump_image_dimension(image_dimension_t *id) {

  uint8_t i;

  printf("image_dimension\n");

  for (i = 0; i < 8; i++)
    printf("  dim[%u]:     %u\n", i, id->dim[i]);

  printf("  unused8:    %u\n", id->unused8);
  printf("  unused9:    %u\n", id->unused9);
  printf("  unused10:   %u\n", id->unused10);
  printf("  unused11:   %u\n", id->unused11);
  printf("  unused12:   %u\n", id->unused12);
  printf("  unused13:   %u\n", id->unused13);
  printf("  unused14:   %u\n", id->unused14);
  printf("  datatype:   "); print_datatype(id->datatype);
  printf("  bitpix:     %u\n", id->bitpix);
  printf("  dim_un0:    %u\n", id->dim_un0);

  for (i = 0; i < 8; i++)
    printf("  pixdim[%u]:  %f\n", i, id->pixdim[i]);

  printf("  vox_offset: %f\n", id->vox_offset);
  printf("  funused1:   %f\n", id->funused1);
  printf("  funused2:   %f\n", id->funused2);
  printf("  funused3:   %f\n", id->funused3);
  printf("  cal_max:    %f\n", id->cal_max);
  printf("  cal_min:    %f\n", id->cal_min);
  printf("  compressed: %f\n", id->compressed);
  printf("  verified:   %f\n", id->verified);
  printf("  glmax:      %u\n", id->glmax);
  printf("  glmin:      %u\n", id->glmin);
}

void dump_data_history( data_history_t *dh) {
 
  char *tmp;

  printf("data_history\n");

  tmp = malloc(100);
  if (tmp == NULL) {
    printf("  malloc error\n");
    return;
  }

  tmp[99] = '\0';

  memcpy(tmp, dh->descrip, sizeof(dh->descrip));
  printf("  descrip:     %s\n", tmp);

  memcpy(tmp, dh->aux_file, sizeof(dh->aux_file));
  printf("  aux_file:    %s\n", tmp);

  printf("  orient:      %u\n", dh->orient);

  memcpy(tmp, dh->originator, sizeof(dh->originator));
  printf("  originator:  %s\n", tmp);

  memcpy(tmp, dh->scannum, sizeof(dh->scannum));
  printf("  scannum:     %s\n", tmp);

  memcpy(tmp, dh->patient_id, sizeof(dh->patient_id));
  printf("  patient_id:  %s\n", tmp);

  memcpy(tmp, dh->exp_date, sizeof(dh->exp_date));
  printf("  exp_date:    %s\n", tmp);

  memcpy(tmp, dh->exp_time, sizeof(dh->exp_time));
  printf("  exp_time:    %s\n", tmp);

  memcpy(tmp, dh->hist_un0, sizeof(dh->hist_un0));
  printf("  hist_un0:    %s\n", tmp);

  printf("  views:       %u\n", dh->views);
  printf("  vols_added:  %u\n", dh->vols_added);
  printf("  start_field: %u\n", dh->start_field);
  printf("  field_skip:  %u\n", dh->field_skip);
  printf("  omax:        %u\n", dh->omax);
  printf("  omin:        %u\n", dh->omin);
  printf("  smax:        %u\n", dh->smax);
  printf("  smin:        %u\n", dh->smin);

  free(tmp);
}

void image_stats(
  dsr_t *hdr, uint8_t *img, double *min, double *max, double *mean) {

  uint64_t i;
  uint32_t nvals;
  double   val;
  double   lmin;
  double   lmax;
  double   tally;

  lmin  =  DBL_MAX;
  lmax  = -DBL_MAX;
  tally = 0;

  nvals = analyze_num_vals(  hdr);

  for (i = 0; i < nvals; i++) {

    val = analyze_read_by_idx(hdr, img, i);

    tally += val;

    if (val < lmin) lmin = val;
    if (val > lmax) lmax = val;
  }

  *min  = lmin;
  *max  = lmax;
  *mean = tally / nvals;
}
