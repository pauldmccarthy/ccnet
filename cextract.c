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

#include "stats/stats.h"
#include "graph/graph.h"
#include "util/startup.h"
#include "util/array.h"
#include "io/ngdb_graph.h"

/**
 * Maximum number of label values/components
 * that can be passed in on the command line.
 */
#define MAX_LABEL_VALUES 50

/**
 * Input arguments.
 */
typedef struct args {
  char    *input;                    /**< name of input file               */
  char    *output;                   /**< name of output file              */
  uint8_t  labelext;                 /**< non-0: extract by labels (default),
                                          otherwise extract by component   */
  uint32_t labels[MAX_LABEL_VALUES]; /**< labels (or components) to
                                          include in subgraph              */
  uint8_t  nlabels;                  /**< number of labels (or components) */
} args_t;

static char doc[]   = "cextract - extract a subgraph by "\
                      "label value or component";
static char usage[] = "usage: cextract INPUT OUTPUT " \
                      "[cmp|lbl] LABEL [LABEL ...]";

/**
 * Struct which provides an index mapping for a node, 
 * between the input graph and the output graph
 */
typedef struct _node {

  uint32_t gin_idx;  /**< node index in the input graph  */
  uint32_t gout_idx; /**< node index in the output graph */
} node_t;

/**
 * Creates a subgraph of the given input graph, by extracting nodes which
 * have any of the given labels.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _extract_by_label(
  graph_t  *gin,    /**< input graph            */
  graph_t  *gout,   /**< empty graph for output */
  uint32_t *labels, /**< labels to include      */
  uint8_t   nlabels /**< number of labels       */
);

/**
 * Creates a subgraph of the given input graph, by extracting nodes which
 * are in any of the given components.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _extract_by_component(
  graph_t  *gin,  /**< input graph            */
  graph_t  *gout, /**< empty graph for output */
  uint32_t *cmps, /**< components to include  */
  uint8_t   ncmps /**< number of components   */
);

/**
 * Finds all nodes in the given graph which have one of the label values in
 * the given label list. Adds node mappings to the given array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_nodes_by_label(
  graph_t   *g,       /**< input graph                           */
  uint32_t  *labels,  /**< labels to search for                  */
  uint8_t    nlabels, /**< number of labels                      */
  array_t   *nodes    /**< will be populated with node_t structs */
);

/**
 * Finds all nodes in the given graph which are in any of the given
 * components. Adds node mappings to the given array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _find_nodes_by_component(
  graph_t   *g,     /**< input graph                           */
  uint32_t  *cmps,  /**< components to include                 */
  uint8_t    ncmps, /**< number of components                  */
  array_t   *nodes  /**< will be populated with node_t structs */
);

/**
 * Tests whether the given node is in the list of included labels.
 *
 * \return 1 if the node label matches, 0 otherwise.
 */
static uint8_t _test_label(
  graph_t  *g,      /**< graph             */
  uint32_t  nid,    /**< node to test      */
  uint32_t *labels, /**< labels to include */
  uint8_t   nlabels /**< number of labels  */
);

/**
 * Copies node labels from the input graph to the output graph, for the
 * given list of nodes.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _copy_nodelabels(
  graph_t *gin,   /**< input graph             */
  graph_t *gout,  /**< output graph            */
  array_t *nodes  /**< array of node_t structs */
);

/**
 * Copies edges from the input graph to the output graph, for the given
 * list of nodes.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _copy_edges(
  graph_t *gin,  /**< input graph         */
  graph_t *gout, /**< output graph        */
  array_t *nodes /**< node index mappings */
);

/**
 * Parses command line arguments, and populates the given args_t 
 * struct. Exits if command line arguments are invalid.
 */
static void _parse_opt(int argc, char *argv[], args_t *args) {

  uint16_t i;
  uint32_t off;

  if (argc >= 2 && !strcmp(argv[1], "-?")) {

    printf("%s\n", doc);
    printf("%s\n", usage);
    exit(0);
  }

  if (argc < 4) goto fail;

  args->input   = argv[1];
  args->output  = argv[2];

  args->labelext = 1;
  off            = 4;

  if      (!strcmp(argv[3], "cmp")) args->labelext = 0;
  else if (!strcmp(argv[3], "lbl")) args->labelext = 1;
  else                             off            = 3;

  args->nlabels  = argc - off;

  for (i = off; i < argc; i++) 
    args->labels[i-off] = atoi(argv[i]);

  return;

fail:
  printf("%s\n", usage);
  exit(1);
}

