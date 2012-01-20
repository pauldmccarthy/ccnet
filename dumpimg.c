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
#include "util/startup.h"
#include "util/dimorder.h"

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

  startup("dumpimg", argc, argv, NULL, NULL);

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

  dimorder_parse(&hdr, argv+dooff, dimorder, argc-dooff);

  _dumpimg(&hdr, data, dimorder, pcoord);

  free(data);
  free(dimorder);
  exit(0);

 fail:
  if (data     != NULL) free(data);
  if (dimorder != NULL) free(dimorder);
  exit(1);
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

  uint32_t  i;
  uint8_t   ndims;
  uint32_t  nvals;
  uint32_t *dims;
  double    val;
  char      str[30];

  dims     = NULL;
  ndims    = analyze_num_dims(hdr);
  nvals    = analyze_num_vals(hdr);

  dims = calloc(ndims, sizeof(int));
  if (dims == NULL) goto fail;

  /*print out the image, one value at a time*/
  for (i = 0; i < nvals; i++) {

    /*read the value from the image*/
    val = analyze_read_val(hdr, image, dims);
    
    /*print coordinates if necessary*/
    if (pcoord) _dump_coords(hdr, dims, pcoord);

    analyze_sprint_val(hdr, str, val);
    printf("%s\n", str);

    /* update dimension indices */
    dimorder_next(hdr, dims, dimorder);
  }

  free(dims);

  return;

fail:
  if (dims != NULL) free(dims);
  return;
}
