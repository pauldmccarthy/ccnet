/**
 * Prints out graph measures averaged by region, and inter/intra regional
 * densities for a ngdb file, optionally reading node labels from a
 * corresponding ANALYZE75 image file.
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
#include "stats/stats.h"
#include "stats/stats_cache.h"

static char doc[] = "creg -- calculate and print inter/intra regional "\
                    "densities";

static struct argp_option opts[] = {
  {"lblfile", 'l', "FILE", 0, "ANALYZE75 file containing node labels"},
  {"lblmap",  'a', "FILE", 0, "text file containing label mappings"},
  {"real",    'r',  NULL,  0, "node coordinates are in real units"},
  {"region",  'e',  NULL,  0, "Print out regional density matrix"},
  {"sizes",   's',  NULL,  0, "print out number of nodes in each region"},
  {"means",   'm',  NULL,  0, "print out node measures for each region"},
  {"nonorm",  'n',  NULL,  0, "show edge counts, rather "\
                              "than normalised densities"},
  {0}
};

typedef struct _args {

  char   *input;
  char   *lblfile;
  char   *lblmap;
  uint8_t real;
  uint8_t region;
  uint8_t sizes;
  uint8_t means;
  uint8_t nonorm;

} args_t;

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *args = state->input;

  switch (key) {

    case 'l': args->lblfile = arg; break;
    case 'a': args->lblmap  = arg; break;
    case 'r': args->real    = 1;   break;
    case 'e': args->region  = 1;   break;
    case 's': args->sizes   = 1;   break;
    case 'm': args->means   = 1;   break;
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

static void _print_region_sizes(
  node_partition_t *ptn
);

static uint8_t _print_region_means(
  graph_t          *g,
  node_partition_t *ptn
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

  startup("creg", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error loading graph file %s\n", args.input);
    goto fail;
  }

  if (stats_cache_init(&g)) {
    printf("error initialising stats cache\n");
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

  if (args.lblmap) {
    if (graph_relabel_map(&g, args.lblmap)) {
      printf("error relabelling graph with mapping file %s\n", args.lblmap);
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

  if (args.region) {  
    if (_mk_density_matrix(&g, &ptn, matrix)) {
      printf("error creating density matrix\n");
      goto fail;
    }

    _print_density_matrix(&g, &ptn, matrix, args.nonorm);
  }

  if (args.sizes) {
    printf("\n");
    _print_region_sizes(&ptn);
  }

  if (args.means) {
    
    _print_region_means(&g, &ptn);
  } 

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
  uint32_t normfac;
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

        if (i == j) normfac = (isz*(isz-1)/2.0);
        else        normfac = isz*jsz;

        if (normfac > 1) val /= normfac;

        printf("%12.10f ", val);
      }
    }
    printf("\n");
  }
}

void _print_region_sizes(node_partition_t *ptn) {
  
  uint64_t i;
  uint32_t id;
  array_t  part;

  for (i = 0; i < ptn->nparts; i++) {

    array_get(ptn->partids, i, &id);
    array_get(ptn->parts,   i, &part);
    printf("%u %u\n", id,  part.size);
  }
}


uint8_t _print_region_means(graph_t *g, node_partition_t *ptn) {

  uint64_t i;
  uint64_t j;
  uint32_t id;
  uint32_t nnodes;
  uint32_t node;
  array_t  part;
  double   avgdegree;
  uint8_t *mask;
  double   regeff;

  nnodes = graph_num_nodes(g);
  mask   = calloc(nnodes, sizeof(uint8_t));
  if (mask == NULL) goto fail;
  
  printf("region, size, degree, efficiency\n");

  for (i = 0; i < ptn->nparts; i++) {

    array_get(ptn->partids, i, &id);
    array_get(ptn->parts,   i, &part);

    /* create node mask for regional efficiency calculation */
    memset(mask, 1, nnodes*sizeof(uint8_t));
    for (j = 0; j < part.size; j++) {

      array_get(&part, j, &node);
      mask[node] = 0;
    }

    regeff    = stats_sub_efficiency(g, part.size, mask);
    avgdegree = 0;

    for (j = 0; j < part.size; j++) {

      array_get(&part, j, &node);

      avgdegree += graph_num_neighbours(g, node);
    }

    avgdegree /= part.size;

    printf("%3u, %4u, %8.4f, %0.6f\n",
           id, part.size, avgdegree, regeff);
  }

  free(mask);
  return 0;
  
fail:
  if (mask != NULL) free(mask);
  return 1;
}
