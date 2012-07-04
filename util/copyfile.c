/**
 * Function which copies a file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1048576

uint8_t copyfile(char *src, char *dst) {

  size_t nread;
  char  *buf;
  FILE  *inf;
  FILE  *outf;
  
  buf  = NULL;
  inf  = NULL;
  outf = NULL;

  inf  = fopen(src, "rb");
  if (inf == NULL) goto fail;
  
  outf = fopen(dst, "wb");
  if (outf == NULL) goto fail;

  buf = malloc(BUF_SIZE);
  if (buf == NULL) goto fail;

  while (feof(inf) == 0) {

    clearerr(inf);
    clearerr(outf);
    
    nread = fread(buf, 1, BUF_SIZE, inf);

    if (ferror(inf)) goto fail;
    if (nread == 0)  goto fail;

    if (fwrite(buf, 1, nread, outf) != nread) goto fail;

    if (ferror(outf)) goto fail;
  }

  fclose(inf);
  fclose(outf);
  free(buf);

  return 0;

fail:
  if (inf  != NULL) fclose(inf);
  if (outf != NULL) fclose(outf);
  if (buf  != NULL) free(buf);

  return 1;
}
