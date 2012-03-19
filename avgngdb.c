/**
 * 'Average' a collection of ngdb graph files.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <argp.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "util/compare.h"
#include "io/ngdb_graph.h"

#define MAX_INPUTS 50


typedef enum {

  SUM_WEIGHTS = 0,
  COUNT_EDGES,
  AVG_WEIGHTS,

} edge_weight_t;

typedef struct _args {
  char         *inputs[MAX_INPUTS];
  char         *output;
  uint16_t      ninputs;
  edge_weight_t edgeweight;
} args_t;

static struct argp_option options[] = {
  {"sumweights", 's', NULL, 0, "set output edge weights to the sum of "\
                               "corresponding input edge weights (default)"},
  {"countedges", 'c', NULL, 0, "set output edge weights to the number "\
                               "of corresponding input edges"},
  {"avgweights", 'a', NULL, 0, "set output edge weights to average "\
                               "of corresponding input edge weights"},
  {0}
};

static char doc[] = "avgngdb -- create an average graph from "\
                    "a collection of input graph files";

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {

    case 's': args->edgeweight = SUM_WEIGHTS; break; 
    case 'c': args->edgeweight = COUNT_EDGES; break;
    case 'a': args->edgeweight = AVG_WEIGHTS; break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->output = arg;
      else {
        
        if (args->ninputs < MAX_INPUTS) args->inputs[args->ninputs++] = arg;
        else printf("too many inputs - ignoring %s\n", arg);
      }
        
      break;

    case ARGP_KEY_END:
      if (state->arg_num <= 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN; 
  }
  
  return 0;
}

/**
 * This struct contains two sets of mappings:
 *   - A mapping from all unique label values in all input graphs, to the
 *     node indices for each input graph that correspond to said label values.
 *   - For each input graph, a mapping from the input graph node IDs to the
 *     output graph node IDs.
 *
 * This struct is created and populated by the _mk_nlbl_map function.
 */
typedef struct __nlbl_map {

  array_t labels;  /**< Array of graph_label_t structs, all
                        unique labels in all input graphs.
                        The size of this array will become
                        the number of nodes in the output graph.   */
  array_t nodeids; /**< Array of arrays, one for each unique
                        label (i.e. this array is the same
                        length as the labels array). Each
                        array contains [ninputs] int64_t
                        values, which are the node ID in
                        the respective input graph that
                        has the the unique label, or -1 if
                        there is no such node in the input graph.  */
  array_t sizes;   /**< Array of uint32_t values, one for
                        each input graph, the number of
                        nodes in that input graph.                 */
  array_t idmap;   /**< Array of array_t structs, one for
                        each input graph. Each array contains
                        a uint32_t value for every node in
                        the input graph, which is ID of the
                        corresponding node in the output graph.    */

} nlbl_map_t;

/**
 * Searches through the input graphs and builds a list of the node indices
 * that correspond to all unique label values. Then, using this information,
 * builds a node ID mapping  from the nodes in each input graph to these
 * unique label vales (i.e. the nodes in the output graph).
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mk_nlbl_map(
  char      **inputs,  /**< list of input graph files          */
  uint16_t    ninputs, /**< number of input files              */
  nlbl_map_t *map      /**< pointer to uninitialised label map */
);

/**
 * Updates the label map for the given input graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_nlbl_map(
  graph_t    *g,       /**< input graph                */
  uint16_t    ninputs, /**< total number of inputs     */
  uint16_t    inidx,   /**< input number of this graph */
  nlbl_map_t *map      /**< label map to update        */
);

/**
 * Builds the node ID mapping from the given input graph to the output graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mk_id_map(
  nlbl_map_t *map,  /**< label map to update */
  uint16_t    inidx /**< input graph index   */
);

/**
 * Compares two graph_label_t structs on their x,y and z values.
 *
 * \return >0 if (*a > *b), 0 if (*a == *b), <0 if (*a < *b).
 */
static int _compare_glbl(
  const void *a, /**< pointer to a graph_label_t struct */
  const void *b  /**< pointer to a graph_label_t struct */
);

/**
 * Compares two graph_label_t structs on their x,y and z values.
 *
 * \return 0 if (*a >= *b) and b is the last element in the array, or *a is
 * less than the next value in the array, <0 if *a < *b, >0 otherwise. 
 */
