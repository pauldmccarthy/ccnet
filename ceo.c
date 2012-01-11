/**
 * ceo - convert a Radatools lol partition file, or an Infomap .tree file,
 * into an equivalent ngdb file.  All communities in the ngdb file are fully
 * connected.
 *
 * See:
 *   - http://deim.urv.cat/~sgomez/radatools.php
 *   - http://www.tp.umu.se/~rosvall/code.html
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "util/startup.h"
#include "graph/graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "io/ngdb_graph.h"
#include "io/lol.h"
#include "io/infomap.h"


static char doc[] =
  "ceo - convert a radatools lol/infomap tree to an ngdb file";


/**
 * File type identifiers
 */
typedef enum {
  
  FILE_RADATOOLS = 0, /**< radatools .lol file */
  FILE_INFOMAP   = 1  /**< infomap .tree file  */
}  file_type_t;

typedef struct args {
  
  char       *input;
  char       *output;
  char       *connfile;
  file_type_t type;

} args_t;

static struct argp_option options[] = {
  {"type",     't', "STRING",   0, "file type (either 'lol' or 'tree')"},
  {"connfile", 'c', "NGDBFILE", 0, "ngdb graph file from which connectivity "\
                                   "and labels can be extracted"},
  {0}
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  args_t *a = state->input;

  switch(key) {

    case 't':
      if      (!strcmp(arg, "lol"))  a->type = FILE_RADATOOLS;
      else if (!strcmp(arg, "tree")) a->type = FILE_INFOMAP;
      else                           argp_usage(state);
      break;

    case 'c':
      a->connfile = arg;
      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->input  = arg;
      else if (state->arg_num == 1) a->output = arg;
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


static uint8_t _partition_to_graph(
  node_partition_t *part,
  graph_t          *g,
  graph_t          *conn
);


int main (int argc, char *argv[]) {

  node_partition_t part;
  graph_t          g;
  graph_t          conn;
  graph_t         *pconn;
  args_t           args;
  struct argp      argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  pconn = NULL;
  memset(&args, 0, sizeof(args_t));

  startup("ceo", argc, argv, &argp, &args);

  if (args.connfile) {
    if (ngdb_read(args.connfile, &conn)) {
      printf("error loading connectivity/label source file %s\n",
             args.connfile);
      goto fail;
    }
    pconn = &conn;
  }

  if      (args.type == FILE_RADATOOLS) {
    if (lol_load(args.input, &part)) {
      printf("error loading lolcatz ... i mean, lolfile %s\n", args.input);
      goto fail;
    }
  }
  else if (args.type == FILE_INFOMAP) {
    if (infomap_load(args.input, &part)) {
      printf("error loading infomap file %s\n", args.input);
      goto fail;
    }
  }

  if (_partition_to_graph(&part, &g, pconn)) {
    printf("error converting partition to graph\n");
    goto fail;
  }

  if (args.connfile) {
    if (graph_copy_nodelabels(pconn, &g)) {
      printf("error copying node labels from %s\n", args.connfile);
      goto fail;
    }
  }

  if (ngdb_write(&g, args.output)) {
    printf("error writing graph to file %s\n", args.output);
    goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t _partition_to_graph(
  node_partition_t *part, graph_t *g, graph_t *conn) {
  
  uint64_t  i; 
  uint32_t *group;
  uint32_t  groupsz;

  g->neighbours = NULL;

  if (graph_create(g, part->nnodes, 0)) goto fail;

  for (i = 0; i < part->nparts; i++) {

    group   = (uint32_t *)part->parts[i].data;
    groupsz = part->parts[i].size;

    if (conn == NULL) {
      if (graph_connect(g, group, groupsz))
        goto fail;
    }
    else {
      if (graph_connect_from(g, conn, group, groupsz))
        goto fail;
    }
  }

  return 0;

fail:
  if (g->neighbours != NULL) graph_free(g);
  return 1;
}
