/**
 * Generates a graph from a .mat file containing a correlation matrix. The
 * matrix file is assumed to be symmetric.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <math.h>
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "io/mat.h"
#include "io/ngdb_graph.h"
#include "util/startup.h"
#include "graph/graph.h"
#include "graph/graph_log.h"

#define MAX_LABELS 50

typedef struct __args {

  char    *input;
  char    *output;
  char    *hdrmsg;
  uint8_t  absval;
  uint8_t  weighted;
  uint8_t  directed;
  double   threshold;
  uint8_t  reverse;
  uint8_t  ninclbls;
  uint8_t  nexclbls;
  
  double   inclbls[MAX_LABELS];
  double   exclbls[MAX_LABELS];  

} args_t;

static char doc[] =
  "tsgraph -- generate a graph from a .mat file";


static struct argp_option options[] = {
  {"hdrmsg",    'h', "MSG",   0,
   "message to save to .ngdb file header"},
  {"absval",    'a',  NULL,   0,
   "use absolute correlation value (default: false)"},
  {"weighted",  'w',  NULL,   0,
   "create weighted graph (default: false)"},
  {"directed",  'd',  NULL,   0,
   "create directed graph (default: false)"}, 
  {"threshold", 't', "FLOAT", 0,
   "discard correlation values below this (default: 0.9)"},
  {"reverse",   'r',  NULL,   0,
   "discard correlation values above the threshold, \
    rather than below (default: false)"}, 
  {"incl",      'i', "FLOAT", 0,
   "include only rows/columns with this label"},
  {"excl",      'e', "FLOAT", 0,
   "exclude rows/columns with this label"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;
  args = state->input;

  switch (key) {
    case 'h': args->hdrmsg    = arg;       break;
    case 'a': args->absval    = 1;         break;
    case 'w': args->weighted  = 1;         break;
    case 't': args->threshold = atof(arg); break;
    case 'r': args->reverse   = 1;         break;
      
    case 'i':
      if (args->ninclbls < MAX_LABELS) 
        args->inclbls[(args->ninclbls)++] = atof(arg);
      break;
      
    case 'e':
      if (args->nexclbls < MAX_LABELS) 
        args->exclbls[(args->nexclbls)++] = atof(arg);
      break; 

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 2) argp_usage(state);
      break;      

    default:
      return ARGP_ERR_UNKNOWN;

  }

  return 0;
}

/**
 * Compiles a list of row/column IDs (i.e. node IDs) from the given
 * include/exclude lists. The node IDs are stored in the given nodes
 * array, and the number of nodes returned. The nodes array must already
 * be allocated, and have the length of the matrix rows/columns.
 *
 * \return number of nodes on success, negative on failure.
 */
static int64_t _apply_label_mask(
  mat_t    *mat,      /**< the matrix file                           */
  double   *inclbls,  /**< include only rows/columns with this label */
  double   *exclbls,  /**< exclude rows/columns with this label      */
  uint8_t   ninclbls, /**< number of labels in include list          */
  uint8_t   nexclbls, /**< number of labels in exclude list          */
  uint32_t *nodes     /**< place to store row/column/node IDs        */
);

/**
 * \return 1 if the label should be included, 0 otherwise.
 */
static uint8_t _check_label(
  double   *inclbls,  /**< include only labels in this list */
  double   *exclbls,  /**< exclude labels in this list      */
  uint8_t   ninclbls, /**< number of labels in include list */
  uint8_t   nexclbls, /**< number of labels in include list */
  double    lblval    /**< label to check                   */
);

/**
 * Adds edges between the nodes in the graph, according to the correlation
 * values in the matrix file.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _connect_graph(
  mat_t    *mat,       /**< mat file                             */
  graph_t  *graph,     /**< initialised empty graph              */
  uint32_t *nodes,     /**< row/column/node ids to include       */
  uint32_t  nnodes,    /**< number of nodes                      */
  double    threshold, /**< ignore correlation values below this */
  uint8_t   absval,    /**< use absolute correlation value       */
  uint8_t   reverse    /**< ignore correlation values above the
                            threshold, rather than below         */
);

/**
 * Copies the row labels from the mat file to the graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _copy_labels(
  mat_t    *mat,   /**< mat file                     */
  graph_t  *graph, /**< initialised graph            */
  uint32_t *nodes, /**< row/column ids into mat file */
  uint32_t  nnodes /**< number of nodes              */
);

