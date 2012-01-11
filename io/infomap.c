/**
 * Read in an infomap .tree file, and convert to a node_partition_t struct.
 *
 * See http://www.tp.umu.se/~rosvall/code.html
 *
 *
 * A .tree file contains a single header line, and then one line for
 * every node in the graph; here's an example:
 *
 * # Code length 3.32773 in 2 modules.
 * 1:1 0.0820133 "Node 1"
 * 1:2 0.0790863 "Node 4"
 * 1:3 0.0459137 "Node 3"
 * 2:1 0.0429867 "Node 2"
 * 2:2 0.0820133 "Node 5"
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/getline.h"
#include "graph/graph.h"
#include "util/array.h"
#include "util/compare.h"
#include "io/infomap.h"

/**
 * Reads the number of modules from the header line. Allocates memory for
 * module arrays.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_nmodules(
  char             *hdrline, /**< string containing first line of tree file */
  node_partition_t *infomap  /**< empty infomap_t struct                    */
);

/**
 * Reads one line from the tree file, adds the corresponding node ID
 * to the corresponding module array in the given infomap struct.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_node(
  char             *nodeline, /**< string containing module ID and node ID */
  node_partition_t *infomap   /**< infomap_t struct                        */
);

uint8_t infomap_load(char *fname, node_partition_t *infomap) {

  FILE    *f;
  uint64_t i;
  char    *line;
  size_t   len;

  f                 = NULL;
  line              = NULL;
  len               = 0;
  infomap->nparts   = 0;
  infomap->nnodes   = 0;
  infomap->parts    = NULL;

  f = fopen(fname, "rt");
  if (f == NULL) goto fail;

  /* read number of modules from header line */
  if (getline(&line, &len, f) < 0)   goto fail;
  if (_read_nmodules(line, infomap)) goto fail;
  if (infomap->nparts == 0)          goto fail;

  /* read node lines */
  while (1) {

    if (getline(&line, &len, f) < 0) break;

    if (_read_node(line, infomap)) goto fail;

    free(line);
    line = NULL;
  }

  if (infomap->nnodes == 0) goto fail;

  if (line != NULL) free(line);
  return 0;

fail:
  
  if (line != NULL) free(line);
  if (f    != NULL) fclose(f);

  if (infomap->parts != NULL) {
    
    for (i = 0; i < infomap->nparts; i++) {
      
      if (infomap->parts[i].data != NULL)
        array_free(&infomap->parts[i]);
    }

    free(infomap->parts);
    infomap->parts = NULL;
  }
  
  return 1;
}

uint8_t _read_nmodules(char *hdrline, node_partition_t *infomap) {

  uint64_t i;
  uint32_t nparts;
  double   code_length;
  char     fmt [] = "# Code length %f in %u modules.";

  if (sscanf(hdrline, fmt, &code_length, &nparts) != 2)
    goto fail;

  infomap->nparts = nparts;

  /* create arrays to contain node IDs */
  infomap->parts = calloc(infomap->nparts, sizeof(array_t));
  if (infomap->parts == NULL) goto fail;
  for (i = 0; i < infomap->nparts; i++) {
    
    if (array_create(&infomap->parts[i], sizeof(uint32_t), 10))
      goto fail;
    array_set_cmps(&infomap->parts[i], compare_u32, compare_u32_insert);
  }
  
  return 0;

fail:
  return 1;
}

uint8_t _read_node(char *nodeline, node_partition_t *infomap) {

  uint32_t module;
  uint32_t rank;
  double   length;
  uint32_t node;
  char     fmt [] = "%u:%u %f \"%u\"";

  if (sscanf(nodeline, fmt, &module, &rank, &length, &node) != 4)
    goto fail;

  if (module == 0 || module > infomap->nparts)
    goto fail;

  if (array_insert_sorted(&infomap->parts[module-1],
                          &node,
                          1,
                          NULL))
    goto fail;

  infomap->nnodes ++;
  
  return 0;
  
fail:
  return 1;
}
