/**
 *   ngdb - Not the Gnu DeBugger (nGraphDataBase)
 *
 * ngdb is a simple file format and API for file-based storage and access of
 * graph based data. See README.NGDB for documentation.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "io/ngdb.h"

/**********************************
 * Data structures and definitions.
 *********************************/

#define NGDB_FILE_ID   0x1357
#define NGDB_NODE_SYNC 0x2468
#define NGDB_REF_SYNC  0x9753

typedef enum __ngdb_mode {

  NGDB_MODE_READ,
  NGDB_MODE_CREATE
} ngdb_mode_t;

/**
 * An ngdb graph file.
 */
struct __ngdb {

  FILE       *fid;       /**< handle to the graph file          */
  uint16_t    hdata_len; /**< header data section length        */
  uint16_t    ndata_len; /**< node data section length          */
  uint16_t    rdata_len; /**< reference data section length     */
  uint32_t    num_nodes; /**< number of nodes in the graph      */
  uint32_t    num_refs;  /**< number of references in the graph */
  ngdb_mode_t mode;      /**< read only/create mode             */

};

/**
 * A node section. 
 */
typedef struct __node {

  uint32_t  num_refs;  /**< number of references this node has */
  uint32_t  first_ref; /**< address of first reference         */
  uint32_t  last_ref;  /**< address of last reference          */
  uint8_t  *data;      /**< pointer to the data in this node   */
  uint16_t  dlen;      /**< actual length of the data array    */
  uint32_t  idx;       /**< index of this node                 */

} node_t;

/**
 * A reference section. 
 */
typedef struct __ref {

  uint32_t idx;  /**< node index of this reference      */
  uint32_t next; /**< address of next reference         */
  uint32_t addr; /**< address of this reference         */
  uint8_t *data; /**< pointer to data in this reference */
  uint16_t dlen; /**< actual length of the data array   */

} ref_t;

/*****************************
 * Private function prototypes
 ****************************/

/**
 * Reads the header from the start of the ngdb->fid file.and copies the
 * information into the other fields of ngdb_t. Also checks the file ID bytes,
 * and fails if they are not correct. 
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_read_header(
  ngdb_t *ngdb /**< ngdb struct with open fid */
);

/**
 * Write a header to the start of the ngdb->fid file, with the information
 * contained in ngdb.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_write_header(
  ngdb_t *ngdb /**< ngdb struct with open fid */
);

/**
 * Read the header data from the given graph file.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_read_hdr_data(
  ngdb_t  *ngdb, /**< the graph in question      */
  uint8_t *data  /**< memory to store the header */
);

/**
 * Write the given data to the header in the given graph file.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_write_hdr_data(
  ngdb_t  *ngdb, /**< the graph in question */
  uint8_t *data, /**< the data              */
  uint16_t dlen  /**< length of the data    */
);

/**
 * Reads the node at the given index from the graph, including its data. The
 * node->data array must either be NULL, or be big enough to store
 * ngdb->data_len bytes; if node->data is NULL, the data is not read in or
 * copied.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_read_node(
  ngdb_t   *ngdb, /**< the graph in question         */
  uint32_t  idx,  /**< the node to read              */
  node_t   *node  /**< node struct to copy node into */
);

/**
 * Writes/rewrites the given node to the given graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_write_node(
  ngdb_t *ngdb, /**< the graph in question */
  node_t *node  /**< node info             */
);

/**
 * Read the reference at the given file address.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_read_ref(
  ngdb_t   *ngdb, /**< the graph in question            */
  uint32_t  addr, /**< address of reference             */
  ref_t    *ref   /**< ref_t struct to put reference in */
);

/**
 * Writes/rewrites the given reference to the graph.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _ngdb_write_ref(
  ngdb_t *ngdb, /**< the graph in question  */
  ref_t  *ref   /**< the reference to write */
);

/**
 * Appends the given reference address to the given node's reference list.
 *
 * \return 0 on success, non-0 on error;
 */
static uint8_t _ngdb_add_to_ref_list(
  ngdb_t   *ngdb, /**< the graph in question */
  node_t   *node, /**< the node in question  */
  ref_t    *ref   /**< the new reference     */
);

