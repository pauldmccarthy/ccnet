/**
 * Read in simple text based graph files. See io/edgefile.h for more details.
 *
 * Author: Paul McCarthy <pauldmccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "io/edgefile.h"
#include "graph/graph.h"
#include "io/ngdb_graph.h"




uint8_t edgefile_read(graph_t *g, uint32_t nnodes, char *fname) {

  FILE  *f;
  char  *line;
  size_t len;

  uint32_t u;
  uint32_t v;

  line = malloc(64);

  if (line == NULL) goto fail;

  f = fopen(fname, "rt");

  if (f == NULL) goto fail;

  if (graph_create(g, nnodes, 0)) goto fail;

  while (getline(&line, &len, f) != -1) {

    sscanf(line, "%u %u", &u, &v);

    if (graph_add_edge(g, u, v, 1)) goto fail;
  }

  fclose(f);

  free(line);
  return 0;

fail:
  return 1;
}  
