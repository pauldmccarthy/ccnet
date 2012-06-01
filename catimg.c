/**
 * Program which concatenates a series of images into a single volume.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "io/analyze75.h"
#include "util/startup.h"
#include "util/suffix.h"
#include "util/filesize.h"

/**
 * Creates a new file which is the concatenation of the given input files.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _concat(
  char     *output, /**< name of output file   */
  char    **inputs, /**< names of input files  */
  uint16_t  ninputs /**< number of input files */
);

/**
 * Creates a header for the new concatenated image.
 */
static void _mk_hdr(
  dsr_t   *hdr,        /**< place to put new header */
  dsr_t   *input_hdrs, /**< input headers           */
  uint16_t ninputs,    /**< number of input headers */
  float    dimsz       /**< size of new dimension   */
);

int main (int argc, char *argv[]) {

  char     *output;
  char    **inputs;
  dsr_t     newhdr;
  dsr_t    *hdrs;
  uint16_t  i;
  uint16_t  ninputs;
  float     dimsz;

  hdrs = NULL;

  startup("catimg", argc, argv, NULL, NULL);

  if (argc < 5) {
    printf("usage: catimg output dimsz input input [input ...]\n");
    goto fail;
  }

  ninputs = argc-3;
  output  = argv[1];
  dimsz   = atof(argv[2]);
  inputs  = argv+3;

  hdrs = malloc(ninputs*sizeof(dsr_t));
  if (hdrs == NULL) goto fail;

  for (i = 0; i < ninputs; i++) {
    if (analyze_load_hdr(inputs[i], hdrs+i)) goto fail;
  }

  if (analyze_hdr_compat(ninputs, hdrs, 0)) goto fail;

  _mk_hdr(&newhdr, hdrs, ninputs, dimsz);

  if (_concat(output, inputs, ninputs)) goto fail;

  if (analyze_write_hdr(output, &newhdr)) goto fail;
  
  free(hdrs);
  return 0;

fail:
  printf("Cat failed\n");
  if (hdrs != NULL) free(hdrs);
  return 1;
}

uint8_t _concat(char *filename, char **inputs, uint16_t ninputs) {

  uint16_t i;
  int      size;
  uint8_t *buf;
  FILE    *outf;
  FILE    *inf;
  char    *infname;

  buf       = NULL;
  outf      = NULL;
  inf       = NULL;

  filename = set_suffix(filename, "img");
  if (filename == NULL) goto fail;
  
  outf = fopen(filename, "wb");
  if (outf == NULL) goto fail;

  for (i = 0; i < ninputs; i++, inf = NULL, buf = NULL) {

    infname = set_suffix(inputs[i], "img");
    if (infname == NULL) goto fail;

    inf = fopen(infname, "rb");
    if (inf == NULL) goto fail;

    size = filesize(inf);
    buf  = malloc(size);

    if (buf == NULL) goto fail;

    if (fread( buf, 1, size, inf)  != size) goto fail;
    if (fwrite(buf, 1, size, outf) != size) goto fail;

    fclose(inf);
    free(buf);
    free(infname);
    infname = NULL;
    inf     = NULL;
  }
 
  fclose(outf);
  free(filename);
  return 0;

 fail:
  if (outf     != NULL) fclose(outf);
  if (inf      != NULL) fclose(inf);
  if (filename != NULL) free(filename);
  if (buf      != NULL) free(buf);
  return 1;
}

void _mk_hdr(dsr_t *hdr, dsr_t *input_hdrs, uint16_t ninputs, float dimsz) {

  uint8_t  ndims;

  memcpy(hdr, input_hdrs, sizeof(dsr_t));

  ndims = analyze_num_dims(hdr);
  hdr->dime.dim   [0]       = ndims+1;
  hdr->dime.dim   [ndims+1] = ninputs;
  hdr->dime.pixdim[ndims+1] = dimsz;
}
