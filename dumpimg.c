/**
 * Reads in data from an ANALYZE75 image file, and writes 
 * it to standard output.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <argp.h>

#include "io/analyze75.h"

/**
 * This function is called to generate the dimension
 * ordering (the fastest-to-slowest changing dimension,
 * which determines the order in which values are 
 * printed),
 *
 * \return 0 on success, non-0 otherwise.
 */
static uint8_t _parse_dimorder(
  dsr_t   *hdr,     /**< image header                  */
  char    *order[], /**< orderings that were passed in */
  uint8_t *dims,    /**< space to store ordering       */
  uint8_t  orderlen /**< length of order (and dims)    */
);

/**
 * Prints current voxel coordinates to stdout.
 */
static void _dump_coords(
  dsr_t    *hdr,   /**< image header                                  */
  uint32_t *dims,  /**< voxel indices                                 */
  char      pcoord /**< 'r' -> print real units, 'v' -> print indices */
);

/**
 * Prints the image to standard out.
 */
static void _dumpimg(
  dsr_t   *hdr,      /**< image header                                  */
  uint8_t *image,    /**< the image                                     */
  uint8_t *dimorder, /**< dimension order (fastest to slowest changing) */
  char     pcoord    /**< whether to print voxel coordinates            */
);

/**
 * Formats the value stored in data, which is of type datatype. 
 */
static void _sprint_val(
  uint16_t datatype, /**< type of value to be formatted   */
  char    *str,      /**< string to store formatted value */
  double   val       /**< the raw value                   */
);

int main(int argc, char *argv[]) {

  dsr_t    hdr;
  uint8_t *data;
  uint8_t  dooff;
  uint8_t *dimorder;
  char     pcoord;

  dooff    = 2;
  pcoord   = 0;
  data     = NULL;
  dimorder = NULL;

  if (argc < 2) {
    printf("usage: dumpimg file.img [-r|-v] [dimension order]\n" 
           "  -r: print voxel coordinates as real units\n"
           "  -v: print voxel coordinates as indices\n");
    goto fail;
  }

  if (argc > 2 && argv[2][0] == '-') {
    dooff  ++;
    pcoord = argv[2][1];
  }

  if (analyze_load(argv[1], &hdr, &data)) {
    printf("error loading image (%i)\n", errno);
    goto fail;
  }

  /*create the dimension order*/
  dimorder = malloc(analyze_num_dims(&hdr));
  if (dimorder == NULL) goto fail;

  _parse_dimorder(&hdr, argv+dooff, dimorder, argc-dooff);

  _dumpimg(&hdr, data, dimorder, pcoord);

  free(data);
  free(dimorder);
  exit(0);

 fail:
  if (data     != NULL) free(data);
  if (dimorder != NULL) free(dimorder);
  exit(1);
}

uint8_t _parse_dimorder(
  dsr_t   *hdr,
  char    *order[],
  uint8_t *dims,
  uint8_t  orderlen)
{
  uint8_t i,j;
  uint8_t ndims;

  ndims = analyze_num_dims(hdr);

  if (orderlen > ndims) goto fail;

  for (i = 0; i < orderlen; i++) {

    if (strlen(order[i]) != 1) goto fail;
    if (!isdigit(order[i][0])) goto fail;

    dims[i] = atoi(order[i]);

    if (dims[i] >= ndims) goto fail;
  }

  /*
   * If the user didn't specify all dimensions in 
   * the ordering, complete the ordering by finding 
   * those dimensions not specified in the ordering, 
   * and adding them to the ordering, after the 
   * dimensions that were specified in the ordering. 
   * Ordering ordering ordering.
   */
  for (i = 0; orderlen < ndims && i < ndims; i++) {

    for (j = 0; j < orderlen; j++) if (dims[j] == i) break;
    if (j < orderlen) continue;

    dims[orderlen++] = i;
  }

  return 0;

fail: 
  return 1;
}

void _dump_coords(dsr_t *hdr, uint32_t *dims, char pcoord) {

  float   real;
  uint8_t i;
  uint8_t ndims;

  ndims = analyze_num_dims(hdr) - 1;

  if (pcoord == 'r') {
    for (i = 0; i < ndims; i++) {
      real = analyze_pixdim_size(hdr, i);
      printf("%0.2f ", dims[i]*real);      
    }
  }
  else if (pcoord == 'v') {
    for (i = 0; i < ndims; i++) {
      printf("%u ", dims[i]);
    }
  }
}

void _dumpimg(
  dsr_t *hdr, uint8_t *image, uint8_t *dimorder, char pcoord) {

  uint32_t  i, j;
  uint16_t  datatype;
  uint8_t   ndims;
  uint32_t  nvals;
  uint8_t   valsize;
  uint32_t *dims;
  uint32_t *dimsz;
  uint32_t *dimoffs;
  double    val;
  char      str[30];

  dims    = NULL;
  dimsz   = NULL;
  dimoffs = NULL;

  ndims    = analyze_num_dims(  hdr);
  nvals    = analyze_num_vals(  hdr);
  valsize  = analyze_value_size(hdr);
  datatype = analyze_datatype(  hdr);

  dims    = malloc(ndims*sizeof(int));
  if (dims    == NULL) goto fail;
  dimsz   = malloc(ndims*sizeof(int));
  if (dimsz   == NULL) goto fail;
  dimoffs = malloc(ndims*sizeof(int));
  if (dimoffs == NULL) goto fail;

  /* 
   * intialise dimension indices, 
   * and store local copy of 
   * dimension offsets and sizes
   */
  for (i = 0; i < ndims; i++) {
    dims   [i] = 0;
    dimoffs[i] = analyze_dim_offset(hdr, i);
    dimsz  [i] = analyze_dim_size(  hdr, i);
  }

  /*print out the image, one value at a time*/
  for (i = 0; i < nvals; i++) {

    /*read the value from the image*/
    val = analyze_read_val(hdr, image, dims);

    /*print coordinates if necessary*/
    if (pcoord) _dump_coords(hdr, dims, pcoord);

    /*format/print the value*/
    _sprint_val(datatype, str, val);
    printf("%s\n", str);

    /*update dimension indices*/
    for (j = 0; j < ndims; j++) {
      dims[dimorder[j]] = (dims[dimorder[j]] + 1) % dimsz[dimorder[j]];
      if (dims[dimorder[j]] != 0) break;
    }
  }

  free(dims);
  free(dimsz);
  free(dimoffs);

  return;

fail:
  if (dims    != NULL) free(dims);
  if (dimsz   != NULL) free(dimsz);
  if (dimoffs != NULL) free(dimoffs);
  return;
}

void _sprint_val(uint16_t datatype, char *str, double val) {
  switch (datatype) {

    case DT_UNSIGNED_CHAR: sprintf(str, "%u",    (uint8_t) val); break;
    case DT_SIGNED_SHORT:  sprintf(str, "%i",    (int16_t) val); break;
    case DT_SIGNED_INT:    sprintf(str, "%i",    (uint32_t)val); break;
    case DT_FLOAT:         sprintf(str, "%0.3f", (float)   val); break;
    case DT_DOUBLE:        sprintf(str, "%0.3f",           val); break;
    default:                                                     break;
  }
}