static int _compare_glbl_ins(
  const void *a, /**< pointer to a graph_label_t struct */
  const void *b  /**< pointer to a graph_label_t struct */
);

/**
 * Adds all of the edges from the given input graph to the output graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_avg_graph(
  graph_t      *gin,        /**< the input graph                     */
  graph_t      *gavg,       /**< the average (output) graph          */
  array_t      *nodemap,    /**< input -> output node ID mapping     */
  edge_weight_t edgeweight, /**< how to set output graph edge weight */
  uint16_t      ninputs     /**< number of input graphs              */
);

/**
 * Creates an average graph from all of the input graphs.
 * 
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _mk_avg_graph(
  char        **inputs,    /**< input graph files                   */
  uint16_t      ninputs,   /**< number of input files               */
  graph_t      *gavg,      /**< pointer to uninitialised graph      */
  nlbl_map_t   *map,       /**< label->node index mapping           */
  edge_weight_t edgeweight /**< how to set output graph edge weight */
);

int main(int argc, char *argv[]) {

  nlbl_map_t  map;
  args_t      args;
  graph_t     gavg;
  struct argp argp = {options, _parse_opt, "OUTPUT [INPUT ...]", doc};

  memset(&args, 0, sizeof(args));
  memset(&map,  0, sizeof(map));

  startup("avgngdb", argc, argv, &argp, &args);

  if (_mk_nlbl_map(args.inputs, args.ninputs, &map)) {
    printf("error creating node label map\n");
    goto fail;
  }

  if (_mk_avg_graph(args.inputs, args.ninputs, &gavg, &map, args.edgeweight)) {
    printf("error creating average graph\n");
    goto fail;
  }

  if (ngdb_write(&gavg, args.output)) {
    printf("error writing graph to %s\n", args.output);
    goto fail;
  }

  return 0;
  
fail:
  return 1;
}


uint8_t _mk_nlbl_map(char **inputs, uint16_t ninputs, nlbl_map_t *map) {

  uint32_t i;
  graph_t  g;

  if (array_create(&(map->labels),  sizeof(graph_label_t), 1000))    goto fail;
  if (array_create(&(map->nodeids), sizeof(array_t),       1000))    goto fail;
  if (array_create(&(map->sizes),   sizeof(uint32_t),      ninputs)) goto fail;
  if (array_create(&(map->idmap),   sizeof(array_t),       ninputs)) goto fail;

  array_set_cmps(&(map->labels), _compare_glbl, _compare_glbl_ins);

  for (i = 0; i < ninputs; i++) {

    if (ngdb_read(inputs[i], &g))              goto fail;
    if (_update_nlbl_map(&g, ninputs, i, map)) goto fail;
    graph_free(&g);
  }

  for (i = 0; i < ninputs; i++) {
    if (_mk_id_map(map, i)) goto fail;
  }

  return 0;
  
fail:
  return 1;
}

uint8_t _update_nlbl_map(
  graph_t *g, uint16_t ninputs, uint16_t inidx, nlbl_map_t *map) {

  graph_label_t *lbl;
  int64_t        i;
  int64_t        tmp;
  uint32_t       j;
  uint32_t       lblidx;
  uint32_t       nnodes;
  array_t        nodeids;
  array_t       *pnodeids;

  tmp = -1;

  nnodes = graph_num_nodes(g);

  array_set(&(map->sizes), inidx, &nnodes);

  for (i = 0; i < nnodes; i++) {
    
    lbl = graph_get_nodelabel(g, i);

    switch (array_insert_sorted(&(map->labels), lbl, 1, &lblidx)) {

      case 0:
        if (array_create(&nodeids, sizeof(int64_t), ninputs)) goto fail; 
        for (j = 0; j < ninputs; j++) array_set(&nodeids, j, &tmp);
        
        if (array_insert(&(map->nodeids), lblidx, &nodeids))  goto fail;

        break;

      case 1:
        break;
      default: goto fail;
    }

    pnodeids = array_getd(&(map->nodeids), lblidx);
    array_set(pnodeids, inidx, &i);
  }

  return 0;
fail:
  return 1;
}

