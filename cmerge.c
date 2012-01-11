/**
 * Program to merge nodes in a graph into a single node, based on label value.
 *
 * This program may be used to merge multiple nodes into a single node, based
 * on their label value. A single 'merge set' is a list of input labels, and a
 * single output label; every node in the input graph which has one of the
 * input labels will be merged into a single node in the output graph, which
 * has the output label. The xyz coordinates of the output node is the mean of
 * the xyz coordinates of the input nodes
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"

/**
 * Maximum number of merge sets that can be specified on the command line.
 */
#define MAX_MERGESETS     10

/**
 * Maximum number of label values within one merge set.
 */
#define MAX_MERGESET_SIZE 50

/**
 * A merge set, specifying a mapping from 
 * a set of labels in the input graph to 
 * a single label in the output graph.
 */
typedef struct _merge_set {

  uint8_t  ninputs;                   /**< number of input labels          */
  uint32_t inputs[MAX_MERGESET_SIZE]; /**< input labels                    */
  uint32_t output;                    /**< output label                    */
  uint32_t nnodes;                    /**< number of input nodes - updated 
                                           by the _create_nodemap function */
} merge_set_t;

/**
 * Input arguments.
 */
typedef struct args {
  char        *input;                    /**< name of input file   */
  char        *output;                   /**< name of output file  */
  merge_set_t  mergesets[MAX_MERGESETS]; /**< merge sets           */
  uint8_t      nmergesets;               /**< number of merge sets */
} args_t;

static char doc[]   = "cmerge - merge nodes with specified labels";
static char usage[] = "usage: cmerge INPUT OUTPUT MERGESET [MERGESET ...]\n"\
                      "  where MERGESET is "\
                      "'outputlabel,inputlabel[,inputlabel...]";

/**
 * Parses a single merge set specified on the command line.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _parse_mergeset(char *arg, merge_set_t *ms) {

  char *copy;
  char *tkn;

  copy = NULL;

  /*strtok is destructive, so we use a copy*/
  copy = malloc(strlen(arg)+1);
  if (copy == NULL) goto fail;
  strcpy(copy, arg);

  /*read the output label value*/
  tkn = strtok(copy, ",");
  if (tkn == NULL) goto fail;

  ms->output = atoi(tkn);

  /*read the input label values*/
  ms->ninputs = 0;
  tkn = strtok(NULL, ",");
  while (tkn != NULL) {

    ms->inputs[ms->ninputs] = atoi(tkn);
    ms->ninputs ++;

    tkn = strtok(NULL, ",");
  }

  /*we need at least one input label*/
  if (ms->ninputs == 0) goto fail;
  
  free(copy);
  return 0;

fail:
  if (copy != NULL) free(copy);
  return 1;
}

/**
 * Parses command line arguments, and populates the given args_t 
 * struct. Exits if command line arguments are invalid.
 */
static void _parse_opt(int argc, char *argv[], args_t *args) {

  uint16_t i;

  if (argc >= 2 && !strcmp(argv[1], "-?")) {

    printf("%s\n", doc);
    printf("%s\n", usage);
    exit(0);
  }

  if (argc < 4) goto fail;

  args->input      = argv[1];
  args->output     = argv[2];
  args->nmergesets = argc - 3;

  for (i = 0; i < args->nmergesets; i++) {
    if (_parse_mergeset(argv[i+3], &(args->mergesets[i]))) {
      printf("malformed mergeset: %s\n", argv[i+3]);
      goto fail;
    }
  }

  return;

fail:
  printf("%s\n", usage);
  exit(1);
}

/**
 * Merges nodes in the input graph, according to the given list of merge
 * sets. The resulting graph is created, and stored via the gout pointer.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _merge(
  graph_t     *gin,       /**< input graph               */
  graph_t     *gout,      /**< place to put output graph */
  merge_set_t *mergesets, /**< merge sets                */
  uint8_t      nmergesets /**< number of merge sets      */
);

