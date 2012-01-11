/**
 * Program to generate graphs of different types.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>

#include "util/startup.h"
#include "graph/graph.h"
#include "io/ngdb_graph.h"

typedef enum {
  TYPE_ER_RANDOM = 0, /* "errandom"  */
  TYPE_CLUSTERED = 1, /* "clustered" */
} graph_type_t;

static char doc[] = "cgen - generate graphs";

typedef struct args {

  char *output;

  uint32_t     numnodes;
  graph_type_t type;
  double       density;
  uint32_t     numclusters;
  double       internal;
  double       external;
  double       sizerange;
  uint8_t      iedegree;
  uint8_t      intext;
  uint8_t      intdens;
  
} args_t;

static struct argp_option options[] = {
  {"numnodes",    'n', "INT",    0, "number of nodes"},
  {"type",        't', "STRING", 0, "graph type"},
  {"density",     'd', "DOUBLE", 0, "overall graph density"},
  {"numclusters", 'c', "INT",    0, "number of clusters, for "\
                                    "clustered graphs"},
  {"internal",    'i', "DOUBLE", 0, "internal density/degree for "\
                                    "clustered graphs"},
  {"external",    'e', "DOUBLE", 0, "external density/degree for "\
                                    "clustered graphs"},
  {"sizerange",   'r', "DOUBLE", 0, "cluster size variation, for "\
                                    "clustered graphs"},
  {"iedegree",    'g',  NULL,    0, "use internal/external degree "\
                                     "for clustered graphs"},
  {"intext"  ,    'l',  NULL,    0, "use internal/external density "\
                                    "for clustered graphs"},
  {"intdens",     's',  NULL,    0, "use internal and total density "\
                                    "for clustered graphs"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {

    case 'n': a->numnodes = atoi(arg); break;
    case 't':
      if      (!strcmp(arg, "errandom"))  a->type = TYPE_ER_RANDOM;
      else if (!strcmp(arg, "clustered")) a->type = TYPE_CLUSTERED;
      else argp_usage(state);
      break;

    case 'd': a->density     = atof(arg); break;
    case 'c': a->numclusters = atoi(arg); break;
    case 'i': a->internal    = atof(arg); break;
    case 'e': a->external    = atof(arg); break;
    case 'r': a->sizerange   = atof(arg); break;
    case 'g': a->iedegree    = 0xFF;      break;
    case 'l': a->intext      = 0xFF;      break;
    case 's': a->intdens     = 0xFF;      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->output = arg;
      else                          argp_usage(state);
      break;
      
    case ARGP_KEY_END:  
      if (state->arg_num < 1) argp_usage(state);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  
  graph_t        g;
  args_t         args;
  struct argp    argp = {options, _parse_opt, "OUTPUT", doc};

  memset(&args, 0, sizeof(args_t));
  startup("cgen", argc, argv, &argp, &args);

  switch(args.type) {

    case TYPE_ER_RANDOM:
      if (graph_create_er_random(&g, args.numnodes, args.density)) {
        printf("could not create random graph\n");
        goto fail;
      }
      break;

    case TYPE_CLUSTERED:
      if (args.iedegree || !(args.iedegree || args.intext || args.intdens)) {
        if (graph_create_clustered_by_degree(
              &g,
              args.numnodes,
              args.numclusters,
              args.internal,
              args.external,
              args.sizerange)) {
          printf("could not create clustered graph\n");
          goto fail;
        }     
      }
      else if (args.intext) {
        if (graph_create_clustered(
              &g,
              args.numnodes,
              args.numclusters,
              args.internal,
              args.external,
              args.sizerange)) {
          printf("could not create clustered graph\n");
          goto fail;
        }
      }
      else {
        if (graph_create_clustered_by_total(
              &g,
              args.numnodes,
              args.numclusters,
              args.internal,
              args.density,
              args.sizerange)) {
          printf("could not create clustered graph\n");
          goto fail;
        }
      }
      break;
    default:
      printf("unknown graph type");
      goto fail;
  }

  if (ngdb_write(&g, args.output)) {
    printf("Could not write to %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}