/**
 * Cache for graph statistics.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graph/graph.h"
#include "util/array.h"
#include "stats/stats_cache.h"

/**
 * Frees the memory used by the given cache. Called by graph_free.
 */
static void _cache_free(void *cache);

/**
 * Reads data from the given cache file.
 *
 * \return 0 on success, non-0 otherwise.
 */
static uint8_t _file_cache_read(
  FILE    *fd,    /**< file handle                                        */
  void    *data,  /**< place to store data (must be nvals*size in length) */
  uint32_t nvals, /**< number of values to read                           */
  uint16_t size,  /**< size of one value                                  */
  long     offset /**< offset into file                                   */
);

/**
 * Writes data to the given cache file.
 *
 * \return 0 on success, non-0 otherwise.
 */
static uint8_t _file_cache_write(
  FILE    *fd,    /**< file handle                                  */
  void    *data,  /**< data to write (must be nvals*size in length) */
  uint32_t nvals, /**< number of values to write                    */
  uint16_t size,  /**< size of one value                            */
  long     offset /**< offset into file                             */
);

/**
 * Searches in the given array for an entry with the given ID. If a
 * corresponding entry is found, and the 'entry' pointer is not NULL, the
 * found entry is copied into the 'entry' pointer.
 *
 * \return 1 if an entry was found, 0 if an entry was not found
 */
static uint8_t _get_cache_entry(
  array_t       *entries, /**< array of cache entries                */
  uint16_t       id,      /**< id to search for                      */
  cache_entry_t *entry    /**< pointer to store entry if it is found */
);

/**
 * Comparison function for cache_entry_t structs. Only tests
 * for equality, not order.
 *
 * \return 0 if the given entries have the same ID, non-0 otherwise.
 */
static int _entry_cmp(
  const void *a, /**< pointer to a cache_entry_t       */
  const void *b  /**< pointer to another cache_entry_t */
);

/**
 * Create a new graph cache entry.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _new_graph_field(
  stats_cache_t *c,   /**< the cache                            */
  cache_entry_t *ce,  /**< cache_entry_t struct to be populated */
  uint16_t       size /**< size of the value                    */
);

static uint8_t _new_list_field(
  stats_cache_t *c,   /**< the cache                            */
  cache_entry_t *ce,  /**< cache_entry_t struct to be populated */
  uint16_t       size /**< size of the value                    */
);

/**
 * Create a new node cache entry.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _new_node_field(
  stats_cache_t *c,   /**< the cache                            */
  cache_entry_t *ce,  /**< cache_entry_t struct to be populated */
  uint16_t       size /**< size of the value                    */

);

/**
 * Create a new pair cache entry.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _new_pair_field(
  stats_cache_t *c,   /**< the cache                            */
  cache_entry_t *ce,  /**< cache_entry_t struct to be populated */
  uint16_t       size /**< size of the value                    */

);

/**
 * Create a new edge cache entry.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _new_edge_field(
  stats_cache_t *c,   /**< the cache                            */
  cache_entry_t *ce,  /**< cache_entry_t struct to be populated */
  uint16_t       size /**< size of the value                    */
);

/**
 * Check to see if data has been cached for the given graph field.  If there
 * is cached data, and the 'd' pointer is not NULL, the data is copied into
 * the 'data' pointer.
 *
 * \return 1 if data is cached, 0 if it is not, -1 on failure.
 */
static int8_t _check_graph_field(
  stats_cache_t *c, /**< the cache                             */
  cache_entry_t *e, /**< the cache entry                       */
  uint32_t       u, /**< node index - ignored for graph fields */
  void          *d  /**< optional place to copy cached data    */
);

/**
 * Check to see if data has been cached for the given list field.  If there is
 * cached data, and the 'd' pointer is not NULL, the array struct is copied
 * into the 'data' pointer.
 *
 * \return 1 if data is cached, 0 if it is not, -1 on failure.
 */
static int8_t _check_list_field(
  stats_cache_t *c, /**< the cache                            */
  cache_entry_t *e, /**< the cache entry                      */
  uint32_t       u, /**< node index - ignored for list fields */
  void          *d  /**< optional place to copy cached data   */
);

/**
 * Check to see if data has been cached for the given node field, for the
 * given node. If there is cached data, and the 'd' pointer is not NULL, the
 * data is copied into the 'data' pointer.
 *
 * \return 1 if data is cached, 0 if it is not, -1 on failure.
 */
static int8_t _check_node_field(
  stats_cache_t *c, /**< the cache                          */
  cache_entry_t *e, /**< the cache entry                    */
  uint32_t       u, /**< node index                         */
  void          *d  /**< optional place to copy cached data */
);