/**
 * Creates the node map.
 *
 * The node map is a mapping of node indices from the input graph to the
 * output graph. For every node in the input graph, a corresponding output
 * graph index is given - the value at nodemap[i] is the output graph index
 * for input graph node i. 
 *
 * Multiple input graph nodes which are part of a merge set will map to a
 * single index in the output graph; these 'merge set' nodes are given
 * indices 0 to (nmergesets-1) in the output graph; all remaining nodes are
 * given indices (nmergesets) to (nnodes in output graph - 1).
 *
 * The total number of nodes in the output graph is calculated as
 * (nnodes in input graph)-(nnodes in merge sets)+(nmergesets).
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _create_nodemap(
  graph_t     *gin,        /**< input graph                                */
  merge_set_t *mergesets,  /**< merge sets                                 */
  uint8_t      nmergesets, /**< number of merge sets                       */
  uint32_t    *nodemap,    /**< pre-allocated array to store node map      */
  uint32_t     ninnodes,   /**< size of node map (== graph_num_nodes(gin)) */
  uint32_t    *noutnodes   /**< Updated to store the number of 
                                nodes in the output graph                  */
);

/**
 * Tests the given node to determine whether it is part of a merge set.
 *
 * \return The mergeset index of the node, -1 if the node is not in a
 * mergeset, or 0x100 on failure.
 */
static int16_t _get_mergeset(
  graph_t     *g,          /**< the graph            */
  merge_set_t *mergesets,  /**< the merge sets       */
  uint8_t      nmergesets, /**< number of merge sets */
  uint32_t     nidx        /**< the node index       */
);

/**
 * Copies edges from the input graph to the output graph, according to the
 * given node map.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _copy_edges(
  graph_t  *gin,     /**< input graph                                */
  graph_t  *gout,    /**< output graph                               */
  uint32_t *nodemap, /**< node map                                   */
  uint32_t  nnodes   /**< size of node map (== graph_num_nodes(gin)) */
);

/**
 *
 */
static uint8_t _copy_nodelabels(
  graph_t     *gin,
  graph_t     *gout,
  uint32_t    *nodemap,
  uint32_t     nnodes,
  merge_set_t *mergesets,
  uint8_t      nmergesets
);

/**
 * Adds the xyz coordinates of the two labels, storing the result in label a.
 */
static void _add_labels(
  graph_label_t *a, /**< label a, the place to store the result */
  graph_label_t *b  /**< label b                                */
);

/**
 * Divids the xyz coordinates of the given label by the given value.
 */
static void _avg_label(
  graph_label_t *lbl,  /**< the label   */
  uint8_t        nlbls /**< the divisor */
);