int main (int argc, char *argv[]) {

  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};
  mat_t      *mat;
  graph_t     graph;
  uint32_t   *nodes;
  uint16_t    mathdrlen;
  char       *mathdrmsg;
  int64_t     nnodes;

  mathdrmsg = NULL;
  nodes     = NULL;

  memset(&args, 0, sizeof(args));
  args.threshold = 0.9;

  startup("tsgraph", argc, argv, &argp, &args);

  /*open mat file*/
  mat = mat_open(args.input);
  if (mat == NULL) {
    printf("error opening mat file %s\n", args.input);
    goto fail;
  }

  /*figure out which rows/columns to include*/
  nodes = calloc(mat_num_rows(mat), sizeof(uint32_t));
  if (nodes == NULL) {
    printf("out of memory?!?\n");
    goto fail;
  }
  
  nnodes = _apply_label_mask(
    mat,
    args.inclbls,
    args.exclbls,
    args.ninclbls,
    args.nexclbls,
    nodes);
  
  if (nnodes < 0) {
    printf("error masking rows/columns\n");
    goto fail;
  }

  /*create graph */
  if (graph_create(&graph, nnodes, args.directed)) {
    printf("error creating graph\n");
    goto fail;
  }

  /*connect graph */
  if (_connect_graph(
        mat,
        &graph,
        nodes,
        nnodes,
        args.threshold,
        args.absval,
        args.reverse)) {
    printf("error connecting graph\n");
    goto fail;
  }

  /*copy row labels into graph*/
  if (_copy_labels(mat, &graph, nodes, nnodes)) {
    printf("error copying labels\n");
    goto fail;
  }

  /*write header message, including header from mat file*/
  mathdrlen = mat_hdr_data_size(mat);
  mathdrmsg = calloc(mathdrlen, 1);
  if (mathdrmsg == NULL) {
    printf("out of memory?!?\n");
    goto fail;
  }
  
  if (mat_read_hdr_data(mat, mathdrmsg)) {
    printf("error reading header data from %s\n", args.input);
    goto fail;
  }

  if ((strlen(mathdrmsg) > 0) || args.hdrmsg != NULL) {
    if (graph_log_init(&graph)) {
      printf("error initialising graph log\n");
      goto fail;
    }
  }

  if ((strlen(mathdrmsg) > 0) && graph_log_import(&graph, mathdrmsg, "\n")) {
    printf("error adding header message: %s\n", mathdrmsg);
    goto fail;
  }
  if ((args.hdrmsg != NULL) && graph_log_add(&graph, args.hdrmsg)) {
    printf("error adding header message: %s\n", args.hdrmsg);
    goto fail;
  }
  
  /*write graph to file*/
  if (ngdb_write(&graph, args.output)) {
    printf("error writing graph to %s\n", args.output);
    goto fail;
  }

  graph_free(&graph);
  free(nodes);
  mat_close(mat);
  return 0;

fail:
  if (nodes  != NULL) free(nodes);
  return 1;
}

int64_t _apply_label_mask(
  mat_t    *mat,
  double   *inclbls,
  double   *exclbls,
  uint8_t   ninclbls,
  uint8_t   nexclbls,
  uint32_t *nodes) {

  uint64_t      i;
  uint32_t      nrows;
  uint32_t      nnodes;
  graph_label_t lbl;

  nrows = mat_num_rows(mat);

  if (ninclbls == 0 && nexclbls == 0) {
    
    for (i = 0; i < nrows; i++) nodes[i] = i;
    return nrows;
  }

  for (i = 0, nnodes = 0; i < nrows; i++) {
    
    if (mat_read_row_label(mat, i, &lbl)) goto fail;

    if (_check_label(inclbls, exclbls, ninclbls, nexclbls, lbl.labelval))
      nodes[nnodes++] = i;
  }

  return nnodes;

fail:
  return -1;
}

uint8_t _check_label(
  double   *inclbls,
  double   *exclbls,
  uint8_t   ninclbls,
  uint8_t   nexclbls,
  double    lblval) {

  uint64_t i;

  if (ninclbls == 0 && nexclbls == 0) return 1;

  for (i = 0; i < nexclbls; i++) {
    if (lblval == exclbls[i]) return 0;
  }

  if (ninclbls == 0) return 1;

  for (i = 0; i < ninclbls; i++) {
    if (lblval == inclbls[i]) return 1;
  }

  return 0;
}

uint8_t _connect_graph(
  mat_t    *mat,
  graph_t  *graph,
  uint32_t *nodes,
  uint32_t  nnodes,
  double    threshold,
  uint8_t   absval,
  uint8_t   reverse) {

  uint64_t i;
  uint64_t j;
  double   corrval;
  double   corrvalcpy;
  uint8_t  addedge;

  for (i = 0; i < nnodes; i++) {
    for (j = i+1; j < nnodes; j++) {

      corrval    = mat_read_elem(mat, nodes[i], nodes[j]);
      corrvalcpy = corrval;
      
      if (absval) corrval = fabs(corrval);

      if (!reverse) addedge = corrval >= threshold;
      else          addedge = corrval <= threshold;
      
      if (addedge) {
        if (graph_add_edge(graph, i, j, corrvalcpy))
          goto fail;
      }
    }
  }

  return 0;
  
fail:
  return 1;
}

uint8_t _copy_labels(
  mat_t *mat, graph_t *g, uint32_t *nodes, uint32_t nnodes) {

  uint64_t      i;
  graph_label_t lbl;

  for (i = 0; i < nnodes; i++) {

    if (mat_read_row_label( mat, nodes[i], &lbl)) goto fail;
    if (graph_set_nodelabel(g,   i,        &lbl)) goto fail;
  }

  return 0;

fail:
  return 1;
}
