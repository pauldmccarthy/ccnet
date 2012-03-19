/**
 * Program to extract subgraphs by label.
 *
 * This program may be used to extract a subgraph from a parent graph. Nodes
 * to be included in the subgraph are selected either by their label value, or
 * by component number.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include "stats/stats.h"
#include "graph/graph.h"
#include "graph/graph_log.h"
#include "graph/graph_mask.h"
#include "util/startup.h"
#include "util/array.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"

/**
 * Maximum number of label values/components
 * that can be passed in on the command line.
 */
#define MAX_LABEL_VALUES 50

/**
 * Input arguments.
 */
typedef struct _args {
  char    *input;                    /**< name of input file               */
  char    *output;                   /**< name of output file              */
  char    *lblfile;                  /**< ANALYZE75 file containing
                                          node labels                      */
  uint8_t  real;                     /**< node coordinates are in
                                          real units                       */
  char    *hdrmsg;                   /**< message to add to output file    */
  uint8_t  component;                /**< extract by component
                                          instead of by label              */ 
  uint8_t  exclude;                  /**< exclude by label/component
                                          instead of include               */
  uint32_t labels[MAX_LABEL_VALUES]; /**< labels (or components) to
                                          include in subgraph              */
  uint8_t  nlabels;                  /**< number of labels (or components) */
} args_t;

static char doc[]   = "cextract - extract a subgraph by "\
                      "label value or component";

static struct argp_option options[] = {
  {"component", 'c',  NULL,  0, "extract by component instead of by label"},
  {"exclude",   'e',  NULL,  0, "exclude by label/component, "\
                                "instead of include"},
  {"hdrmsg",    'h', "MSG",  0, "message to save to .ngdb file header"},  
  {"lblval",    'l', "INT",  0, "label/component value/number"},
  {"lblfile",   'f', "FILE", 0, "ANALYZE75 file containing node labels"},
  {"real",      'r',  NULL,  0, "node coordinates are in real units"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {
  
  args_t *args;

  args = state->input;

  switch(key) {

    case 'c': args->component = 1;   break;
    case 'e': args->exclude   = 1;   break;
    case 'f': args->lblfile   = arg; break;
    case 'r': args->real      = 1;   break;
    case 'h': args->hdrmsg    = arg; break;
    case 'l':
      if (args->nlabels < MAX_LABEL_VALUES) 
        args->labels[args->nlabels++] = atoi(arg);
      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else if (state->arg_num == 1) args->output = arg;
      else argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 2) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;    
}

/**
 * Finds all nodes in the given graph which have one of the label values in
 * the given label list. Masks said nodes in the given mask array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_nodes_by_label(
  graph_t   *g,       /**< input graph                           */
  uint8_t    exclude, /**< exclude by label, instead of include  */
  uint32_t  *labels,  /**< labels to search for                  */
  uint8_t    nlabels, /**< number of labels                      */
  uint8_t   *mask     /**< enmpty mask array to populate         */
);

/**
 * Finds all nodes in the given graph which are in any of the given
 * components. Masks said nodes in the given mask array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_nodes_by_component(
  graph_t   *g,       /**< input graph                              */
  uint8_t    exclude, /**< exclude by component, instead of include */  
  uint32_t  *cmps,    /**< components to include                    */
  uint8_t    ncmps,   /**< number of components                     */
  uint8_t   *mask     /**< empty mask array to populate             */
);

/**
 * Tests whether the given node is in the list of included labels.
 *
 * \return 1 if the node label matches, 0 otherwise.
 */
static uint8_t _test_label(
  graph_t  *g,       /**< graph                      */
  uint8_t   exclude, /**< exclude instead of include */
  uint32_t  nid,     /**< node to test               */
  uint32_t *labels,  /**< labels to include          */
  uint8_t   nlabels  /**< number of labels           */
);

int main (int argc, char *argv[]) {


  graph_t     gin;
  graph_t     gout;
  dsr_t       hdr;
  uint8_t    *img;
  uint32_t    nginnodes;
  uint8_t    *mask;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};
  args_t      args;  

  memset(&args, 0, sizeof(args_t));
  img = NULL;

  startup("cextract", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  if (args.lblfile) {
    if (analyze_load(args.lblfile, &hdr, &img)) {
      printf("Could not load image file %s\n", args.lblfile);
      goto fail;
    }

    if (graph_relabel(&gin, &hdr, img, args.real)) {
      printf("Could not relabel graph\n");
      goto fail;
    }
  }

  nginnodes = graph_num_nodes(&gin);

  mask = calloc(nginnodes, sizeof(uint8_t));
  if (mask == NULL) {
    printf("memory error!?\n");
    goto fail;
  }

  if (!args.component) {
    if (_find_nodes_by_label(
          &gin, args.exclude, args.labels, args.nlabels, mask)) {
      printf("Could not find nodes by label\n");
      goto fail;
    }
  }
  else {
    if (_find_nodes_by_component(
          &gin, args.exclude, args.labels, args.nlabels, mask)) {
      printf("Could not find nodes by component\n");
      goto fail;
    }
  }

  if (graph_mask(&gin, &gout, mask)) {
    printf("could not mask graph\n");
    goto fail;
  }

  if (graph_log_copy(&gin, &gout)) {
    printf("Error copying graph log\n");
    goto fail;
  }

  if (args.hdrmsg != NULL) {
    if (graph_log_add(&gout, args.hdrmsg)) {
      printf("Error adding header message\n");
      goto fail;
    }
  }

  if (ngdb_write(&gout, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t _find_nodes_by_label(
  graph_t  *g,
  uint8_t   exclude,
  uint32_t *labels,
  uint8_t   nlabels,
  uint8_t  *mask) {

  uint64_t i;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    if (_test_label(g, exclude, i, labels, nlabels)) {
      mask[i] = 1;
    }
  }

  return 0;
}

uint8_t _test_label(
  graph_t  *g,
  uint8_t   exclude,
  uint32_t  nid,
  uint32_t *labels,
  uint8_t   nlabels) {

  uint16_t       i;
  graph_label_t *label;
  uint8_t        haslbl;

  haslbl = 0;

  label = graph_get_nodelabel(g, nid);
  if (label == NULL) return 0;

  for (i = 0; i < nlabels; i++) {

    if (label->labelval == labels[i]) {
      haslbl = 1;
      if (!exclude) return 1;
    }
  }

  if (exclude && !haslbl) return 1;
  return 0;
}

uint8_t _find_nodes_by_component(
  graph_t  *g,
  uint8_t   exclude,
  uint32_t *cmps,
  uint8_t   ncmps,
  uint8_t  *mask) {

  uint64_t  i;
  uint64_t  j;
  uint32_t *componentnums;
  uint32_t  nnodes;

  componentnums = NULL;
  nnodes        = graph_num_nodes(g);

  componentnums = calloc(nnodes, sizeof(uint32_t));
  if (componentnums == NULL) goto fail;

  stats_num_components(g, 0, NULL, componentnums);

  for (i = 0; i < nnodes; i++) {

    for (j = 0; j < ncmps; j++) {

      if (cmps[j] == componentnums[i]) {

        if (exclude) break;

        mask[i] = 1;
        break;
      }
    }

    if (exclude && j == ncmps) {
      mask[i] = 1;
    }
  }

  free(componentnums);
  return 0;
  
fail:
  if (componentnums != NULL) free(componentnums);
  return 1;
}
