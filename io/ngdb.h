#ifndef __NGDB_H__
#define __NGDB_H__

/**
 *   ngdb - Not the Gnu DeBugger (nGraphDataBase)
 *
 * ngdb is a simple file format and API for file-based storage and access of
 * graph based data. See README.NGDB for documentation.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */

#include <stdint.h>

struct __ngdb;
typedef struct __ngdb ngdb_t;

/**
 * Open the given graph for reading, and return a pointer to a ngdb_t struct
 * which has been allocated on the heap. When this pointer is passed to
 * ngdb_close, it will be freed.
 *
 * \return 0 on success, non-0 on failure.
 */
ngdb_t * ngdb_open(
  char *filename /**< name of the ngdb file to open */
);

/**
 * Close the given graph. You must call this function after creating a
 * graph. If you create a graph, and you don't call this function, the file
 * will not be complete.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t ngdb_close(
  ngdb_t *ngdb /**< the graph to close */
);

/**
 * \return the length of the data section in the graph header.
 */
uint16_t ngdb_hdr_data_len(
  ngdb_t *ngdb /**< the graph to query */
);

/**
 * \return the length of the data section of all nodes in the graph.
 */
uint16_t ngdb_node_data_len(
  ngdb_t *ngdb /**< the graph to query */
);

/**
 * \return the length of the data section of all references in the graph.
 */
uint16_t ngdb_ref_data_len(
  ngdb_t *ngdb /**< the graph to query */
);

/**
 * \return the number of nodes contained in the graph.
 */
uint32_t ngdb_num_nodes(
  ngdb_t *ngdb /**< the graph to query */
);

/**
 * \return the number of references contained in the graph. For directed
 * graphs, this is equal to the total number of edges; for undirected graphs,
 * divide by 2 to get the total number of edges.
 */
uint32_t ngdb_num_refs(
  ngdb_t *ngdb /**< the graph to query */
);

/**
 * \return the number of references for the given node, 2^32 on error.
 */
uint32_t ngdb_node_num_refs(
  ngdb_t   *ngdb, /**< the graph to query */
  uint32_t  idx   /**< the node to query  */
);

/**
 * \return the reference at the given index in the given node's reference
 * list, 2^32 on error.. The reference index, like the node indices, is
 * zero-indexed. If you are planning on retrieving all references from a large
 * graph, use ngdb_node_get_all_refs instead, as it is faster.
 */
uint32_t ngdb_node_get_ref(
  ngdb_t   *ngdb, /**< the graph in question */
  uint32_t  nidx, /**< the node in question  */
  uint32_t  ridx  /**< reference index       */
);

/**
 * Get all references for the given node. You must provide a uint32_t array
 * which has enough space to store the number of references returned by
 * ngdb_node_num_refs for this node. If you don't, memory will be corrupted,
 * and it will be your fault.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t ngdb_node_get_all_refs(
  ngdb_t    *ngdb, /**< the graph to query                            */
  uint32_t   idx,  /**< the node to query                             */
  uint32_t  *refs  /**< memory to store ngdb_node_num_refs references */
);

/**
 * Get the data in the header. You must provide a uint8_t array which has
 * enough space to store the number of bytes returned by ngdb_hdr_data_len. If
 * you don't, memory will be corrupted, and it will be your fault.
 *
 * \return 0 on success, 1 if the header has no data (i.e. ngdb_hdr_data_len
 * == 0), some other value on failure.
 */
uint8_t ngdb_hdr_get_data(
  ngdb_t  *ngdb, /**< the graph to query */
  uint8_t *data  /**< memory to store hdr_data_len bytes */
);

/**
 * Get the data for the given node. You must provide a uint8_t array which has
 * enough space to store the number of bytes returned by ngdb_node_data_len.
 * If you don't, memory will be corrupted, and it will be your fault.
 *
 * \return 0 on success, 1 if the graph has no data (i.e. ngdb_node_data_len
 * == 0), some other value on failure.
 */
uint8_t ngdb_node_get_data(
  ngdb_t   *ngdb, /**< the graph to query                  */
  uint32_t  idx,  /**< the node to query                   */
  uint8_t  *data  /**< memory to store node_data_len bytes */
);

/**
 * Get the data for the given reference. You must provide a uint8_t array
 * which has enough space to store the number of bytes returned by
 * ngdb_ref_data_len. If you don't, memory will be corrupted, and it will be
 * your fault.
 *
 * \return 0 on success, 1 if the graph has no data (i.e. ngdb_ref_data_len ==
 * 0), some other value on failure.
 */
uint8_t ngdb_ref_get_data(
  ngdb_t  *ngdb, /**< the graph to query                 */
  uint32_t nidx, /**< the node to query                  */
  uint32_t ridx, /**< the reference to query             */
  uint8_t *data  /**< memory to store ref_data_len bytes */
);

/********************
 * ngdb file creation
 *******************/

/**
 * Create a ngdb file. 
 *
 * \return 0 on success, non-0 on failure.
 */
ngdb_t * ngdb_create(
  char     *filename,  /**< name of the new file          */
  uint32_t  num_nodes, /**< number of nodes in the graph  */
  uint16_t  hdata_len, /**< header data section length    */
  uint16_t  ndata_len, /**< node data section length      */
  uint16_t  rdata_len  /**< reference data section length */
);

/**
 * Add a reference to the given node. You may pass in NULL and 0 for data and
 * dlen respectively if you wish to set them later, via ngdb_ref_set_data.
 *
 * \return the index of the new reference on success, 2^32 on error.
 */
uint32_t ngdb_add_ref(
  ngdb_t   *ngdb,   /**< the graph in question                        */
  uint32_t  idx,    /**< the node to add a reference to               */
  uint32_t  refidx, /**< the reference to add (idx of the other node) */
  uint8_t  *data,   /**< the data                                     */
  uint16_t  dlen    /**< length of the data                           */

);

/**
 * Set the header data.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t ngdb_hdr_set_data(
  ngdb_t  *ngdb, /**< the graph in question */
  uint8_t *data, /**< the data              */
  uint16_t dlen  /**< length of the data    */
);

/**
 * Set the data for the given node. 
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t ngdb_node_set_data(
  ngdb_t   *ngdb, /**< the graph in question        */
  uint32_t  idx,  /**< the node to set the data for */
  uint8_t  *data, /**< the data                     */ 
  uint16_t  dlen  /**< length of the data           */ 
);

#endif /* __NGDB_H__ */
