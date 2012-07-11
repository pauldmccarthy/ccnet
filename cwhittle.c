/**
 * Removes as many edges from a graph as possible, such that it remains
 * connected.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "graph/bfs.h"
#include "io/ngdb_graph.h"
#include "util/startup.h"
#include "util/array.h"


static char doc[] = "cwhittle - remove as many edges from a graph "\
                    "as possible, such that the graph remains connected";


typedef struct args {
  char    *input;
  char    *output;

  uint8_t  abs;
} args_t;


static struct argp_option options[] = {
  {"abs", 'a', NULL, 0, "use absolute value of edge weight"},
  {0}
};


static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {

    case 'a': a->abs = 1; break;
      
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


/**
 * Compares two graph_edge_t structs. The edge with the higher weight is
 * considered to be the larger.
 *
 * \return 1, 0, or -1.
 */
static int _compare_edges(
  const void *a, /**< pointer to a graph_edge_t struct       */
  const void *b  /**< pointer to another graph_edge_t struct */
);


/**
 * Sorts all of the edges in the given graph; the given edges pointer is
 * allocated to store a graph_edge_t struct for every edge in the graph; the
 * sorted graph_edge_t structs are then stored there.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _sort_edges(
  graph_t       *g,
  graph_edge_t **edges
);


/**
 * Context structure used in breadth first search.
 */
typedef struct _bfs_ctx {
  uint64_t nvisited; /**< number of nodes that have
                          been visited in the search */
} bfs_ctx_t;


/**
 * \return positive if the graph is connected (i.e. consists of a single
 * component), zero otherwise, negative on error.
 */
static int8_t _is_connected(
  graph_t *g, /**< the graph      */
  uint32_t u, /**< starting point */
  uint32_t v  /**< starting point */
);


/**
 * Callback function used by the _is_connected function to run a
 * breadth first search.
 *
 * \return 0 always.
 */
static uint8_t _bfs_cb(
  bfs_state_t *state, /**< search state                  */
  void        *ctx    /**< pointer to a bfs_ctx_t struct */
);


int main(int argc, char *argv[]) {

  uint64_t      i;
  int8_t        is_con;
  uint32_t      nedges;
  graph_t       g;
  graph_edge_t *curedge;
  graph_edge_t *edges;
  args_t        args;
  struct argp   argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));
  startup("cwhittle", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g)) {
    printf("error loading graph %s\n", args.input);
    goto fail;
  }

  nedges = graph_num_edges(&g);
  _sort_edges(&g, &edges);

  for (i = 0; i < nedges; i++) {
    
    curedge = edges+i;

    /* remove the current edge (the edge with the lowest weight) */
    printf("removing edge %5lu (%5u -- %5u: %0.3f) ...\n",
           i, curedge->u, curedge->v, curedge->val);
    if (graph_remove_edge(&g, curedge->u, curedge->v)) {
      printf("error removing edge %5u -- %5u\n", curedge->u, curedge->v);
      goto fail;
    }

    /* test the graph to see if it has become disconnected */
    is_con = _is_connected(&g, curedge->u, curedge->v);

    /* something went wrong */
    if (is_con < 0) {
      printf("error testing connectivity\n");
      goto fail;
    }

    /*
     * Removing this edge caused the graph to become disconnected; put
     * it back, and stop. Otherwise, continue on to the next edge.
     */
    if (is_con == 0) {

      printf("graph disconnected at edge %5lu (%5u -- %5u: %0.3f)\n",
             i, curedge->u, curedge->v, curedge->val);

      if (graph_add_edge(&g, curedge->u, curedge->v, curedge->val)) {
        printf("error re-inserting edge %5u -- %5u: %0.3f\n",
               curedge->u, curedge->v, curedge->val);
        goto fail;
      }

      break;
    }
  }

  if (ngdb_write(&g, args.output)) {
    printf("error writing graph to %s\n", args.output);
    goto fail;
  } 
  
  return 0;

fail:
  return 1;
}


int _compare_edges(const void *a, const void *b) {

  graph_edge_t *ea;
  graph_edge_t *eb;

  ea = (graph_edge_t *)a;
  eb = (graph_edge_t *)b;

  if (ea->val > eb->val) return 1;
  if (ea->val < eb->val) return -1;

  return 0;
}


uint8_t _sort_edges(graph_t *g, graph_edge_t **edges) {

  uint64_t      i;
  uint64_t      j;
  uint32_t      nnbrs;
  uint32_t     *nbrs;
  float        *wts;
  graph_edge_t *ledges;
  uint32_t      iedge;
  uint32_t      nedges;
  uint32_t      nnodes;

  ledges = NULL;
  *edges = NULL;
  nnodes = graph_num_nodes(g);
  nedges = graph_num_edges(g);

  ledges = calloc(nedges, sizeof(graph_edge_t));
  if (ledges == NULL) goto fail;

  iedge = 0;
  for (i = 0; i < nnodes; i++) {

    nbrs  = graph_get_neighbours(g, i);
    nnbrs = graph_num_neighbours(g, i);
    wts   = graph_get_weights   (g, i);

    for (j = 0; j < nnbrs; j++) {

      if (i >= nbrs[j]) continue;
      
      ledges[iedge].u   = i;
      ledges[iedge].v   = nbrs[j];
      ledges[iedge].val = wts[j];

      iedge ++;
    }
  }

  qsort(ledges, nedges, sizeof(graph_edge_t), &_compare_edges);

  *edges = ledges;

  return 0;

fail:

  if (ledges != NULL) free(ledges);
  *edges = NULL;
  return 1;
}


int8_t _is_connected(graph_t *g, uint32_t u, uint32_t v) {

  uint32_t  nnodes;
  uint32_t  unnbrs;
  uint32_t  vnnbrs;
  uint32_t  root;
  bfs_ctx_t ctx;
  
  memset(&ctx, 0, sizeof(bfs_ctx_t));

  nnodes = graph_num_nodes(g);
  unnbrs = graph_num_neighbours(g, u);
  vnnbrs = graph_num_neighbours(g, v);

  if (unnbrs >= vnnbrs) root = u;
  else                  root = v;

  if (bfs(g, &root, 1, NULL, &ctx, NULL, &_bfs_cb, NULL)) goto fail;

  /* -1 because the root node is not visited during the search */
  if (ctx.nvisited == (nnodes - 1)) return 1;

  return 0;

fail:
  return -1;
}


uint8_t _bfs_cb(bfs_state_t *state, void *ctx) {

  bfs_ctx_t *bctx;
  
  bctx            = ctx;
  bctx->nvisited += state->thislevel.size;

  return 0;
}