/**
 * Translates the given node index into a file location.
 *
 * \return the file location.
 */
static uint32_t _ngdb_idx_to_addr(
  ngdb_t   *ngdb, /**< the graph in question */
  uint32_t  idx   /**< the node in question  */
);

/**
 * \return a file address to write the next reference to.
 */
static uint32_t _ngdb_new_ref_addr(
  ngdb_t *ngdb /**< The graph in question */
);

/******************
 * Public functions
 *****************/

/**
 * Opens the given file, reads the header from the file, and creates and
 * populates a ngdb_t struct. Fails if malloc or fopen fails, or if the file
 * does not appear to be an ngdb file.
 */
ngdb_t * ngdb_open(char *filename) {

  ngdb_t *ngdb;
  ngdb = NULL;

  if (filename == NULL) goto fail;

  ngdb = malloc(sizeof(ngdb_t));
  if (ngdb == NULL) goto fail;

  ngdb->fid = fopen(filename, "r");
  if (ngdb->fid == NULL) goto fail;

  if (_ngdb_read_header(ngdb) != 0) goto fail;

  ngdb->mode = NGDB_MODE_READ;
  
  return ngdb;

fail:

  if (ngdb != NULL) {
    if (ngdb->fid != NULL) fclose(ngdb->fid);
    free(ngdb);
  }
  return NULL;
}

/**
 * Closes ngdb->fid and frees ngdb. If ngdb->mode is NGDB_MODE_CREATE, writes
 * out the header section before closing.
 */
uint8_t ngdb_close(ngdb_t *ngdb) {

  if (ngdb      == NULL) goto fail;
  if (ngdb->fid == NULL) goto fail;

  if (ngdb->mode == NGDB_MODE_CREATE) {

    if (_ngdb_write_header(ngdb) != 0) goto fail;
  }

  fclose(ngdb->fid);
  ngdb->fid = NULL;
  free(ngdb);

  return 0;
fail:

  if (ngdb != NULL) free(ngdb);
  return 1;
}

/**
 * \return ngdb->hdata_len
 */
uint16_t ngdb_hdr_data_len(ngdb_t *ngdb) {

  return ngdb->hdata_len;
}

/**
 * \return ngdb->ndata_len
 */
uint16_t ngdb_node_data_len(ngdb_t *ngdb) {

  return ngdb->ndata_len;
}

/**
 * \return ngdb->rdata_len
 */
uint16_t ngdb_ref_data_len(ngdb_t *ngdb) {

  return ngdb->rdata_len;
}

/**
 * \return ngdb->num_nodes
 */
uint32_t ngdb_num_nodes(ngdb_t *ngdb) {

  return ngdb->num_nodes;
}

/**
 * \return ngdb->num_refs
 */
uint32_t ngdb_num_refs(ngdb_t *ngdb) {

  return ngdb->num_refs;
}

/**
 * Looks up the number of references for the given node.
 */
uint32_t ngdb_node_num_refs(ngdb_t *ngdb, uint32_t idx) {

  node_t node;
  node.data = NULL;

  if (ngdb      == NULL)            goto fail;
  if (ngdb->fid == NULL)            goto fail;
  if (idx       >= ngdb->num_nodes) goto fail;

  if (_ngdb_read_node(ngdb, idx, &node) != 0) goto fail;

  return node.num_refs;

fail:
  return 0xFFFFFFFF;
}

uint32_t ngdb_node_get_ref(ngdb_t *ngdb, uint32_t nidx, uint32_t ridx) {

  node_t node;
  ref_t  ref;
  node.data = NULL;
  ref.data  = NULL;

  if (ngdb      == NULL) goto fail;
  if (ngdb->fid == NULL) goto fail;

  if (_ngdb_read_node(ngdb, nidx, &node) != 0) goto fail;

  if (ridx >= node.num_refs) goto fail;

  /*no refs to look up, or invalid first ref*/
  if (node.num_refs  == 0) goto fail;
  if (node.first_ref == 0) goto fail;

  /*step through the list until we 
    reach the requested reference*/
  if (_ngdb_read_ref(ngdb, node.first_ref, &ref) != 0) goto fail;
  while (ref.next != 0 && ridx > 0) {

    if (_ngdb_read_ref(ngdb, ref.next, &ref) != 0) goto fail;
    ridx--;
  }

  /*ref list is less than ridx - this shouldn't 
    happen for a valid file, as it will be caught 
    in the ridx >= node.num_refs test above*/
  if (ridx > 0) goto fail;

  return ref.idx;

fail:
  return 0xFFFFFFFF;
}

