/**
 * Reads/writes a graph_t struct from/to a ngdb file.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "graph/graph_trail.h"
#include "io/ngdb.h"
#include "util/array.h"
#include "util/compare.h"
#include "io/ngdb_graph.h"

/**
 * Reads the neighbours for the given node.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_refs(
  ngdb_t  *ngdb,  /**< ngdb handle  */
  graph_t *graph, /**< ptr to graph */
  uint32_t nidx   /**< node index   */
);

/**
 * Reads the label for the given node.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_label(
  ngdb_t  *ngdb,  /**< ngdb handle  */
  graph_t *graph, /**< ptr to graph */
  uint32_t nidx   /**< node index   */
);

/**
 * Reads header data from the ngdb file, adding it as a
 * trail to the graph (see graph_trail.h).
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_hdr(
  ngdb_t  *ngdb, /**< ngdb handle  */
  graph_t *graph /**< graph handle */
);

uint8_t ngdb_read(char *ngdbfile, graph_t *graph) {

  ngdb_t  *ngdb;
  uint32_t i;
  uint32_t nnodes;

  ngdb = NULL;

  memset(graph, 0, sizeof(graph_t));

  ngdb = ngdb_open(ngdbfile);
  if (ngdb == NULL) goto fail;

  nnodes = ngdb_num_nodes(ngdb);

  if (graph_create(graph, nnodes, 0)) goto fail;
  if (_read_hdr(ngdb, graph))         goto fail;

  for (i = 0; i < nnodes; i++) {
    if (_read_refs (ngdb, graph, i) != 0) goto fail;
    if (_read_label(ngdb, graph, i) != 0) goto fail;
  }

  ngdb_close(ngdb);

  return 0;
fail: 

  if (ngdb != NULL) ngdb_close(ngdb);
  graph_free(graph);

  return 1;
}

static uint8_t _read_hdr(ngdb_t *ngdb, graph_t *graph) {
  
  uint8_t *hdrdata;
  uint16_t hdrlen;

  hdrdata = NULL;
  hdrlen  = ngdb_hdr_data_len(ngdb);

  if (hdrlen == 0) return 0;

  hdrdata = malloc(hdrlen);
  if (hdrdata == NULL) goto fail;

  if (ngdb_hdr_get_data(ngdb, hdrdata))         goto fail;
  if (graph_trail_init(graph))                  goto fail;
  if (graph_trail_import(graph, hdrdata, "\n")) goto fail;

  return 0;
  
fail:
  if (hdrdata != NULL) free(hdrdata);
  return 1;
}

uint8_t _read_refs(ngdb_t *ngdb, graph_t *graph, uint32_t nidx) {

  uint64_t  i;
  uint32_t  numrefs;
  uint32_t *refs;

  refs = NULL;

  numrefs = ngdb_node_num_refs(ngdb, nidx);

  if (numrefs == 0xFFFFFFFF) goto fail;

  if (numrefs > 0) {

    refs = malloc(numrefs*sizeof(uint32_t));

    if (refs                                     == NULL)   goto fail;
    if (ngdb_node_get_all_refs(ngdb, nidx, refs) != 0)      goto fail;
    if (array_expand(&graph->neighbours[nidx], numrefs+1))  goto fail;
    if (array_expand(&graph->weights[   nidx], numrefs+1))  goto fail;

    for (i = 0; i < numrefs; i++) {
      if (graph_add_edge(graph, nidx, refs[i], 1.0)) goto fail;
    }
    
    free(refs);
    refs = NULL;
  }

  return 0;
fail:
  if (refs != NULL) free(refs);
  return 1;
}

uint8_t _read_label(ngdb_t *ngdb, graph_t *graph, uint32_t nidx) {

  uint8_t bytes[sizeof(graph_label_t)];

  if (ngdb_node_get_data(ngdb, nidx, bytes)) goto fail;

  graph_set_nodelabel(graph, nidx, (graph_label_t *)bytes);

  return 0;

fail:
  return 1;
}

/**
 * Writes the ngdb file header.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _write_hdr(
  ngdb_t  *ngdb, /**< ngdb handle  */
  graph_t *g     /**< ptr to graph */
);

/**
 * Writes the graph nodes to the ngdb file.
 *
 * \return 0 on success, non-0 on failure. 
 */
static uint8_t _write_nodes(
  ngdb_t  *ngdb, /**< ngdb handle  */
  graph_t *g     /**< ptr to graph */
);

/**
 * Writes the graph edges as references to the ngdb file.
 *
 * \return 0 on success, non-0 on failure. 
 */
static uint8_t _write_refs(
  ngdb_t  *ngdb, /**< ngdb handle  */
  graph_t *g     /**< ptr to graph */
);

uint8_t ngdb_write(graph_t *g, char *f) {

  ngdb_t *ngdb;

  ngdb = ngdb_create(
    f,
    graph_num_nodes(g),
    NGDB_HDR_DATA_SIZE,
    sizeof(graph_label_t),
    8);

  if (ngdb == NULL)          goto fail;
  if (_write_hdr  (ngdb, g)) goto fail;
  if (_write_nodes(ngdb, g)) goto fail;
  if (_write_refs (ngdb, g)) goto fail;
  if (ngdb_close(ngdb))      goto fail;

  return 0;

fail:
  if (ngdb != NULL) ngdb_close(ngdb);
  return 1;
}

uint8_t _write_hdr(ngdb_t *ngdb, graph_t *g) {

  uint16_t len;
  uint8_t *data;
  char    *delim = "\n";

  data = NULL;

  if (!graph_trail_exists(g)) return 0;

  len = graph_trail_total_len(g) +
       (graph_trail_num_msgs(g)-1) * strlen(delim) + 1;

  data = calloc(len, 1);
  if (data != NULL) goto fail;

  if (graph_trail_export(g, data, delim)) goto fail;

  if (len > NGDB_HDR_DATA_SIZE) {
    len = NGDB_HDR_DATA_SIZE;
    data[len-1] = '\0';
  }

  if (ngdb_hdr_set_data(ngdb, data, len)) goto fail;

  free(data);

  return 0;
  
fail:
  if (data != NULL) free(data);
  return 1;
}

uint8_t _write_nodes(ngdb_t *ngdb, graph_t *g) {

  uint32_t       i;
  uint32_t       nnodes;
  graph_label_t *lbl;

  nnodes = graph_num_nodes(g);

  for (i = 0; i < nnodes; i++) {

    lbl = graph_get_nodelabel(g, i);
    if (lbl == NULL) break;

    if (ngdb_node_set_data(ngdb, 
                           i, 
                           (uint8_t *)lbl, 
                           sizeof(graph_label_t))) 
      goto fail;
  }

  return 0;
fail:
  return 1;
}

uint8_t _write_refs(ngdb_t *ngdb, graph_t *g) {

  uint32_t  u;
  uint32_t  v;
  uint32_t  nnodes;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  float    *wts;
  uint8_t  *data;
  uint8_t   dlen;

  nnodes = graph_num_nodes(g);

  for (u = 0; u < nnodes; u++) {

    nnbrs = graph_num_neighbours(g, u);
    nbrs  = graph_get_neighbours(g, u);
    wts   = graph_get_weights   (g, u);

    for (v = 0; v < nnbrs; v++) {

      data = (wts == NULL) ? NULL : (uint8_t *)(wts+v);
      dlen = (wts == NULL) ? 0    : sizeof(float);
      
      if (ngdb_add_ref(ngdb, u, nbrs[v], data, dlen) == 0xFFFFFFFF)
        goto fail;
    }
  }

  return 0;
fail:
  return 1;
}
