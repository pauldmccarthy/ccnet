/**
 * Print a graph in VTK format.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdio.h>
#include <stdint.h>

#include "graph/graph.h"

#include "io/vtk.h"

uint8_t vtk_print_graph(
  FILE    *f,
  graph_t *g,
  uint8_t  nscalars,
  char   **scalar_names,
  double **scalars
)
{
  uint8_t  i;

  if (vtk_print_hdr(  f, g)) goto fail;
  if (vtk_print_nodes(f, g)) goto fail;
  if (vtk_print_edges(f, g)) goto fail;

  for (i = 0; i < nscalars; i++) {
    if (vtk_print_node_scalar(f, g, i == 0, scalar_names[i], scalars[i]))
      goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t vtk_print_hdr(FILE *f, graph_t *g) {

  uint64_t       i;
  uint32_t       npoints;
  graph_label_t *label;

  npoints = graph_num_nodes(g);

  if (fprintf(f, "# vtk DataFile Version 3.0\n") < 0) goto fail;
  if (fprintf(f, "cvtk graph\n")                 < 0) goto fail;
  if (fprintf(f, "ASCII\n")                      < 0) goto fail;
  if (fprintf(f, "DATASET POLYDATA\n")           < 0) goto fail;
  if (fprintf(f, "POINTS %u FLOAT\n", npoints)   < 0) goto fail;

  for (i = 0; i < npoints; i++) {

    label = graph_get_nodelabel(g, i);
    if (label == NULL) goto fail;

    if (fprintf(f, "%0.6f %0.6f %0.6f\n", 
        label->xval,
        label->yval,
        label->zval) < 0) goto fail;
  }
 
  return 0;

fail:
  return 1;
}

uint8_t vtk_print_nodes(FILE *f, graph_t *g) {

  uint32_t i;
  uint32_t npoints;
  
  npoints = graph_num_nodes(g);

  if (fprintf(f, "VERTICES %u %u\n", npoints, npoints*2) < 0) goto fail;

  for (i = 0; i < npoints; i++) {

    if (fprintf(f, "1 %u\n", i) < 0) goto fail;
  } 

  return 0;

fail:
  return 1;
}

uint8_t vtk_print_edges(FILE *f, graph_t *g) {

  uint32_t  i;
  uint32_t  j;
  uint32_t  nlines;
  uint32_t  npoints;
  uint32_t *nbrs;

  npoints = graph_num_nodes(g);
  nlines  = graph_num_edges(g);

  if (fprintf(f, "LINES %u %u\n", nlines, nlines*3) < 0) goto fail;

  for (i = 0; i < npoints; i++) {

    nlines = graph_num_neighbours(g, i);
    nbrs   = graph_get_neighbours(g, i);

    for (j = 0; j < nlines; j++) {

      if (nbrs[j] <= i) continue;

      if (fprintf(f, "2 %u %u\n", i, nbrs[j]) < 0) goto fail;
    }
  }

  return 0;

fail: 
  return 1;
}

uint8_t vtk_print_node_scalar(
  FILE *f, graph_t *g, uint8_t first, char *name, double *data) {

  uint32_t i;
  uint32_t len;

  len = graph_num_nodes(g);

  if (first) {

    if (fprintf(f, "POINT_DATA %u\n",       len)  < 0) goto fail;
    if (fprintf(f, "SCALARS %s double 1\n", name) < 0) goto fail;
    if (fprintf(f, "LOOKUP_TABLE default\n")      < 0) goto fail;
  }
  else {
    if (fprintf(f, "FIELD FieldData 1\n")         < 0) goto fail;
    if (fprintf(f, "%s 1 %u double\n", name, len) < 0) goto fail;
  }

  for (i = 0; i < len; i++) {
    if (fprintf(f, "%0.5f\n", data[i]) < 0) goto fail;
  }

  return 0;

fail:
  return 1;
}
