/**
 * Calculates and prints a bunch of statistics over a graph.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <argp.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"


static char doc[] = "cnet - calculate and print statistics "\
                    "over ngdb graph files";

static struct argp_option options[] = {
  {"assortativity", 'a', NULL,  0, "print the assortativity"},
  {"nodelabel",     'b', NULL,  0, "print the label for each node"},
  {"connected",     'c', NULL,  0, "print the number of "\
                                   "connected/disconnected nodes"},
  {"density",       'd', NULL,  0, "print the graph density"},
  {"edges",         'e', NULL,  0, "print the number of edges"},
  {"lefficiency",   'f', NULL,  0, "print the local efficiency"},
  {"degree",        'g', NULL,  0, "print the average degree"},
  {"numpaths",      'h', NULL,  0, "print the number of paths"},
  {"closeness",     'i', NULL,  0, "print the closeness centrality"},
  {"betweenness",   'j', NULL,  0, "print the betweenness centrality"},
  {"comppops",      'k', NULL,  0, "print the nodes in each component"},
  {"clustering",    'l', NULL,  0, "print the clustering coefficient"},
  {"components",    'm', NULL,  0, "print the number of components"},
  {"nodes",         'n', NULL,  0, "print the number of nodes"},
  {"modularity",    'o', NULL,  0, "print the modularity"},
  {"pathlength",    'p', NULL,  0, "print the characteristic path length"},
  {"newmanerror",   'q', NULL,  0, "print the newman clustering error"},
  {"approxclust",   'r', "NSAMPLES", OPTION_ARG_OPTIONAL,
                                   "print an approximation of the "\
                                   "clustering coefficient"},
  {"ersmallworld",  's', NULL,  0, "print the small-world index, using "\
                                   "Erdos-Renyi random graphs for comparison"},
  {"nintra",        't', NULL,  0, "print the number of intra-cluster edges"},
  {"ninter",        'u', NULL,  0, "print the number of inter-cluster edges"},
  {"edgedist",      'v', NULL,  0, "print the distance of all edges"},
  {"labelvals",     'w', NULL,  0, "print unique node label values"},
  {"all",           'x', NULL,  0, "print everything"},
  {"gefficiency",   'y', NULL,  0, "print the global efficiency"},
  {"mutualinfo",    'z', NULL,  0, "print the normalised mutual information"},
  {"compspan",      'A', NULL,  0, "print spatial span of each component"},
  {"nodestart",     'B', "INT", 0, "start index for printing node values"},
  {"nodeend",       'C', "INT", 0, "end index for printing node values"},
  {"alledges",      'D', NULL,  0, "print all edges"},
  {"avgedist",      'E', NULL,  0, "print average edge distance for each "\
                                   "node"},
  {"degcent",       'F', NULL,  0, "print the degree centrality for each " \
                                   "node"},
  {"chira",         'G', NULL,  0, "print the Chira community strength"},
  {"ebmatrix",      '0', NULL,  0, "print edge-betweenness matrix"},
  {"psmatrix",      '1', NULL,  0, "print path-sharing matrix"},
  {0}
};

struct args {
  char    *input;
  int64_t  nodestart;
  int64_t  nodeend;
  uint8_t  assortativity;
  uint8_t  nodelabel;
  uint8_t  connected;
  uint8_t  density;
  uint8_t  edges;
  uint8_t  lefficiency;
  uint8_t  degree;
  uint8_t  numpaths;
  uint8_t  closeness;
  uint8_t  betweenness;
  uint8_t  comppops;
  uint8_t  clustering;
  uint8_t  components;
  uint8_t  nodes;
  uint8_t  modularity;
  uint8_t  pathlength;
  uint8_t  newmanerror;
  int32_t  approxclust;
  uint8_t  ersmallworld;
  uint8_t  nintra;
  uint8_t  ninter;
  uint8_t  edgedist;
  uint8_t  labelvals;
  uint8_t  gefficiency;
  uint8_t  mutualinfo;
  uint8_t  compspan;
  uint8_t  alledges;
  uint8_t  avgedist;
  uint8_t  degcent;
  uint8_t  chira;
  
  uint8_t  ebmatrix;
  uint8_t  psmatrix;
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  struct args *a = state->input;

  switch (key) {

    case 'a': a->assortativity = 0xFF; break;
    case 'b': a->nodelabel     = 0xFF; break;
    case 'c': a->connected     = 0xFF; break;
    case 'd': a->density       = 0xFF; break;
    case 'e': a->edges         = 0xFF; break;
    case 'f': a->lefficiency   = 0xFF; break;
    case 'g': a->degree        = 0xFF; break;
    case 'h': a->numpaths      = 0xFF; break;
    case 'i': a->closeness     = 0xFF; break;
    case 'j': a->betweenness   = 0xFF; break;
    case 'k': a->comppops      = 0xFF; break;
    case 'l': a->clustering    = 0xFF; break;
    case 'm': a->components    = 0xFF; break;
    case 'n': a->nodes         = 0xFF; break;
    case 'o': a->modularity    = 0xFF; break;
    case 'p': a->pathlength    = 0xFF; break;
    case 'q': a->newmanerror   = 0xFF; break;
    case 'r': 
      /*when print_stats sees a -1, it will 
        generate a value based on the graph size*/
      if (arg == 0) a->approxclust = -1;
      else          a->approxclust = atoi(arg); 
      break;
    case 's': a->ersmallworld  = 0xFF; break;
    case 't': a->nintra        = 0xFF; break;
    case 'u': a->ninter        = 0xFF; break;
    case 'v': a->edgedist      = 0xFF; break;
    case 'w': a->labelvals     = 0xFF; break;
    case 'x': 
      memset(&(a->assortativity), 0xFF, sizeof(struct args)-sizeof(char*));
      break;
    case 'y': a->gefficiency   = 0xFF;      break;
    case 'z': a->mutualinfo    = 0xFF;      break;
    case 'A': a->compspan      = 0xFF;      break;
    case 'B': a->nodestart     = atoi(arg); break;
    case 'C': a->nodeend       = atoi(arg); break;
    case 'D': a->alledges      = 0xFF;      break;
    case 'E': a->avgedist      = 0xFF;      break;
    case 'F': a->degcent       = 0xFF;      break;
    case 'G': a->chira         = 0xFF;      break;
    
    case '0': a->ebmatrix      = 0xFF;      break;
    case '1': a->psmatrix      = 0xFF;      break;
    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) a->input  = arg;
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

