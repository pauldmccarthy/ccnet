/**
 * Program which converts a NIFTI-1 header file to an ANALYZE75 header.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "io/analyze75.h"
#include "io/nifti1.h"

#define MAX(X,Y) ((X > Y) ? (X) : (Y))

int main (int argc, char *argv[]) {

  nifti1_hdr_t nhdr;
  dsr_t        ahdr;
  char        *infile;
  char        *outfile;

  if (argc != 3) {
    printf("usage: nifticnv infile outfile\n");
    goto fail;
  }

  infile  = argv[1];
  outfile = argv[2];

  if (nifti1_load_hdr(infile, &nhdr)) {
    printf("error reading NIFTI-1 header file\n");
    goto fail;
  }

  if (nifti1_to_analyze(&nhdr, &ahdr)) {
    printf("error converting NIFTI-1 to ANALYZE75 - "\
           "check that datatypes are compatible\n");
    goto fail;
  }

  if (analyze_write_hdr(outfile, &ahdr)) {
    printf("error writing ANALYZE75 header file\n");
    goto fail;
  }

  return 0;

fail:
  return 1;
}
