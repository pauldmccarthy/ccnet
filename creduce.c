/**
 * Program to reduce a labelled graph. 
 *
 * Reduces a labelled graph such that groups of same-labelled nodes
 * in the input graph become a single node in the output graph, and 
 * the number of edges between each pair of same-labelled groups in 
 * the input graph become edge weights in the output graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include "graph/graph.h"
#include "graph/graph_threshold.h"
#include "io/analyze75.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"

/**
 * A node and its label.
 */
typedef struct _node {

  uint32_t      nid; /**< node ID    */
  graph_label_t lbl; /**< node label */

} node_t;

/**
 * A partition, a collection of same-labelled nodes
 * in the input graph. Also corresponds to a single
 * node in the output graph.
 */
typedef struct _partition {

  uint32_t       nnodes; /**< number of nodes       */
  node_t        *nodes;  /**< list of nodes         */
  graph_label_t  plbl;   /**< 'averaged' node label */

} partition_t;

/**
 * Input arguments.
 */
typedef struct args {
  char    *input;     /**< name of input file                 */
  char    *output;    /**< name of outpout file               */
  char    *lblfile;   /**< optional ANALYZE75 label file      */
  float    threshold; /**< threshold to apply                 */
  uint8_t  pcount;    /**< print out label connectivity       */
  uint8_t  norm;      /**< normalise edge counts to densities */
  uint8_t  real;      /**< node coordinates are in real units */
} args_t;

/**
 * Parent reduce function. Takes an input graph, and generates
 * an output graph, by partitioning the nodes in the input graph
 * by label, and counting the number of edges between partitions.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _reduce(
  graph_t *gin,  /**< pointer to the input graph                       */
  graph_t *gout, /**< pointer to an empty graph struct, for the output */
  args_t  *args  /**< program arguments                                */
);

/**
 * Creates node_t structs for every node in the given graph. It
 * is the caller's responsibility to free the nodes pointer when 
 * it is no longer needed. 
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _get_nodes(
  graph_t  *g,    /**< the graph                                          */
  node_t  **nodes /**< will be updated to point to list of node_t structs */
);

/**
 * Counts the number of edges which exist between every pair 
 * of nodes in the given two partitions.
 *
 * \return the number of edges between the nodes in the given partitions.
 */
static uint32_t _count_edges(
  graph_t     *g,  /**< the graph            */
  partition_t *p1, /**< the first partition  */
  partition_t *p2  /**< the second partition */
);

/**
 * Groups the given list of nodes into partitions, according to their label
 * value. It is the caller's responsibility to free the partition list when it
 * is no longed needed. 
 *
 * Note: the node list is sorted in place, and partition entries, rather than
 * containing a copy of the node entries, simply point into different sections
 * of the sorted node list. Thus, you must not free the node list until you no
 * longer need the partition list.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _group_nodes(
  node_t       *nodes,  /**< list of nodes                                 */
  uint32_t      nnodes, /**< number of nodes                               */ 
  partition_t **ptns,   /**< pointer to partition list will be stored here */
  uint32_t     *nptns   /**< number of partitions will be stored here      */
);

/**
 * Sub-function of _group_nodes - creates a single partition from the 
 * same-labelled nodes at the start of the nodes list.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _next_group(
  node_t      *nodes,  /**< list of nodes                  */
  uint32_t     nnodes, /**< number of nodes                */
  partition_t *ptn     /**< pointer to partition to create */
);

/**
 * Compares two node_t structs; used for sorting a list of nodes via qsort.
 */
static int _compare_nodes(const void *vn1, const void *vn2);

static char doc[] = "creduce - reduce a labelled graph";

static struct argp_option options[] = {
  {"threshold", 't', "FLOAT", 0, "output an unweighted graph, using the "\
                                 "given threshold"},
  {"pcount"   , 'p', NULL,    0, "print connectivity between all pairs "\
                                 "of labels"},
  {"norm",      'n', NULL,    0, "save edge weights as normalised "\
                                 "densities, rather than absolute counts"},
  {"lblfile",   'l', "FILE",  0, "ANALYZE75 file containing node labels"},
  {"real",      'r', NULL,    0, "node coordinates are in real units"},
  {0}
};