/**
 * Check to see if data has been cached for the given pair field, for the
 * given node. If there is cached data, and the 'd' pointer is not NULL, the
 * data is copied into the 'data' pointer.
 *
 * \return 1 if data is cached, 0 if it is not, -1 on failure.
 */
static int8_t _check_pair_field(
  stats_cache_t *c, /**< the cache                          */
  cache_entry_t *e, /**< the cache entry                    */
  uint32_t       u, /**< node index                         */
  int64_t        v, /**< node index (optional)              */
  void          *d  /**< optional place to copy cached data */
);

/**
 * Check to see if data has been cached for the given edge field, for the
 * given node. If there is cached data, and the 'd' pointer is not NULL, the
 * data is copied into the 'data' pointer.
 *
 * \return 1 if data is cached, 0 if it is not, -1 on failure.
 */
static int8_t _check_edge_field(
  stats_cache_t *c, /**< the cache                          */
  cache_entry_t *e, /**< the cache entry                    */
  uint32_t       u, /**< node index                         */
  int64_t        v, /**< node index (optional)              */
  void          *d  /**< optional place to copy cached data */
);

/**
 * Updates the cached value for the given graph field.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_graph_field(
  stats_cache_t *c, /**< the cache                             */
  cache_entry_t *e, /**< the cache entry                       */
  uint32_t       u, /**< node index - ignored for graph fields */
  void          *d  /**< pointer to the new cache value        */
);

/**
 * Adds a new cached value for the given list field.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_list_field(
  stats_cache_t *c, /**< the cache                            */
  cache_entry_t *e, /**< the cache entry                      */
  uint32_t       u, /**< node index - ignored for list fields */
  void          *d  /**< pointer to the new cache value       */
);

/**
 * Updates the cached value for the given node field.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_node_field(
  stats_cache_t *c, /**< the cache                      */
  cache_entry_t *e, /**< the cache entry                */
  uint32_t       u, /**< node index                     */ 
  void          *d  /**< pointer to the new cache value */  
);

/**
 * Updates the cached value for the given pair field.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_pair_field(
  stats_cache_t *c, /**< the cache                      */
  cache_entry_t *e, /**< the cache entry                */
  uint32_t       u, /**< node index                     */
  int64_t        v, /**< node index (optional)          */
  void          *d  /**< pointer to the new cache value */
);

/**
 * Updates the cached value for the given edge field.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _update_edge_field(
  stats_cache_t *c, /**< the cache                      */
  cache_entry_t *e, /**< the cache entry                */
  uint32_t       u, /**< node index                     */
  int64_t        v, /**< node index (optional)          */
  void          *d  /**< pointer to the new cache value */
);

/**
 * Frees the memory used by the given graph cache.
 */
static void _free_graph_field(
  cache_entry_t *c /**< graph entry to free */
);

/**
 * Frees the memory used by the given list cache.
 */
static void _free_list_field(
  cache_entry_t *c /**< list entry to free */
);

/**
 * Frees the memory used by the given node cache.
 */
static void _free_node_field(
  cache_entry_t *c /**< node entry to free */
);

/**
 * Frees the memory used by the given pair cache.
 */
static void _free_pair_field(
  cache_entry_t *c /**< pair entry to free */
);

/**
 * Frees the memory used by the given edge cache.
 */
static void _free_edge_field(
  cache_entry_t *c /**< edge entry to free */
);

uint8_t stats_cache_init(graph_t *g) {

  uint32_t       nnodes;
  stats_cache_t *c;

  c      = NULL;
  nnodes = graph_num_nodes(g);

  /*allocate space for the cache struct*/
  c = calloc(1, sizeof(stats_cache_t));
  if (c == NULL) goto fail;
  
  c->g = g;
  
  /*initialise cache struct fields*/
  if (array_create(&(c->cache_entries), sizeof(cache_entry_t), 5)) goto fail;
  array_set_cmps(&c->cache_entries, _entry_cmp, NULL);

  /*attach the cache to the graph*/
  g->ctx[     _GRAPH_STATS_CACHE_CTX_LOC_] = c;
  g->ctx_free[_GRAPH_STATS_CACHE_CTX_LOC_] = _cache_free;

  return 0;
  
fail:
  if (c != NULL) _cache_free(c);
  return 1;
}

uint8_t stats_cache_reset(graph_t *g) {

  stats_cache_t *c;

  c = g->ctx[_GRAPH_STATS_CACHE_CTX_LOC_];

  if (c == NULL) goto fail;

  _cache_free(c);

  return stats_cache_init(g);
  
fail:
  return 1;
}