uint8_t _mk_id_map(nlbl_map_t *map, uint16_t inidx) {

  uint64_t i;
  uint32_t navgnodes;
  uint32_t ninnodes;
  uint32_t avgnodeid;
  int64_t  innodeid;
  array_t *idmap;
  array_t *nodeids;

  idmap     =              array_getd(&(map->idmap), inidx);
  ninnodes  = *(uint32_t *)array_getd(&(map->sizes), inidx);
  navgnodes = map->labels.size;

  if (array_create(idmap, sizeof(uint32_t), ninnodes)) goto fail;

  for (i = 0; i < navgnodes; i++) {

    nodeids = array_getd(&(map->nodeids), i);
    
    innodeid = *(int64_t *)array_getd(nodeids, inidx);

    if (innodeid == -1) continue;

    avgnodeid = i;
    array_set(idmap, innodeid, &avgnodeid);
  }

  return 0;

fail:
  return 1;
}

int _compare_glbl(const void *a, const void *b) {

  graph_label_t *ga;
  graph_label_t *gb;
  
  ga = (graph_label_t *)a;
  gb = (graph_label_t *)b;

  if      (ga->labelval > gb->labelval) return  1;
  else if (ga->labelval < gb->labelval) return -1;

  if      (ga->zval > gb->zval) return  1;
  else if (ga->zval < gb->zval) return -1;
  
  if      (ga->yval > gb->yval) return  1;
  else if (ga->yval < gb->yval) return -1;
  
  if      (ga->xval > gb->xval) return  1;
  else if (ga->xval < gb->xval) return -1;
  
  return 0;
}

int _compare_glbl_ins(const void *a, const void *b) {
  
  return compare_insert(a, b, sizeof(graph_label_t), _compare_glbl);
}


uint8_t _update_avg_graph(
  graph_t      *gin,
  graph_t      *gavg,
  array_t      *nodemap,
  edge_weight_t edgeweight,
  uint16_t      ninputs) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  ginnodes;
  uint32_t *nbrs;
  uint32_t  nnbrs;
  uint32_t  outi;
  uint32_t  outj;
  float     inwt;
  float     outwt;

  ginnodes = graph_num_nodes(gin);

  for (i = 0; i < ginnodes; i++) {

    outi  = *(uint32_t *)array_getd(nodemap, i);
    nnbrs = graph_num_neighbours(gin, i);
    nbrs  = graph_get_neighbours(gin, i);

    for (j = 0; j < nnbrs; j++) {

      if (i >= nbrs[j]) continue;

      outj = *(uint32_t *)array_getd(nodemap, nbrs[j]);

      /*will have no effect if edge already exists*/
      graph_add_edge(gavg, outi, outj, 0.0);

      inwt  = graph_get_weight(gin,  i,    nbrs[j]);
      outwt = graph_get_weight(gavg, outi, outj);

      switch (edgeweight) {

        case SUM_WEIGHTS: outwt = outwt + inwt;           break;
        case COUNT_EDGES: outwt = outwt + 1;              break;
        case AVG_WEIGHTS: outwt = outwt + (inwt/ninputs); break;
        default:          goto fail;
      }

      if (graph_set_weight(gavg, outi, outj, outwt))
         goto fail;
    }
  }

  return 0;

fail:
  return 1;
}

uint8_t _mk_avg_graph(
  char        **inputs,
  uint16_t      ninputs,
  graph_t      *gavg,
  nlbl_map_t   *map,
  edge_weight_t edgeweight) {

  uint64_t       i;
  uint32_t       nnodes;
  graph_label_t *lbl;
  array_t       *nodemap;
  graph_t        gin;

  nnodes = map->labels.size;

  if (graph_create(gavg, nnodes, 0)) goto fail;

  for (i = 0; i < nnodes; i++) {
    
    lbl = array_getd(&(map->labels), i);
    if (graph_set_nodelabel(gavg, i, lbl)) goto fail;
  }

  for (i = 0; i < ninputs; i++) {

    nodemap = array_getd(&(map->idmap), i);
    
    if (ngdb_read(inputs[i], &gin)) goto fail;

    if (_update_avg_graph(&gin, gavg, nodemap, edgeweight, ninputs))
      goto fail;
    graph_free(&gin);
  }

  return 0;

fail:
  return 1;
}