static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch (key) {
    case 't': a->threshold = atof(arg); break;
    case 'p': a->pcount    = 0xFF;      break;
    case 'n': a->norm      = 0xFF;      break;
    case 'l': a->lblfile   = arg;       break;
    case 'r': a->real      = 0xFF;      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->input  = arg;
      else if (state->arg_num == 1) a->output = arg;
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

int main (int argc, char *argv[]) {

  graph_t  gin;
  graph_t  gwt;
  graph_t  gout;
  dsr_t    hdr;
  uint8_t *img;

  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT OUTPUT", doc};
  
  memset(&args, 0, sizeof(struct args));
  startup("creduce", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &gin)) {
    printf("Could not read in %s\n", args.input);
    goto fail;
  }

  if (args.lblfile) {

    if (analyze_load(args.lblfile, &hdr, &img)) {
      printf("error loading ANALYZE75 image: %s\n", args.lblfile);
      goto fail;
    }

    if (graph_relabel(&gin, &hdr, img, args.real)) {
      printf("error relabelling graph\n");
      goto fail;
    }
  }

  if (_reduce(&gin, &gwt, &args)) {
    printf("Graph reduce failed\n");
    goto fail;
  }

  graph_free(&gin);

  if (args.threshold > 0) {
    if (graph_threshold_weight(&gwt, &gout, args.threshold)) {
      printf("Graph threshold failed\n");
      goto fail;
    }
    graph_free(&gwt);
  }
  else {
    memcpy(&gout, &gwt, sizeof(graph_t));
  }

  if (ngdb_write(&gout, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t _reduce(graph_t *gin, graph_t *gout, args_t *args) {

  partition_t *ptns;
  node_t      *nodes;
  uint32_t     nnodes;
  uint32_t     nptns;
  float        wt;
  uint32_t     i;
  uint32_t     j;

  ptns  = NULL;
  nptns = 0;

  /*
   * 1. partition nodes into label groups
   * 2. create an edge between every partition in
   *    output graph
   * 3. count number of edges which exist between 
   *    every pair of partitions, in input graph
   * 4. (optional) normalise edge weights
   * 5. assign edge counts from #3-#4 as edge
   *    weights in output graph
   */

  nnodes = graph_num_nodes(gin);

  if (_get_nodes(  gin, &nodes))                  goto fail;
  if (_group_nodes(nodes, nnodes, &ptns, &nptns)) goto fail;
  if (graph_create(gout, nptns, 0))               goto fail;

  /*count edges between every pair of partitions*/
  for (i = 0; i < nptns; i++) {
    for (j = i+1; j < nptns; j++) {

      wt = _count_edges(gin, ptns+i, ptns+j);

      if (args->norm) 
        wt /= ((ptns[i].nnodes) * (ptns[j].nnodes)) / 2.0;

      if (args->pcount) {
        printf("  %u -> %u: %0.4f\n", 
          ptns[i].plbl.labelval,
          ptns[j].plbl.labelval,
          wt);
      }

      /*don't create an edge for partitions with no edges between them*/
      if (wt == 0) continue;

      if (graph_add_edge(gout, i, j, wt)) goto fail;
    }

    /*set the averaged label as the new node label*/
    if (graph_set_nodelabel(gout, i, &(ptns[i].plbl))) goto fail;
  }

  return 0;

fail:
  graph_free(gout);
  return 1;
}

uint8_t _get_nodes(graph_t *g, node_t **nodes) {

  uint32_t       i;
  uint32_t       nnodes;
  graph_label_t *lbl;

  *nodes  = NULL;

  nnodes = graph_num_nodes(g);

  *nodes = malloc(sizeof(node_t)*nnodes);
  if (*nodes == NULL) goto fail;

  for (i = 0; i < nnodes; i++) {

    (*nodes)[i].nid = i;

    lbl = graph_get_nodelabel(g, i);
    memcpy(&((*nodes)[i].lbl), lbl, sizeof(graph_label_t));
  }

  return 0;

fail:
  if (*nodes != NULL) free(*nodes);
  *nodes = NULL;
  return 1;
}

uint32_t _count_edges(graph_t *g, partition_t *p1, partition_t *p2) {

  uint32_t ct;
  uint32_t i;
  uint32_t j;

  ct = 0;

  for (i = 0; i < p1->nnodes; i++) {
    for (j = 0; j < p2->nnodes; j++) {

      if (graph_are_neighbours(g, p1->nodes[i].nid, p2->nodes[j].nid)) 
        ct++;
    }
  }
  return ct;
}

uint8_t _group_nodes(
  node_t *nodes, uint32_t nnodes, partition_t **ptns, uint32_t *nptns) {

  partition_t  next;
  partition_t *lptns;
  uint32_t     lnptns;

  *ptns  = NULL;
  lptns  = NULL;
  *nptns = 0;
  lnptns = 0;

  /*sort the nodes by label*/
  qsort(nodes, nnodes, sizeof(node_t), _compare_nodes);

  /*create partitions by stepping through the sorted node list*/
  while (nnodes > 0) {

    _next_group(nodes, nnodes, &next);

    if (next.nnodes > nnodes) goto fail;

    nnodes -= next.nnodes;
    nodes  += next.nnodes;

    lnptns++;
    lptns = realloc(lptns, lnptns*sizeof(partition_t));
    if (lptns == NULL) goto fail;

    memcpy(&(lptns[lnptns-1]), &next, sizeof(partition_t));
  }

  *ptns  = lptns;
  *nptns = lnptns;

  return 0;

fail:
  if (lptns != NULL) free(lptns);
  *ptns  = NULL;
  *nptns = 0;
  return 1;
}

uint8_t _next_group(node_t *nodes, uint32_t nnodes, partition_t *ptn) {

  uint32_t i;

  if (nnodes == 0) goto fail;

  ptn->plbl.labelval = nodes[0].lbl.labelval;
  ptn->plbl.xval     = 0;
  ptn->plbl.yval     = 0;
  ptn->plbl.zval     = 0;

  ptn->nnodes = 0;
  ptn->nodes  = nodes;

  for (i = 0; i < nnodes; i++) {

    if (nodes[i].lbl.labelval != ptn->plbl.labelval) break;

    ptn->nnodes++;

    ptn->plbl.xval += nodes[i].lbl.xval;
    ptn->plbl.yval += nodes[i].lbl.yval;
    ptn->plbl.zval += nodes[i].lbl.zval;
  }

  ptn->plbl.xval /= ptn->nnodes;
  ptn->plbl.yval /= ptn->nnodes;
  ptn->plbl.zval /= ptn->nnodes;

  return 0;

fail:
  return 1;
}

int _compare_nodes(const void *vn1, const void *vn2) {

  node_t *n1;
  node_t *n2;

  n1 = (node_t *)vn1;
  n2 = (node_t *)vn2;

  if (n1->lbl.labelval  > n2->lbl.labelval) return 1;
  if (n1->lbl.labelval  < n2->lbl.labelval) return -1;

  return 0;
}