uint8_t ngdb_node_get_all_refs(ngdb_t *ngdb, uint32_t idx, uint32_t *refs) {

  node_t node;
  ref_t  ref;
  node.data = NULL;
  ref.data  = NULL;

  if (ngdb      == NULL)            goto fail;
  if (ngdb->fid == NULL)            goto fail;
  if (refs      == NULL)            goto fail;
  if (idx       >= ngdb->num_nodes) goto fail;

  if (_ngdb_read_node(ngdb, idx, &node) != 0) goto fail;

  /*no refs to look up, or invalid first ref*/
  if (node.num_refs  == 0) return 0;
  if (node.first_ref == 0) goto fail;

  ref.next = node.first_ref;
  do {

    if (_ngdb_read_ref(ngdb, ref.next, &ref) != 0) goto fail;
    memcpy(refs++, &(ref.idx), sizeof(ref.idx));
  } while (ref.next != 0);

  return 0;

fail:
  return 1;
}

/**
 * Copies the header data section into the data.
 */
uint8_t ngdb_hdr_get_data(ngdb_t *ngdb, uint8_t *data) {

  if (ngdb            == NULL) goto fail;
  if (ngdb-> fid      == NULL) goto fail;
  if (data            == NULL) goto fail;
  if (ngdb->hdata_len == 0)    return 1;

  if (_ngdb_read_hdr_data(ngdb, data) != 0) goto fail;

  return 0;

fail:
  return 2;
}

/**
 * Copies the data section of the given node into data.
 */
uint8_t ngdb_node_get_data(ngdb_t *ngdb, uint32_t idx, uint8_t *data) {

  node_t node;

  if (ngdb      == NULL)            goto fail;
  if (ngdb->fid == NULL)            goto fail;
  if (data      == NULL)            goto fail;
  if (idx       >= ngdb->num_nodes) goto fail;

  /*there is no data in this graph*/
  if (ngdb->ndata_len == 0) return 1;

  node.data = data;

  if (_ngdb_read_node(ngdb, idx, &node) != 0) goto fail;

  return 0;

fail:
  return 2;
}

uint8_t ngdb_ref_get_data(
ngdb_t *ngdb, uint32_t nidx, uint32_t ridx, uint8_t *data) {

  node_t node;
  ref_t  ref;

  if (ngdb            == NULL)            goto fail;
  if (ngdb->fid       == NULL)            goto fail; 
  if (data            == NULL)            goto fail; 
  if (nidx            >= ngdb->num_nodes) goto fail;
  if (ngdb->rdata_len == 0)               return 1;

  node.data = NULL;
  node.dlen = 0;

  if (_ngdb_read_node(ngdb, nidx, &node) != 0) goto fail;

  if (ridx >= node.num_refs) goto fail;
  if (node.num_refs == 0)    goto fail;

  ref.data = data;
  ref.dlen = ngdb->rdata_len;

  if (_ngdb_read_ref(ngdb, node.first_ref, &ref) != 0) goto fail;
  while (ref.next != 0 && ridx > 0) {

    if (_ngdb_read_ref(ngdb, ref.next, &ref) != 0) goto fail;
    ridx--;
  }
  if (ridx > 0) goto fail;

  return 0;
fail:
  return 2;
}

/*******************
 *ngdb file creation
 ******************/