int main (int argc, char *argv[]) {

  graph_t  gin;
  graph_t  gout;
  args_t   args;

  memset(&args, 0, sizeof(args_t));

  startup("cextract", argc, argv, NULL, NULL);
  _parse_opt(argc, argv, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  if (args.labelext) {
    if (_extract_by_label(&gin, &gout, args.labels, args.nlabels)) {
      printf("Could not extract subgraph by labels\n");
      goto fail;
    }
  }
  else {
    if (_extract_by_component(&gin, &gout, args.labels, args.nlabels)) {
      printf("Could not extract subgraph by components\n");
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

static uint8_t _extract_by_label(
  graph_t *gin, graph_t *gout, uint32_t *labels, uint8_t nlabels) {

  array_t nodes;
  nodes.data = NULL;

  if (array_create(&nodes, sizeof(node_t), 100))          goto fail;
  
  if (_find_nodes_by_label(gin, labels, nlabels, &nodes)) goto fail;
  if (graph_create(        gout, nodes.size, 0))          goto fail;
  if (_copy_nodelabels(    gin, gout, &nodes))            goto fail;
  if (_copy_edges(         gin, gout, &nodes))            goto fail;

  array_free(&nodes);

  return 0;

fail:

  if (nodes.data != NULL) array_free(&nodes);
  return 1;
}

static uint8_t _find_nodes_by_label(graph_t *gin,
  uint32_t *labels, uint8_t nlabels, array_t *nodes) {

  uint64_t i;
  uint64_t j;
  uint32_t nnodes;
  node_t   node;

  nnodes = graph_num_nodes(gin);

  j = 0;
  for (i = 0; i < nnodes; i++) {
    if (_test_label(gin, i, labels, nlabels)) {

      node.gin_idx  = i;
      node.gout_idx = j;
      j++;
      
      if (array_append(nodes, &node)) goto fail;
    }
  }

  return 0;

fail:
  return 1;
}

uint8_t _test_label(
  graph_t *g, uint32_t nid, uint32_t *labels, uint8_t nlabels) {

  uint16_t       i;
  graph_label_t *label;

  label = graph_get_nodelabel(g, nid);
  if (label == NULL) return 0;

  for (i = 0; i < nlabels; i++) {

    if (label->labelval == labels[i]) return 1;
  }

  return 0;
}

uint8_t _extract_by_component(
  graph_t *gin, graph_t *gout, uint32_t *cmps, uint8_t ncmps) {

  array_t nodes;
  nodes.data = NULL;

  if (array_create(&nodes, sizeof(node_t), 100))          goto fail;
  
  if (_find_nodes_by_component(gin, cmps, ncmps, &nodes)) goto fail;
  if (graph_create(            gout, nodes.size, 0))      goto fail;
  if (_copy_nodelabels(        gin, gout, &nodes))        goto fail;
  if (_copy_edges(             gin, gout, &nodes))        goto fail;

  array_free(&nodes);

  return 0;

fail:

  if (nodes.data != NULL) array_free(&nodes);
  return 1;

  return 0;
}

uint8_t _find_nodes_by_component(
  graph_t *g, uint32_t *cmps, uint8_t ncmps, array_t *nodes) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnewnodes;
  uint32_t *componentnums;
  uint32_t  nnodes;
  node_t    node;

  componentnums = NULL;
  nnewnodes     = 0;
  nnodes        = graph_num_nodes(g);

  componentnums = calloc(nnodes, sizeof(uint32_t));
  if (componentnums == NULL) goto fail;

  stats_num_components(g, 0, NULL, componentnums);

  for (i = 0; i < nnodes; i++) {

    for (j = 0; j < ncmps; j++) {

      if (cmps[j] == componentnums[i]) {

        node.gin_idx  = i;
        node.gout_idx = nnewnodes++;

        if (array_append(nodes, &node)) goto fail;

        break;
      }
    }
  }

  free(componentnums);
  return 0;
  
fail:
  if (componentnums != NULL) free(componentnums);
  return 1;
}

static uint8_t _copy_edges(
  graph_t *gin, graph_t *gout, array_t *nodes) {

  uint64_t  i;
  uint64_t  j;
  node_t   *nodei;
  node_t   *nodej;

  for (i = 0; i < nodes->size; i++) {
    for (j = i+1; j < nodes->size; j++) {

      nodei = array_getd(nodes, i);
      nodej = array_getd(nodes, j);

      if (graph_are_neighbours(gin, nodei->gin_idx, nodej->gin_idx)) {
        if (graph_add_edge(gout, nodei->gout_idx, nodej->gout_idx, 1))
          goto fail;
      }
    }
  }

  return 0;

fail:
  return 1;
}

static uint8_t _copy_nodelabels(
  graph_t *gin, graph_t *gout, array_t *nodes) {

  uint64_t       i;
  graph_label_t *label;
  node_t        *node;

  for (i = 0; i < nodes->size; i++) {

    node = array_getd(nodes, i);

    label = graph_get_nodelabel(gin,  node->gin_idx);
    if (label == NULL) goto fail;
    
    if (graph_set_nodelabel(gout, node->gout_idx, label)) goto fail;
  }

  return 0;

fail:
  return 1;
}
