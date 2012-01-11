/**
 * Output a graph, and associated statistics, in ASCII vtk format.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "util/getline.h"
#include "io/vtk.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "io/ngdb_graph.h"

#define MAX_SCALAR_FILES 50

static char doc[] = "cvtk - output a graph, and associated statistics, "\
                    "in ASCII vtk format";

static struct argp_option options[] = {
  {"degree",     'd', NULL,        0, "include degree as a node scalar"},
  {"label",      'l', NULL,        0, "include node label as a node scalar"},
  {"clustering", 'c', NULL,        0, "include clustering coefficient as a "\
                                 "node scalar"},
  {"pathlength", 'p', NULL,        0, "include path length as a node scalar "},
  {"efficiency", 'e', NULL,        0, "include local efficiency as a node "\
                                      "scalar"},
  {"scalarfile", 'f', "FILE NAME", 0, "include scalar data from the given "\
                                      "file (this option may be specificed "\
                                      "multiple times)"},
  {"omitedges",  'o', NULL,        0, "do not export edges"},
  {"omitnodes",  'm', NULL,        0, "do not export nodes"},
  {0}
};

typedef struct args {
  char   *input;
  char   *output;
  uint8_t nscalarfiles;
  char   *scalarfiles[    MAX_SCALAR_FILES];
  char   *scalarfilenames[MAX_SCALAR_FILES];
  uint8_t degree;
  uint8_t label;
  uint8_t clustering;
  uint8_t pathlength;
  uint8_t efficiency;
  uint8_t omitedges;
  uint8_t omitnodes;
} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch (key) {
    case 'd': a->degree                         = 0xFF; break;
    case 'l': a->label                          = 0xFF; break;
    case 'c': a->clustering                     = 0xFF; break;
    case 'p': a->pathlength                     = 0xFF; break;
    case 'e': a->efficiency                     = 0xFF; break;
    case 'o': a->omitedges                      = 0xFF; break;
    case 'm': a->omitnodes                      = 0xFF; break;
    case 'f':
      if (a->nscalarfiles < MAX_SCALAR_FILES) {
        
        if (state->next >= state->argc) argp_usage(state);
        
        a->scalarfiles[    a->nscalarfiles]   = arg;
        a->scalarfilenames[a->nscalarfiles++] = state->argv[state->next];

        state->next++;
      }
      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->input  = arg;
      else if (state->arg_num == 1) a->output = arg;
      else                          argp_usage(state);
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/**
 * Generates a VTK file from the given graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _print_graph(
  graph_t *g,   /**< the graph              */
  args_t  *args /**< command line arguments */
);

/**
 * Prints the scalar data as specified on the command line.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _print_node_scalars(
  FILE    *f,    /**< file handle            */ 
  graph_t *g,    /**< the graph              */
  args_t  *args  /**< command line arguments */
);

/**
 * Loads scalar data from the given file. The file must contain
 * one line per node, with each line containing a single floating
 * point value.
 *
  *\return number of values read on success, negative value on failure.
 */
static int64_t _load_file_scalar(
  char    *fname, /**< name of file to load data from                 */
  uint32_t len,   /**< maximum number of values (i.e. length of data) */
  double  *data   /**< place to store data                            */
);

int main (int argc, char *argv[]) {

  graph_t     g;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT [OUTPUT]", doc};
  
  memset(&args, 0, sizeof(args_t));
  startup("cvtk", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g) != 0) {
    printf("error loading %s\n", args.input);
    goto fail;
  }

  if (_print_graph(&g, &args)) {
    printf("error converting graph to vtk\n");
    goto fail;
  }

  return 0;
fail:
  return 1;
}

uint8_t _print_graph(graph_t *g, args_t *args) {

  FILE *fout;
  
  fout = NULL;

  if (args->output != NULL) {

    fout = fopen(args->output, "wt");
    if (fout == NULL) goto fail;
  }
  else
    fout = stdout;

  if (vtk_print_hdr(                          fout, g))       goto fail;
  if (!args->omitnodes && vtk_print_nodes(    fout, g))       goto fail;
  if (!args->omitedges && vtk_print_edges(    fout, g))       goto fail;
  if (!args->omitnodes && _print_node_scalars(fout, g, args)) goto fail;

  if (fout != stdout) fclose(fout);
  return 0;

fail:

  if (fout != NULL && fout != stdout) fclose(fout);
  return 1;
}

uint8_t _print_node_scalars(FILE *f, graph_t *g, args_t *args) {

  uint32_t       i;
  uint32_t       sidx;
  double        *data;
  uint32_t       nnodes; 
  graph_label_t *lbl;

  sidx   = 0;
  data   = NULL;
  nnodes = graph_num_nodes(g);

  data   = calloc(nnodes, sizeof(double));
  if (data == NULL) goto fail;

  if (args->degree) {
    
    for (i = 0; i < nnodes; i++) 
      data[i] = graph_num_neighbours(g, i);

    if (vtk_print_node_scalar(f, g, !(sidx++), "degree", data)) goto fail;
  }

  if (args->label) {
    for (i = 0; i < nnodes; i++) {
      
      lbl = graph_get_nodelabel(g, i);
      if (lbl == NULL) goto fail;
      
      data[i] = lbl->labelval;
    }

    if (vtk_print_node_scalar(f, g, !(sidx++), "label", data)) goto fail;
  }

  if (args->clustering) {
    if (stats_cache_node_clustering(g, -1, data))                   goto fail;
    if (vtk_print_node_scalar(f, g, !(sidx++), "clustering", data)) goto fail;
  }

  if (args->pathlength) {
    if (stats_cache_node_pathlength(g, -1, data))                   goto fail;
    if (vtk_print_node_scalar(f, g, !(sidx++), "pathlength", data)) goto fail;
  }

  if (args->efficiency) {
    if (stats_cache_node_local_efficiency(g, -1, data))             goto fail;
    if (vtk_print_node_scalar(f, g, !(sidx++), "efficiency", data)) goto fail;
  }


  for (i = 0; i < args->nscalarfiles; i++) {

    memset(data, 0, nnodes*sizeof(double));
    if (_load_file_scalar(args->scalarfiles[i], nnodes, data) < 0)
      goto fail;
    if (vtk_print_node_scalar(f, g, !(sidx++), args->scalarfilenames[i], data))
      goto fail;
  }

  free(data);
  return 0;
fail:

  if (data != NULL) free(data);
  return 1;
}

int64_t _load_file_scalar(char *fname, uint32_t len, double *data) {

  int64_t  i;
  FILE    *f;
  char    *line;
  size_t   linelen;

  f       = NULL;
  line    = NULL;
  linelen = 200;
  i       = 0;

  line = calloc(linelen, sizeof(char));
  if (line == NULL) goto fail;

  f = fopen(fname, "rt");
  if (f == NULL) goto fail;

  while (getline(&line, &linelen, f) > 0 && i < len) {

    data[i] = atof(line);
    i++;
  }

  free(line);
  fclose(f);

  return i;
  
fail:

  if (f    != NULL) fclose(f);
  if (line != NULL) free(line);
  return -1;
}