ngdb_t * ngdb_create(
char *filename, uint32_t num_nodes, 
uint16_t hdata_len, uint16_t ndata_len, uint16_t rdata_len) {

  ngdb_t *ngdb;
  uint32_t i;
  node_t node;

  ngdb = NULL;

  if (filename == NULL) goto fail;

  ngdb = malloc(sizeof(ngdb_t));
  if (ngdb == NULL) goto fail;

  ngdb->fid = fopen(filename, "wb");
  if (ngdb->fid == NULL) goto fail;

  ngdb->hdata_len  = hdata_len;
  ngdb->ndata_len  = ndata_len;
  ngdb->rdata_len  = rdata_len;
  ngdb->num_nodes  = num_nodes;
  ngdb->num_refs   = 0;
  ngdb->mode       = NGDB_MODE_CREATE;

  for (i = 0; i < num_nodes; i++) {

    node.num_refs  = 0;
    node.first_ref = 0;
    node.last_ref  = 0;
    node.data      = NULL;
    node.dlen      = 0;
    node.idx       = i;
    if (_ngdb_write_node(ngdb, &node) != 0) goto fail;
  }
  if (fflush(ngdb->fid) != 0) goto fail;

  return ngdb;

fail:

  if (ngdb != NULL) {

    if (ngdb->fid != NULL) {
      fclose(ngdb->fid);
      remove(filename);
    }
    free(ngdb);
  }
 
  return NULL;
}

uint32_t ngdb_add_ref(
ngdb_t *ngdb, uint32_t idx, uint32_t refidx, uint8_t *data, uint16_t dlen) {

  node_t   node;
  ref_t    ref;
  uint32_t raddr;

  if (ngdb       == NULL)             goto fail;
  if (ngdb->fid  == NULL)             goto fail;
  if (idx        >= ngdb->num_nodes)  goto fail;
  if (refidx     >= ngdb->num_nodes)  goto fail;
  if (ngdb->mode != NGDB_MODE_CREATE) goto fail;

  /*
   * write new reference to end of file
   * read in 'idx' node
   * add new ref to idx node's ref list
   * update idx node's ref count
   */
  raddr = _ngdb_new_ref_addr(ngdb);

  ref.idx   = refidx;
  ref.next  = 0;
  ref.addr  = raddr;
  ref.dlen  = dlen;
  ref.data  = data;
  node.data = NULL;
  node.dlen = 0;

  if (_ngdb_write_ref(ngdb, &ref)              != 0) goto fail; 
  if (_ngdb_read_node(ngdb, idx, &node)        != 0) goto fail;
  if (_ngdb_add_to_ref_list(ngdb, &node, &ref) != 0) goto fail;

  ngdb->num_refs++;

  return node.num_refs-1;

fail:
  return 0xFFFFFFFF;
}

uint8_t ngdb_hdr_set_data(ngdb_t *ngdb, uint8_t *data, uint16_t dlen) {

  if ( ngdb            == NULL)             goto fail;
  if ( ngdb->fid       == NULL)             goto fail;
  if ( ngdb->mode      != NGDB_MODE_CREATE) goto fail;
  if ( ngdb->hdata_len == 0)                goto fail;
  if ( dlen            != 0 
    && data            == NULL)             goto fail;

  if (_ngdb_write_hdr_data(ngdb, data, dlen) != 0) goto fail;

  return 0;

fail:
  return 1;
}

uint8_t ngdb_node_set_data(
ngdb_t *ngdb, uint32_t idx, uint8_t *data, uint16_t dlen) {

  node_t node;
  node.data = NULL;

  if (ngdb              == NULL)             goto fail;
  if (ngdb->fid         == NULL)             goto fail;
  if (ngdb->mode        != NGDB_MODE_CREATE) goto fail;
  if (dlen != 0 && data == NULL)             goto fail;
  if (dlen              >  ngdb->ndata_len)  goto fail;
  if (idx               >= ngdb->num_nodes)  goto fail;

  if (_ngdb_read_node(ngdb, idx, &node) != 0) goto fail;

  node.data = data;
  node.dlen = dlen;

  if (_ngdb_write_node(ngdb, &node) != 0) goto fail;

  return 0;
fail:
  return 1;
}

/*******************
 * Private functions
 ******************/