int main (int argc, char *argv[]) {

  graph_t  gin;
  graph_t  gout;
  args_t   args;

  startup("cmerge", argc, argv, NULL, NULL);
  
  memset(&args, 0, sizeof(args_t));
  _parse_opt(argc, argv, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  printf("merging...\n");
  if (_merge(&gin, &gout, args.mergesets, args.nmergesets)) {
    printf("Could not perform merge\n");
    goto fail;
  }

  if (ngdb_write(&gout, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}

static uint8_t _merge(
  graph_t *gin, graph_t *gout, merge_set_t *mergesets, uint8_t nmergesets) {


  uint32_t *nodemap;
  uint32_t  ninnodes;
  uint32_t  noutnodes;

  nodemap = NULL;
  memset(gout, 0, sizeof(graph_t));

  ninnodes = graph_num_nodes(gin);

  nodemap = calloc(ninnodes, sizeof(uint32_t));
  if (nodemap == NULL) goto fail;

  if (_create_nodemap(
      gin, mergesets, nmergesets, nodemap, ninnodes, &noutnodes))
    goto fail;

  if (graph_create(gout, noutnodes, 0)) goto fail;

  if (_copy_edges(     gin, gout, nodemap, ninnodes)) goto fail;
  if (_copy_nodelabels(gin, gout, nodemap, ninnodes,
                       mergesets, nmergesets))        goto fail;

  free(nodemap);
  return 0;

fail:
  if (nodemap != NULL) free(nodemap);
  graph_free(gout);
  return 1;
}

static uint8_t _create_nodemap(
  graph_t     *gin,
  merge_set_t *mergesets,
  uint8_t      nmergesets,
  uint32_t    *nodemap,
  uint32_t     ninnodes,
  uint32_t    *noutnodes) {

  uint64_t i;
  uint32_t nidx;
  int16_t  msidx;

  /*
   * merge set nodes are given indices 0-(nmergesets-1) 
   * in the output graph; all remaining nodes are given 
   * indices (nmergesets)-(nnodes in output graph - 1)
   */
  msidx      = 0;
  nidx       = nmergesets;

  for (i = 0; i < ninnodes; i++) {

    msidx = _get_mergeset(gin, mergesets, nmergesets, i);
    switch(msidx) {

      case 0x100: goto fail;
      case -1:    nodemap[i] = nidx++; break;
      default: 

        nodemap[i] = msidx;
        mergesets[msidx].nnodes ++;
        break;
    }
  }

  *noutnodes = nidx;

  return 0;

fail:
  return 1;
}

static int16_t _get_mergeset(
  graph_t *g, merge_set_t *mergesets, uint8_t nmergesets, uint32_t nidx) {

  uint16_t       msidx;
  uint16_t       inidx;
  graph_label_t *lbl;

  lbl = graph_get_nodelabel(g, nidx);
  if (lbl == NULL) goto fail;

  for (msidx = 0; msidx < nmergesets; msidx++) {
    for (inidx = 0; inidx < mergesets[msidx].ninputs; inidx++) {

      if (mergesets[msidx].inputs[inidx] == lbl->labelval) return msidx;
    }
  }

  return -1;

fail:
  return 0x100;
}

static uint8_t _copy_edges(
  graph_t *gin, graph_t *gout, uint32_t *nodemap, uint32_t nnodes) {

  uint64_t  nidx;
  uint64_t  nbridx;
  uint64_t  nnbrs;
  uint32_t *nbrs;

  for (nidx = 0; nidx < nnodes; nidx++) {

    nnbrs = graph_num_neighbours(gin, nidx);
    nbrs  = graph_get_neighbours(gin, nidx);

    for (nbridx = 0; nbridx < nnbrs; nbridx++) {

      if (nodemap[nidx] == nodemap[nbrs[nbridx]]) continue;

      if (graph_add_edge(gout, nodemap[nidx], nodemap[nbrs[nbridx]], 1))
        goto fail;
    }
  }

  return 0;

fail:
  return 1;
}

static uint8_t _copy_nodelabels(
  graph_t     *gin, 
  graph_t     *gout, 
  uint32_t    *nodemap, 
  uint32_t     nnodes, 
  merge_set_t *mergesets,
  uint8_t      nmergesets) {

  uint64_t       nidx;
  graph_label_t *lbla;
  graph_label_t *lblb;

  /*
   * step through input nodes, adding labels for 
   * merge nodes, or copying for non-merge nodes
   */
  for (nidx = 0; nidx < nnodes; nidx++) {

    lbla = graph_get_nodelabel(gin, nidx);
    if (lbla == NULL) goto fail;

    if (nodemap[nidx] < nmergesets) {

      lblb = graph_get_nodelabel(gout, nodemap[nidx]);
      if (lblb == NULL) goto fail;
      _add_labels(lbla, lblb);
    }

    if (graph_set_nodelabel(gout, nodemap[nidx], lbla)) goto fail;
  }

  /*correct the labels for merged nodes (average + set label value)*/
  for (nidx = 0; nidx < nmergesets; nidx++) {

    lbla = graph_get_nodelabel(gout, nidx);
    if (lbla == NULL) goto fail;

    _avg_label(lbla, mergesets[nidx].nnodes);
    lbla->labelval = mergesets[nidx].output;
  }

  return 0;

fail:
  return 1;
}

static void _add_labels(graph_label_t *a, graph_label_t *b) {

  a->xval += b->xval;
  a->yval += b->yval;
  a->zval += b->zval;
}

static void _avg_label(graph_label_t *lbl, uint8_t nlbls) {

  lbl->xval /= nlbls;
  lbl->yval /= nlbls;
  lbl->zval /= nlbls;
}