uint8_t stats_cache_add(
  graph_t *g, uint16_t id, cache_type_t type, uint16_t size) {


  stats_cache_t *c;
  cache_entry_t  e;
  uint8_t        res;

  c = g->ctx[_GRAPH_STATS_CACHE_CTX_LOC_];
  if (c == NULL) goto fail;

  /*ignore if an entry with the given id already exists*/
  if (_get_cache_entry(&(c->cache_entries), id, NULL)) return 0;

  e.id   = id;
  e.type = type;
  e.size = size;

  /*create the new entry*/
  switch (type) {
    case STATS_CACHE_TYPE_GRAPH: res = _new_graph_field(c, &e, size); break;
    case STATS_CACHE_TYPE_LIST:  res = _new_list_field( c, &e, size); break;
    case STATS_CACHE_TYPE_NODE:  res = _new_node_field( c, &e, size); break;
    case STATS_CACHE_TYPE_PAIR:  res = _new_pair_field( c, &e, size); break;
    case STATS_CACHE_TYPE_EDGE:  res = _new_edge_field( c, &e, size); break;
    default:                     goto fail;
  }

  if (res) goto fail;

  /*add the entry to the cache_entries list*/
  if (array_append(&(c->cache_entries), &e)) goto fail;

  return 0;

fail:
  return 1;
}

int8_t stats_cache_check(
  graph_t *g, uint16_t id, uint32_t u, int64_t v, void *d) {

  stats_cache_t *c;
  uint32_t       nnodes;
  cache_entry_t  e;
  int8_t         res;

  nnodes = graph_num_nodes(g);
  c      = g->ctx[_GRAPH_STATS_CACHE_CTX_LOC_];
  
  if (c == NULL)   return 0;
  if (u >= nnodes) goto fail;

  if (!_get_cache_entry(&(c->cache_entries), id, &e)) return 0;

  switch(e.type) {
    case STATS_CACHE_TYPE_GRAPH:
      res = _check_graph_field(c, &e, u, d);
      break;
    case STATS_CACHE_TYPE_LIST:
      res = _check_list_field( c, &e, u, d);
      break;
    case STATS_CACHE_TYPE_NODE:
      res = _check_node_field( c, &e, u, d);
      break;
    case STATS_CACHE_TYPE_PAIR:
      res = _check_pair_field( c, &e, u, v, d);
      break;
    case STATS_CACHE_TYPE_EDGE:
      res = _check_edge_field( c, &e, u, v, d);
      break;
      
    default: goto fail;
  }

  return res;

fail:
  return -1;
}

uint8_t stats_cache_update(
  graph_t *g, uint16_t id, uint32_t u, int64_t v, void *d) {

  stats_cache_t *c;
  uint32_t       nnodes;
  cache_entry_t  e;
  uint8_t        res;

  nnodes = graph_num_nodes(g);
  c = g->ctx[_GRAPH_STATS_CACHE_CTX_LOC_];

  if (u >= nnodes) goto fail;
  if (c == NULL)   return 0;

  if (!_get_cache_entry(&(c->cache_entries), id, &e)) goto fail;

  switch(e.type) {

    case STATS_CACHE_TYPE_GRAPH:
      res = _update_graph_field(c, &e, u, d);
      break;
    case STATS_CACHE_TYPE_LIST:
      res = _update_list_field(c, &e, u, d);
      break;
    case STATS_CACHE_TYPE_NODE:
      res = _update_node_field(c, &e, u, d);
      break;
    case STATS_CACHE_TYPE_PAIR:
      res = _update_pair_field(c, &e, u, v, d);
      break;
    case STATS_CACHE_TYPE_EDGE:
      res = _update_edge_field(c, &e, u, v, d);
      break;

    default: goto fail;
  }

  return res;
  
fail:
  return 1;
}

void _cache_free(void *cache) {

  int64_t        i;
  stats_cache_t *c;
  cache_entry_t  e;
  
  c = (stats_cache_t *)cache;

  if (c == NULL) return;

  for (i = 0; i < c->cache_entries.size; i++) {
    array_get(&(c->cache_entries), i, &e);

    switch(e.type) {
      case STATS_CACHE_TYPE_GRAPH: _free_graph_field(&e); break;
      case STATS_CACHE_TYPE_LIST:  _free_list_field( &e); break;
      case STATS_CACHE_TYPE_NODE:  _free_node_field( &e); break;
      case STATS_CACHE_TYPE_PAIR:  _free_pair_field( &e); break;
      case STATS_CACHE_TYPE_EDGE:  _free_edge_field( &e); break;
      default: continue;
    }
  }

  array_free(&(c->cache_entries));
  free(c);
}