uint8_t _ngdb_read_header(ngdb_t *ngdb) {

  uint16_t id;

  rewind(ngdb->fid);

  /*check file id*/
  if (fread(&id, sizeof(id), 1, ngdb->fid) != 1) goto fail;
  if (id != NGDB_FILE_ID)                        goto fail;

  /*read in header*/
  if (fread(&(ngdb->hdata_len),  sizeof(ngdb->hdata_len), 1, ngdb->fid) != 1)
    goto fail;
  if (fread(&(ngdb->ndata_len),  sizeof(ngdb->ndata_len), 1, ngdb->fid) != 1)
    goto fail;
  if (fread(&(ngdb->rdata_len),  sizeof(ngdb->rdata_len), 1, ngdb->fid) != 1)
    goto fail;
  if (fread(&(ngdb->num_nodes),  sizeof(ngdb->num_nodes), 1, ngdb->fid) != 1)
    goto fail;
  if (fread(&(ngdb->num_refs),   sizeof(ngdb->num_refs),  1, ngdb->fid) != 1)
    goto fail;

  return 0;

fail:
  return 1;
}

uint8_t _ngdb_write_header(ngdb_t *ngdb) {

  uint16_t id;

  rewind(ngdb->fid);

  id = NGDB_FILE_ID;
  if (fwrite(&(id),              sizeof(id),              1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(ngdb->hdata_len), sizeof(ngdb->hdata_len), 1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(ngdb->ndata_len), sizeof(ngdb->ndata_len), 1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(ngdb->rdata_len), sizeof(ngdb->rdata_len), 1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(ngdb->num_nodes), sizeof(ngdb->num_nodes), 1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(ngdb->num_refs),  sizeof(ngdb->num_refs),  1, ngdb->fid) != 1)
    goto fail;
  
  return 0;
fail:
  return 1;
}

static uint8_t _ngdb_read_hdr_data(ngdb_t *ngdb, uint8_t *data) {

  if (fseeko(ngdb->fid, 16, SEEK_SET)            != 0) goto fail;
  if (fread(data, ngdb->hdata_len, 1, ngdb->fid) != 1) goto fail;

  return 0;
fail:
  return 1;
}

static uint8_t _ngdb_write_hdr_data(
ngdb_t *ngdb, uint8_t *data, uint16_t dlen) {

  uint8_t tmp;
  tmp = 0;

  if (fseeko(ngdb->fid, 16, SEEK_SET)  != 0) goto fail;
  if (fwrite(data, dlen, 1, ngdb->fid) != 1) goto fail;

  for (; dlen < ngdb->hdata_len; dlen++) 
    if (fwrite(&tmp, 1, 1, ngdb->fid) != 1) goto fail;

  return 0;
fail:
  return 1;
}

uint8_t _ngdb_read_node(ngdb_t *ngdb, uint32_t idx, node_t *node) {

  uint16_t sync;

  /*seek to the node*/
  if (fseeko(ngdb->fid, _ngdb_idx_to_addr(ngdb, idx), SEEK_SET) != 0)
    goto fail;

  /*check node sync bytes*/

  if (fread(&sync, sizeof(sync), 1, ngdb->fid) != 1) goto fail;
  if (sync != NGDB_NODE_SYNC)                        goto fail;

  /*read in node section*/
  if (fread(&(node->num_refs),  sizeof(node->num_refs),  1, ngdb->fid) != 1)
    goto fail;
  if (fread(&(node->first_ref), sizeof(node->first_ref), 1, ngdb->fid) != 1)
    goto fail;
  if (fread(&(node->last_ref),  sizeof(node->last_ref),  1, ngdb->fid) != 1)
    goto fail;

  /*only read in data if an array has been provided*/
  if ( node->data != NULL) {

    if (fread(node->data, ngdb->ndata_len, 1, ngdb->fid) != 1) goto fail;
    node->dlen = ngdb->ndata_len;
  }
  else node->dlen = 0;

  node->idx = idx;

  return 0;

fail:
  return 1;
}