static void print_matrix(
  graph_t *g, uint8_t (*func)(graph_t *g, uint32_t u, double *d));
static void print_matrix_line(graph_t *g, uint32_t nidx, double *data);
static void print_stats(graph_t *g, struct args *args);
static void print_edge_vals(
  graph_t *g, double (*func)(graph_t *g, uint32_t u, uint32_t v),
  char *prefix);

int main (int argc, char *argv[]) {

  graph_t     g;
  struct args args;
  struct argp argp = {options, _parse_opt, "INPUT", doc};

  memset(&args, 0, sizeof(struct args));
  args.nodestart = -1;
  args.nodeend   = -1; 

  startup("cnet", argc, argv, &argp, &args);

  if (ngdb_read(args.input, &g) != 0) {
    printf("error loading %s\n", args.input);
    goto fail;
  }

  if (stats_cache_init(&g)) {
    printf("error initialising stats cache\n");
    goto fail;
  }

  print_stats(&g, &args);

  return 0;
fail:
  return 1;
}

void print_stats(graph_t *g, struct args *args) {

  uint64_t       i;
  uint64_t       j;
  uint32_t       nodestart;
  uint32_t       nodeend;
  uint32_t       numnodes;
  uint32_t       numcmps;
  double         degree;
  double         degcent;
  double         swidx;
  double         pathlength;
  double         locefficiency;
  uint32_t       connected;
  double         clustering;
  double         closeness;
  double         betweenness;
  
  uint32_t      *components;
  array_t        cmpsizes;
  graph_label_t *label;
  double         tmp;
  uint32_t       nlblvals;
  uint32_t      *lblvals;

  degree         = 0;
  degcent        = 0;
  swidx          = 0;
  pathlength     = 0;
  connected      = 0;
  locefficiency  = 0;
  clustering     = 0;
  closeness      = 0;
  betweenness    = 0;
  nlblvals       = 0;
  
  components     = NULL;
  array_create(&cmpsizes, sizeof(uint32_t), 10);

  numnodes  = graph_num_nodes(g);
  connected = stats_cache_connected(g);

  if (args->nodestart == -1) nodestart = 0;
  else                       nodestart = args->nodestart;
  if (args->nodeend   == -1) nodeend   = numnodes;
  else                       nodeend   = args->nodeend; 

  components = calloc(numnodes, sizeof(uint32_t));

  if (args->nodelabel) {

    for (i = nodestart; i < nodeend; i++) {
      label = graph_get_nodelabel(g,i);
      printf("label %" PRIu64 ":\t%u,%f,%f,%f\n", i, 
        label->labelval, 
        label->xval, 
        label->yval, 
        label->zval);
    }
    printf("\n");
  }

  if (args->degree) {

    for (i = nodestart; i < nodeend; i++) {
      tmp = stats_degree(g, i);
      degree += tmp;

      printf("degree %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->degcent) {

    for (i = nodestart; i < nodeend; i++) {
      tmp = stats_degree_centrality(g, i);
      degcent += tmp;

      printf("degree centraliy %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  } 

  if (args->ersmallworld) {
    swidx = stats_smallworld_index(g);
  }

  if (args->clustering) {

    for (i = nodestart; i < nodeend; i++) {
      stats_cache_node_clustering(g, i, &tmp);
      clustering += tmp;

      printf("clustering %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->pathlength) {

    for (i = nodestart; i < nodeend; i++) {
      stats_cache_node_pathlength(g, i, &tmp);
      pathlength += tmp;

      printf("pathlength %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->closeness) {

    for (i = nodestart; i < nodeend; i++) {
      tmp = stats_closeness_centrality(g, i);
      closeness += tmp;
      
      printf("closeness %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->betweenness) {

    for (i = nodestart; i < nodeend; i++) {
      stats_cache_betweenness_centrality(g, i, &tmp);
      betweenness += tmp;
      
      printf("betweenness %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->lefficiency) {

    for (i = nodestart; i < nodeend; i++) {

      stats_cache_node_local_efficiency(g, i, &tmp);
      locefficiency += tmp;

      printf("efficiency %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->numpaths) {

    for (i = nodestart; i < nodeend; i++) {

      stats_cache_node_numpaths(g, i, &tmp);
      printf("numpaths %" PRIu64 ":\t%f\n", i, tmp);
    }
    printf("\n");
  }

  if (args->components) {
    stats_num_components(g, 1, &cmpsizes, components);
    for (i = nodestart; i < nodeend; i++) {
      printf("component %" PRIu64 ":\t%u\n", i, components[i]);
    }
    for (i = 0; i < cmpsizes.size; i++) {
      printf("component %" PRIu64 " size:\t%u\n", i,
             ((uint32_t *)(cmpsizes.data))[i]);
    }
    printf("\n");
  }

  if (args->comppops) {
    stats_num_components(g, 1, &cmpsizes, components);

    for (i = 0; i < cmpsizes.size; i++) {
      printf("component %" PRIu64 " population: ", i);

      for (j = 0; j < numnodes; j++) {
        if (components[j] == i) printf("%" PRIu64 " ", (j+1));
      }
      printf("\n");
    }
  }

  if (args->labelvals) {
    nlblvals = graph_num_labelvals(g);
    lblvals  = graph_get_labelvals(g);
    

    for (i = 0; i < nlblvals; i++) {
      tmp = stats_num_labelled_nodes(g, lblvals[i]);
      printf("label value %" PRIu64 ": %u (%0.0f)\n", i, lblvals[i], tmp);
    }
  }

  if (args->compspan) {
    numcmps = stats_cache_num_components(g);

    for (i = 0; i < numcmps; i++) {

      printf("component %" PRIu64 " span: %0.6f\n",
             i, stats_component_span(g, i));
    }
  }

  if (args->avgedist) {

    for (i = 0; i < numnodes; i++) {

      printf("avg edge distance %" PRIu64 ": %0.6f\n",
             i, stats_avg_edge_distance(g, i));
    }
  }

  degree         /= (nodeend - nodestart);
  degcent        /= (nodeend - nodestart);
  clustering     /= (nodeend - nodestart);
  pathlength     /= connected;
  locefficiency  /= connected;
  closeness      /= (nodeend - nodestart);
  betweenness    /= (nodeend - nodestart);
  
  if (args->nodes)
    printf("nodes:                 %u\n",    numnodes);
  if (args->edges)
    printf("edges:                 %u\n",    graph_num_edges(g));
  if (args->connected)
    printf("dis/connected:         %u/%u\n", numnodes-connected, connected);
  if (args->density)
    printf("density:               %f\n",    stats_density(g));
  if (args->degree)
    printf("avg degree:            %f\n",    degree);
  if (args->degcent)
    printf("avg degree centrality: %f\n",    degcent); 
  if (args->components) {
    printf("components:            %u\n",    cmpsizes.size);
  }
  if (args->clustering)
    printf("avg clustering:        %f\n",    clustering);
  if (args->approxclust) {
    if (args->approxclust < 0) args->approxclust = numnodes/10;
    printf("approx. clustering:    %f\n",    stats_cache_approx_clustering(
                                               g,args->approxclust));
  }
  if (args->pathlength)
    printf("avg pathlength:        %f\n",    pathlength);
  if (args->gefficiency)
    printf("global efficiency:     %f\n",    stats_cache_global_efficiency(g));
  if (args->lefficiency)
    printf("avg local efficiency:  %f\n",    locefficiency);
  if (args->ersmallworld) {
    printf("er clustering:         %f\n",    stats_er_clustering(g));
    printf("er pathlength:         %f\n",    stats_er_pathlength(g));
    printf("small-world index:     %f\n",    swidx);
  }
  if (args->assortativity)
    printf("assortativity:         %f\n",    stats_cache_assortativity(g));
  if (args->closeness)
    printf("closeness:             %f\n",    closeness);
  if (args->betweenness)
    printf("betweenness:           %f\n",    betweenness);
  if (args->modularity)
    printf("modularity:            %f\n",    stats_cache_modularity(g));
  if (args->chira)
    printf("chira fitness:         %f\n",    stats_cache_chira(g)); 
  if (args->nintra)
    printf("intra-cluster edges:   %0.0f\n", stats_cache_intra_edges(g));
  if (args->ninter)
    printf("inter-cluster edges:   %0.0f\n", stats_cache_inter_edges(g));
  if (args->labelvals)
    printf("num label vals:        %u\n",    nlblvals);
  if (args->newmanerror)
    printf("newman error:          %f\n",    stats_newman_error(g));
  if (args->mutualinfo) {
    printf("disco mutual info:     %f\n",    stats_mutual_information(g, 1));
    printf("mutual info:           %f\n",    stats_mutual_information(g, 0));
  }

  if (args->ebmatrix) print_matrix(   g, &stats_cache_edge_betweenness);
  if (args->psmatrix)
    print_edge_vals(g, &stats_edge_pathsharing, "path-sharing");
  if (args->edgedist)
    print_edge_vals(g, &stats_edge_distance, "distance");
  if (args->alledges)
    print_edge_vals(g, &graph_get_weight, "edge");
}

void print_matrix(
  graph_t *g, uint8_t (*func)(graph_t *g, uint32_t u, double *d)) {

  double  *d;
  uint64_t i;
  uint32_t nnodes;
  
  nnodes = graph_num_nodes(g);
  d      = malloc(nnodes*sizeof(double));

  printf("    | ");
  for (i = 0; i < nnodes; i++) printf("%03" PRIu64 "     ", i); printf("\n");
  printf("    | ");
  for (i = 0; i < nnodes; i++) printf("--------"); printf("\n");

  for (i = 0; i < nnodes; i++) {
    func(g, i, d);
    print_matrix_line(g, i, d);
  }
  free(d);
}

void print_matrix_line(graph_t *g, uint32_t nidx, double *data) {

  uint64_t i;
  uint64_t j;
  uint32_t nnodes;

  nnodes = graph_num_nodes(g);

  j = 0;
  printf("%03u | ", nidx);
  for (i = 0; i < nnodes; i++) {

    if (!graph_are_neighbours(g, nidx, i)) printf("------- ");
    else printf("%7.3f ", data[j++]);
  }
  printf("\n");
}

void print_edge_vals(
  graph_t *g,
  double (*func)(graph_t *g, uint32_t u, uint32_t v),
  char *prefix) {

  uint64_t  i;
  uint64_t  j;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  double    val;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {
    
    nnbrs = graph_num_neighbours(g, i);
    nbrs  = graph_get_neighbours(g, i);

    for (j = 0; j < nnbrs; j++) {

      if (nbrs[j] < i) continue;

      val = func(g, i, nbrs[j]);

      printf("%s %" PRIu64 " -- %u: %0.6f\n", prefix, i, nbrs[j], val);
    }
  }
}