uint8_t _file_cache_read(
  FILE *fd, void *data, uint32_t nvals, uint16_t size, long offset) {

  offset *= size;

  if (fseek(fd, offset, SEEK_SET))           goto fail;
  if (fread(data, size, nvals, fd) != nvals) goto fail;

  return 0;
  
fail:
  return 1;
}

uint8_t _file_cache_write(
  FILE *fd, void *data, uint32_t nvals, uint16_t size, long offset) {

  offset *= size;

  if (fseek( fd, offset, SEEK_SET))           goto fail;
  if (fwrite(data, size, nvals, fd) != nvals) goto fail;
  
  return 0;
  
fail:
  return 1;
}

uint8_t _get_cache_entry(
  array_t *entries, uint16_t id, cache_entry_t *entry) {

  int64_t       idx;
  cache_entry_t e1;

  e1.id = id;

  idx = array_find(entries, &e1, 0);

  if (idx == -1) return 0;

  if (entry != NULL) array_get(entries, idx, entry);

  return 1;
}

int _entry_cmp(const void *a, const void *b) {

  cache_entry_t *e1;
  cache_entry_t *e2;

  e1 = (cache_entry_t *)a;
  e2 = (cache_entry_t *)b;

  if (e1->id == e2->id) return 0;

  return 1;
}

uint8_t _new_graph_field(stats_cache_t *c, cache_entry_t *e, uint16_t size) {

  graph_cache_t *gc;

  gc = calloc(1, sizeof(graph_cache_t));
  if (gc == NULL) goto fail;

  gc->data = calloc(1, size);
  if (gc->data == NULL) goto fail;

  gc->cached = 0;
  e->cache   = gc;

  return 0;

fail:
  if (gc != NULL) {
    if (gc->data != NULL) free(gc->data);
    free(gc);
  }
  return -1;
}

uint8_t _new_list_field(stats_cache_t *c, cache_entry_t *ce, uint16_t size) {

  list_cache_t *lc;

  lc = calloc(1, sizeof(list_cache_t));
  if (lc == NULL) goto fail;
  lc->data.data = NULL;

  if (array_create(&(lc->data), size, 5)) goto fail;

  return 0;

fail:
  if (lc != NULL) {
    if (lc->data.data != NULL) array_free(&(lc->data));
    free(lc);
  }
  return 1;
}

uint8_t _new_node_field(stats_cache_t *c, cache_entry_t *e, uint16_t size) {

  uint32_t nnodes;
  node_cache_t *nc;

  nc = calloc(1, sizeof(node_cache_t));
  if (nc == NULL) goto fail;

  nnodes = graph_num_nodes(c->g);

  if (array_create(&nc->data, size, nnodes)) goto fail;

  nc->cached = calloc(nnodes, 1);
  if (nc->cached == NULL) goto fail;

  e->cache = nc;

  return 0;

fail:
  if (nc != NULL) {
    if (nc->data.data != NULL) array_free(&nc->data);
    if (nc->cached    != NULL) free(nc->cached);
  }
  return -1;
}

uint8_t _new_pair_field(stats_cache_t *c, cache_entry_t *e, uint16_t size) {

  uint32_t      nnodes;
  file_cache_t *fc;

  nnodes       = graph_num_nodes(c->g);

  fc = calloc(1, sizeof(file_cache_t));
  if (fc == NULL) goto fail;

  fc->cached = calloc(nnodes, sizeof(uint8_t));
  if (fc->cached == NULL) goto fail;
  
  fc->cachefile = tmpfile();
  if (fc->cachefile == NULL) goto fail;

  e->cache = fc;

  return 0;
  
fail:
  if (fc != NULL) {
    if (fc->cached    != NULL) free(fc->cached);
    if (fc->cachefile != NULL) fclose(fc->cachefile);
  }
  return -1;
}

uint8_t _new_edge_field(stats_cache_t *c, cache_entry_t *e, uint16_t size) {

  uint32_t nnodes;
  edge_cache_t *ec;

  ec = calloc(1, sizeof(edge_cache_t));
  if (ec == NULL) goto fail;

  nnodes = graph_num_nodes(c->g);

  if (edge_array_create(c->g, size, &ec->data)) goto fail;

  ec->cached = calloc(nnodes, 1);
  if (ec->cached == NULL) goto fail;

  e->cache = ec;

  return 0;

fail:
  if (ec != NULL) {
    if (ec->data.vals != NULL) edge_array_free(&ec->data);
    if (ec->cached    != NULL) free(ec->cached);
  }
  return -1;
}