uint8_t _ngdb_write_node(ngdb_t *ngdb, node_t *node) {

  uint16_t sync;
  uint16_t dlen;
  uint8_t  tmp;

  dlen = node->dlen;
  tmp  = 0;

  if (fseeko(ngdb->fid, _ngdb_idx_to_addr(ngdb, node->idx), SEEK_SET) != 0)
    goto fail;

  sync = NGDB_NODE_SYNC;
  if (fwrite(&sync,              sizeof(sync),            1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(node->num_refs),  sizeof(node->num_refs),  1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(node->first_ref), sizeof(node->first_ref), 1, ngdb->fid) != 1)
    goto fail;
  if (fwrite(&(node->last_ref),  sizeof(node->last_ref),  1, ngdb->fid) != 1)
    goto fail;

  if (dlen != 0) {

    /*write the provided data*/
    if (fwrite(node->data, dlen, 1, ngdb->fid) != 1) goto fail;

    /*zero fill the remainder*/
    for (; dlen < ngdb->ndata_len; dlen++)
      if (fwrite(&tmp, 1, 1, ngdb->fid) != 1) goto fail;
  }

  return 0;
fail: 
  return 1;
}

uint8_t _ngdb_read_ref(ngdb_t *ngdb, uint32_t addr, ref_t *ref) {

  uint16_t sync;

  /*seek to the ref*/
  if (fseeko(ngdb->fid, addr, SEEK_SET) != 0) goto fail;
 
  /*check the ref sync*/
  if (fread(&sync, sizeof(sync), 1, ngdb->fid) != 1) goto fail;
  if (sync != NGDB_REF_SYNC)                         goto fail;

  /*read in the ref info*/
  if (fread(&(ref->idx),  sizeof(ref->idx),  1, ngdb->fid) != 1) goto fail;
  if (fread(&(ref->next), sizeof(ref->next), 1, ngdb->fid) != 1) goto fail;

  /*only read in data if an array has been provided*/
  if (ref->data != NULL) {

    if (fread(ref->data, ngdb->rdata_len, 1, ngdb->fid) != 1) goto fail;
    ref->dlen = ngdb->rdata_len;
  }
  else ref->dlen = 0;

  ref->addr = addr;

  return 0;

fail:
  return 1;

}

uint8_t _ngdb_write_ref(ngdb_t *ngdb, ref_t *ref) {

  uint16_t sync;
  uint16_t dlen;
  uint8_t  tmp;

  dlen = ref->dlen;
  tmp  = 0;

  if (fseeko(ngdb->fid, ref->addr, SEEK_SET) != 0) goto fail;

  sync = NGDB_REF_SYNC;
  if (fwrite(&(sync),      sizeof(sync),      1, ngdb->fid) != 1) goto fail;
  if (fwrite(&(ref->idx),  sizeof(ref->idx),  1, ngdb->fid) != 1) goto fail;
  if (fwrite(&(ref->next), sizeof(ref->next), 1, ngdb->fid) != 1) goto fail;

  if (dlen != 0) {

    /*write the provided data*/
    if (fwrite(ref->data, dlen, 1, ngdb->fid) != 1) goto fail;

    /*zero fill the remainder*/
    for (; dlen < ngdb->rdata_len; dlen++)
      if (fwrite(&tmp, 1, 1, ngdb->fid) != 1) goto fail;
  }

  return 0;

fail:
  return 1;
}

uint8_t _ngdb_add_to_ref_list(ngdb_t *ngdb, node_t *node, ref_t *ref) {

  ref_t tmp;

  tmp.data = NULL;
  tmp.dlen = 0;

  /*first reference for this node - add address to node section*/
  if (node->first_ref == 0) {

    node->first_ref = ref->addr;
    if (_ngdb_write_node(ngdb, node) != 0) goto fail;
  }

  /*otherwise add new ref address to end of list*/
  else {

    if (_ngdb_read_ref(ngdb, node->last_ref, &tmp) != 0) goto fail;

    tmp.next = ref->addr;

    if (_ngdb_write_ref(ngdb, &tmp) != 0) goto fail;
  }

  node->num_refs++;
  node->last_ref = ref->addr;

  if (_ngdb_write_node(ngdb, node) != 0) goto fail;

  return 0;
fail:
  return 1;
}

uint32_t _ngdb_idx_to_addr(ngdb_t *ngdb, uint32_t idx) {

  return (16 + ngdb->hdata_len) + idx*(14 + ngdb->ndata_len);
}

uint32_t _ngdb_new_ref_addr(ngdb_t *ngdb) {

  return _ngdb_idx_to_addr(ngdb, ngdb->num_nodes) + 
         (ngdb->num_refs)*(10 + ngdb->rdata_len);
}
