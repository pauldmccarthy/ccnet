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

typedef struct _args {
  char    *inputs[MAX_INPUTS];
  char    *output;
  uint16_t ninputs;
} args_t;

static char doc[] = "avgngdb -- create an average graph from "\
                    "a collection of input graph files";

static struct argp_option options[] = {
  {"input", 'i', "FILE", 0, "input file"},
  {0}
};

static error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  args_t *args;

  args = state->input;

  switch (key) {
    
    case 'i':
      if (args->ninputs < MAX_INPUTS)
        args->inputs[args->ninputs++] = arg;
      else
        printf("too many inputs - ignoring %s\n", arg);
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->output = arg;
      else                     argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN; 
  }
  
  return 0;
}

typedef struct __nlbl_map {

  array_t labels;  /**< array of graph_label_t structs   */
  array_t nodeids; /**< array of array_t structs, each
                        of which contains int64_t values */

} nlbl_map_t;

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
  const void *a,
  const void *b
);

static uint8_t _update_nlbl_map(
  graph_t    *g,
  uint16_t    ninputs,
  uint16_t    inidx,
  nlbl_map_t *map
);


static uint8_t _mk_nlbl_map(
  char      **inputs,
  uint16_t    ninputs,
  nlbl_map_t *map
);


void _print_map(nlbl_map_t *map, char **inputs, uint16_t ninputs) {

  uint64_t       i;
  uint64_t       j;
  int64_t        nidx;
  uint32_t       nnodes;
  graph_label_t *lbl;
  array_t       *nodeids;

  nnodes = map->labels.size;
  
  printf("                   \t");
  for (j = 0; j < ninputs; j++) {
    printf("%s\t", inputs[j]);
  }
  printf("\n");

  for (i = 0; i < nnodes; i++) {

    lbl     = array_getd(&(map->labels),  i);
    nodeids = array_getd(&(map->nodeids), i);

    printf("%0.2f %0.2f %0.2f: \t", lbl->xval, lbl->yval, lbl->zval);

    for (j = 0; j < ninputs; j++) {

      nidx = *(int64_t *)array_getd(nodeids, j);
      printf("%4" PRIi64 "\t", nidx);
    }

    printf("\n");
  }
}

int main(int argc, char *argv[]) {

  nlbl_map_t  map;
  args_t      args;
  struct argp argp = {options, _parse_opt, "OUTPUT", doc};

  memset(&args, 0, sizeof(args));
  memset(&map,  0, sizeof(map));

  startup("avgngdb", argc, argv, &argp, &args);

  if (_mk_nlbl_map(args.inputs, args.ninputs, &map)) {
    printf("error creating node label map\n");
    goto fail;
  }

  _print_map(&map, args.inputs, args.ninputs);


  return 0;
  
fail:
  return 1;
}


int _compare_glbl(const void *a, const void *b) {

  graph_label_t *ga;
  graph_label_t *gb;

  ga = (graph_label_t *)a;
  gb = (graph_label_t *)b;
  
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

  for (i = 0; i < nnodes; i++) {
    
    lbl = graph_get_nodelabel(g, i);

    printf("node %" PRIi64 " (%0.2f %0.2f %0.2f)\n",
           i, lbl->xval, lbl->yval, lbl->zval);

    switch (array_insert_sorted(&(map->labels), lbl, 1, &lblidx)) {

      case 0:
        printf("new label - inserting new array (%" PRIu32 ")\n", lblidx);
        if (array_create(&nodeids, sizeof(int64_t), ninputs)) goto fail; 
        for (j = 0; j < ninputs; j++) array_set(&nodeids, j, &tmp);
        
        if (array_insert(&(map->nodeids), lblidx, &nodeids))  goto fail;

        break;

      case 1:
        printf("existing label (%" PRIu32 "\n", lblidx);
        break;
      default: goto fail;
    }

    pnodeids = array_getd(&(map->nodeids), lblidx);

    printf("setting index %" PRIu16 ": %" PRIi64 "\n", inidx, i);
    array_set(pnodeids, inidx, &i);
  }

  return 0;
fail:
  return 1;
}

uint8_t _mk_nlbl_map(char **inputs, uint16_t ninputs, nlbl_map_t *map) {

  uint32_t i;
  graph_t  g;

  if (array_create(&(map->labels),  sizeof(graph_label_t), 1000)) goto fail;
  if (array_create(&(map->nodeids), sizeof(array_t),       1000)) goto fail;

  array_set_cmps(&(map->labels), _compare_glbl, _compare_glbl_ins);

  for (i = 0; i < ninputs; i++) {

    printf("loading %s ...\n", inputs[i]);

    if (ngdb_read(inputs[i], &g))              goto fail;
    if (_update_nlbl_map(&g, ninputs, i, map)) goto fail;
    graph_free(&g);
  }

  return 0;
  
fail:
  return 1;
}
