/**
 * Converts an ANALYZE75 volume to a plain text file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>

#include "util/startup.h"

int main (int argc, char *argv[]) {

  startup("dumpvolume", argc, argv, NULL, NULL);



  return 0;
  
fail:

  return 1;
}