int8_t _check_graph_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, void *d) {

  graph_cache_t *gc;

  gc = e->cache;

  if (!gc->cached) return 0;

  if (d != NULL) memcpy(d, gc->data, e->size);

  return 1;
}

int8_t _check_list_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, void *d) {

  list_cache_t *lc;

  lc = e->cache;

  if (lc->data.size == 0) return 0;

  if (d != NULL) memcpy(d, &(lc->data), sizeof(array_t));

  return 1;
}

int8_t _check_node_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, void *d) {

  node_cache_t *nc;

  nc = e->cache;

  if (!nc->cached[u]) return 0;

  if (d != NULL) array_get(&nc->data, u, d);

  return 1;
}

int8_t _check_pair_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, int64_t v, void *d) {

  uint32_t      nnodes;
  file_cache_t *fc;

  fc     = e->cache;
  nnodes = graph_num_nodes(c->g);

  if (!fc->cached[u]) return 0;

  if (d != NULL) {
    if (_file_cache_read(fc->cachefile, d, nnodes, e->size, u*nnodes))
      goto fail;
  }

  return 1;
fail:
  return -1;
}

int8_t _check_edge_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, int64_t v, void *d) {

  uint32_t      nnbrs;
  uint32_t      sz;
  edge_cache_t *ec;
  void         *tmp;

  ec    = e->cache;
  nnbrs = graph_num_neighbours(c->g, u);

  if (!ec->cached[u] && v < 0) return 0;

  if (d != NULL) {

    if (v < 0) sz = e->size * nnbrs;
    else       sz = e->size;
    
    tmp = edge_array_get_all(&ec->data, u);
    memcpy(d, tmp, sz);
  }

  return 1;
}

uint8_t _update_graph_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, void *d) {

  graph_cache_t *gc;
  gc = e->cache;
  memcpy(gc->data, d, e->size);
  gc->cached = 1;

  return 0;
}

uint8_t _update_list_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, void *d) {

  list_cache_t *lc;
  lc = e->cache;

  if (array_append(&(lc->data), d)) goto fail;

  return 0;
  
fail:
  return 1;
}

uint8_t _update_node_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, void *d) {

  node_cache_t *nc;

  nc  = e->cache;

  array_set(&nc->data, u, d);
  nc->cached[u] = 1;
  
  return 0;
}

uint8_t _update_pair_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, int64_t v, void *d) {

  uint32_t      nnodes;
  file_cache_t *fc;

  nnodes = graph_num_nodes(c->g);
  fc     = e->cache;

  if (_file_cache_write(fc->cachefile, d, nnodes, e->size, u*nnodes))
    goto fail;

  fc->cached[u] = 1;
  
  return 0;

fail:
  return 1;
}

uint8_t _update_edge_field(
  stats_cache_t *c, cache_entry_t *e, uint32_t u, int64_t v, void *d) {

  uint32_t      nnbrs;
  edge_cache_t *ec;

  nnbrs = graph_num_neighbours(c->g, u);
  ec    = e->cache;

  if (v < 0) 
    edge_array_set_all(&ec->data, u, d);
  else 
    edge_array_set(&ec->data, u, v, d);
  
  ec->cached[u] = 1;
  if (v >= 0) ec->cached[v] = 1;

  return 0;
}

static void _free_graph_field(cache_entry_t *c) {
  
  graph_cache_t *gc;
  gc = (graph_cache_t *)c->cache;
  free(gc->data);
  free(gc);
}

void _free_list_field(cache_entry_t *c) {

  list_cache_t *lc;
  lc = (list_cache_t *)c->cache;
  array_free(&(lc->data));
  free(lc);
}

static void _free_node_field(cache_entry_t *c) {
  
  node_cache_t *nc;
  nc = (node_cache_t *)c->cache;
  array_free(&nc->data);
  free(nc->cached);
  free(nc);
}

static void _free_pair_field(cache_entry_t *c) {

  file_cache_t *fc;
  fc = (file_cache_t *)c->cache;
  free(fc->cached);
  fclose(fc->cachefile);
  free(fc);
}

static void _free_edge_field(cache_entry_t *c) {
  
  edge_cache_t *ec;
  ec = (edge_cache_t *)c->cache;
  edge_array_free(&ec->data);
  free(ec->cached);
  free(ec);
}
