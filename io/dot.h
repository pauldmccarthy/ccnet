/**
 * Read/write graphviz dot files.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef __DOT_H__
#define __DOT_H__

#include <stdio.h>
#include <stdint.h>

#include "graph/graph.h"

/**
 * Various options to control output.
 */
typedef enum {

  DOT_RAND_COLOUR   = 0, /**< Randomise per-label node colours     */
  DOT_EDGE_LABELS   = 1, /**< Set edge weights as labels           */
  DOT_NODE_POS      = 2, /**< Include node positions               */
  DOT_NODE_LABELVAL = 3, /**< Include node labels in dot labels    */
  DOT_NODE_NODEID   = 4, /**< Include node IDs in dot labels       */
  DOT_CMP_COLOUR    = 5  /**< randomise per-component node colours */

} dot_opts_t;

/**
 * Writes the given graph_t struct as a graphviz dot file. The opts
 * parameter can be used to select the dot_opts_t options - set the
 * corresponding bit to 1 to select the option.
 *
 * The colour map file can be used to specify colours to be used for
 * nodes with a specified label value. The file should contain lines
 * of the format:
 *
 *   label colour
 *
 * where:
 *   label  - the label value (integer), e.g. 181
 *   colour - the colour to use (RGB hex), e.g. a42365
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t dot_write(
  FILE    *hd,   /**< file to write to                          */
  graph_t *g,    /**< graph to write                            */
  char    *cmap, /**< file specifying label <-> colour mappings */
  uint16_t opts  /**< output options                            */
);

#endif
