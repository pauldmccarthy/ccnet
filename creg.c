/**
 * Prints out inter/intra regional densities for a ngdb file, optionally
 * reading node labels from a corresponding ANALYZE75 image file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <argp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "graph/graph.h"
#include "graph/graph_log.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"
#include "util/startup.h"

static char doc[] = "cseg -- calculate and print inter/intra regional "\
                    "densities";

static struct argp_option opts[] = {
  {"lblfile", 'l', "FILE", 0, "ANALYZE75 file containing node labels"},
  {"real",    'r',  NULL,  0, "node coordinates are in real units"},
  {"nonorm",  'n',  NULL,  0, "show edge counts, rather "\
                              "than normalised densities"},
  {0}
};

typedef struct _args {

  char   *input;
  char   *lblfile;
  uint8_t real;
  uint8_t nonorm;

} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch (key) {

    case 'l': args->lblfile = arg; break;
    case 'r': args->real    = 1;   break;
    case 'n': args->nonorm  = 1;   break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) args->input  = arg;
      else                          argp_usage(state);
      break;
    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static uint8_t _mk_density_matrix(
  graph_t          *g,
  node_partition_t *ptn,
  double           *matrix
);

static void _print_density_matrix(
  graph_t          *g,
  node_partition_t *ptn,
  double           *matrix,
  uint8_t           nonorm
);

int main (int argc, char *argv[]) {

  graph_t          g;
  dsr_t            hdr;
  uint8_t         *img;
  args_t           args;
  node_partition_t ptn;
  double          *matrix;
  struct argp      argp = {opts, _parse_opt, "INPUT", doc};

  matrix = NULL;
  memset(&args, 0, sizeof(args));

  startup("cseg", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error loading graph file %s\n", args.input);
    goto fail;
  } 

  if (args.lblfile != NULL) {
    if (analyze_load(args.lblfile, &hdr, &img)) {
      printf("error loading label file %s\n", args.lblfile);
      goto fail;
    }

    if (graph_relabel(&g, &hdr, img, args.real)) {
      printf("error relabelling graph\n");
      goto fail;
    }
  }

  if (graph_group_by_label(&g, &ptn)) {
    printf("error partitioning graph\n");
    goto fail;
  }

  matrix = calloc(sizeof(double), ptn.nparts*ptn.nparts);
  if (matrix == NULL) {
    printf("out of memory?\n");
    goto fail;
  }

  if (_mk_density_matrix(&g, &ptn, matrix)) {
    printf("error creating density matrix\n");
    goto fail;
  }
  _print_density_matrix(&g, &ptn, matrix, args.nonorm);

  return 0;
  
fail:
  return 1;
}

uint8_t _mk_density_matrix(
  graph_t *g, node_partition_t *ptn, double *matrix) {

  uint64_t       i;
  uint64_t       j;
  int64_t        iptnid;
  int64_t        jptnid;
  uint32_t       nnodes;
  uint32_t       nparts;
  uint32_t       nnbrs;
  uint32_t      *nbrs;
  graph_label_t *lbl;

  nparts = ptn->nparts;
  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    nnbrs  = graph_num_neighbours(g, i);
    nbrs   = graph_get_neighbours(g, i);
    lbl    = graph_get_nodelabel( g, i);
    iptnid = array_find(ptn->partids, &(lbl->labelval), 1);
    if (iptnid < 0) goto fail;
    
    for (j = 0; j < nnbrs; j++) {

      if (i > nbrs[j]) continue;

      lbl = graph_get_nodelabel(g, nbrs[j]);

      jptnid = array_find(ptn->partids, &(lbl->labelval), 1);
      if (jptnid < 0) goto fail;

      if (iptnid == jptnid) {
        matrix[iptnid*nparts + jptnid] ++;
      }
      else {
        matrix[iptnid*nparts + jptnid] ++;
        matrix[jptnid*nparts + iptnid] ++;
      }
    }
  }

  return 0;
  
fail:
  return 1;
}

void _print_density_matrix(
  graph_t *g, node_partition_t *ptn, double *matrix, uint8_t nonorm) {

  uint64_t i;
  uint64_t j;
  uint32_t iid;
  uint32_t isz;
  uint32_t jsz;
  uint32_t nparts;
  double   val;

  nparts = ptn->nparts;

  printf("       ");
  for (i = 0; i < nparts; i++) {
    iid = *(uint32_t *)array_getd(ptn->partids, i);
    printf("%6u ", iid);
  }
  printf("\n\n");

  for (i = 0; i < nparts; i++) {
    
    iid = *(uint32_t *)array_getd(ptn->partids, i);
    isz  = ((array_t *)array_getd(ptn->parts,   i))->size;
    
    printf("%6u ", iid);
    
    for (j = 0; j < nparts; j++) {

      val = matrix[i * nparts + j];

      jsz = ((array_t *)array_getd(ptn->parts, j))->size;

      if (nonorm) printf("%6u ", (uint32_t)val);

      else {
        if (i == j) val /= (isz*(isz-1)/2.0);
        else        val /= (isz*jsz/2.0);

        printf("%6.4f ", val);
      }
    }
    printf("\n");
  }
}
