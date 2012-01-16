/**
 * Functions for reading a collection of 3D ANALYZE75 images.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>

#include "io/analyze75.h"
#include "timeseries/analyze_volume.h"

uint8_t analyze_open_volume(char *path, analyze_volume_t *vol) {

  /*get list of .img files in directory*/
  /*turn file names into prefixes */
  /*sort prefixes numerically */
  /*turn prefixes back into .img files*/
  /*open img files*/
  
  return 0;
}


void analyze_free_volume(analyze_volume_t *vol) {

  uint32_t i;

  if (vol        == NULL) return;
  if (vol->hdrs  == NULL) return;
  if (vol->imgs  == NULL) return;
  if (vol->nimgs == 0)    return;

  free(vol->hdrs);

  for (i = 0; i < vol->nimgs; i++) free(vol->imgs[i]);

  free(vol->imgs);
}
