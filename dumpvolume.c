/**
 * Converts an ANALYZE75 volume to a plain text file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <argp.h>

#include "io/analyze75.h"
#include "util/startup.h"
#include "util/dimorder.h"
#include "timeseries/analyze_volume.h"

static uint8_t _dumpvol(
  analyze_volume_t *vol,
  uint8_t          *dimorder
);

int main (int argc, char *argv[]) {

  uint8_t         *dimorder;
  analyze_volume_t vol;

  dimorder = NULL;

  startup("dumpvolume", argc, argv, NULL, NULL);

  if (argc < 2) {
    printf("usage: dumpvolume volpath [dimension order]\n");
    goto fail;
  }

  if (analyze_open_volume(argv[1], &vol)) {
    printf("error opening volume: %s\n", argv[1]);
    goto fail;
  }

  /*create the dimension order*/
  dimorder = malloc(analyze_num_dims(vol.hdrs));
  if (dimorder == NULL) {
    printf("out of memory?\n");
    goto fail;
  }

  dimorder_parse(vol.hdrs, argv+2, dimorder, argc-2);

  if (_dumpvol(&vol, dimorder)) {
    printf("error dumping volume\n");
    goto fail;
  }

  analyze_free_volume(&vol);
  return 0;
  
fail:

  return 1;
}

uint8_t _dumpvol(analyze_volume_t *vol, uint8_t *dimorder) {

  uint64_t i, j;
  uint32_t nvals;
  double  *tsdata;
  uint32_t dims[4];
  char     str[20];

  memset(dims, 0, sizeof(dims));

  tsdata = NULL;
  nvals  = analyze_num_vals(vol->hdrs);

  tsdata = calloc(vol->nimgs, sizeof(double));
  if (tsdata == NULL) goto fail;

  for (i = 0; i < nvals; i++) {

    analyze_read_timeseries(vol, dims[0], dims[1], dims[2], tsdata);
    dimorder_next(vol->hdrs, dims, dimorder);

    for (j = 0; j < vol->nimgs; j++) {
      
      analyze_sprint_val(vol->hdrs, str, tsdata[j]);
      printf("%s", str);
      if (j < (vol->nimgs-1)) printf(" ");
    }
    printf("\n");
  }

  free(tsdata);
  return 0;

fail:

  if (tsdata != NULL) free(tsdata);
  return 1;
}
