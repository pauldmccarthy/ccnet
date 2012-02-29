/**
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "io/dot.h"
#include "stats/stats.h"
#include "stats/stats_cache.h"
#include "graph/graph.h"
#include "util/getline.h"

/**
 * Writes the given graph to the given file.
 */
static void _write_graph(
  FILE      *hd,      /**< file to write to                           */
  graph_t   *g,       /**< graph to write                             */
  uint16_t   opts,    /**< output options                             */
  uint32_t  *lblvals, /**< label values for label<->colour mapping    */
  char     **colours, /**< colour strings  for label<->colour mapping */
  uint32_t   ncolours /**< number of label<->colour mappings          */
);

/**
 * Writes the specified node.
 */
static void _write_node(
  FILE      *hd,      /**< file to write to                           */
  graph_t   *g,       /**< graph to write                             */
  uint32_t   u,       /**< node to write                              */
  uint16_t   opts,    /**< output options                             */
  uint32_t  *lblvals, /**< label values for label<->colour mapping    */
  char     **colours, /**< colour strings  for label<->colour mapping */
  uint32_t   ncolours /**< number of label<->colour mappings          */
);

/**
 * Writes the edges for the specified node.
 */
static void _write_edges(
  FILE      *hd,      /**< file to write to                           */
  graph_t   *g,       /**< graph to write                             */
  uint32_t   u,       /**< node whose edges are to be written         */
  uint16_t   opts,    /**< output options                             */
  uint32_t  *lblvals, /**< label values for label<->colour mapping    */
  char     **colours, /**< colour strings  for label<->colour mapping */
  uint32_t   ncolours /**< number of label<->colour mappings          */
);

/**
 * Reads a colour map from the given file, specifying node label <-> colour
 * mappings to be used in the dot output. Space is allocated to store the
 * label values and colour strings - the caller is responsible for freeing
 * them.
 *
 * \return the number of label <-> colour mappings read in on success, 0 if
 * file is NULL, -1 on failure.
 */
static int64_t _read_colourmap(
  char       *file,    /**< file to read from                */
  uint32_t  **lblvals, /**< pointer which will be updated to
                            point to list of label values    */
  char     ***colours  /**< pointer which will be updated to
                            point to list of colour strings  */
);

/**
 * Populates the given string with a random RGB colour
 * string. There must be space for 7 characters in the string.
 */
static void _mk_rand_color(
  char *hex, /**< place to store colour string */
  int   seed /**< generator seed               */
);

/**
 * Joins the given tokens with the given character.
 */
static void _join(
  char     *dst,   /**< place to put the joined string -
                        there must be enough room        */
  char    **tkns,  /**< tokens to join                   */
  uint32_t  ntkns, /**< number of tokens                 */
  char      with   /**< character to join tokens with    */
);


uint8_t dot_write(FILE *hd, graph_t *g, char *cmap, uint16_t opts) {

  int64_t    ncolours;
  uint32_t  *lblvals;
  char     **colours;

  ncolours = 0;
  lblvals  = NULL;
  colours  = NULL;

  ncolours = _read_colourmap(cmap, &lblvals, &colours);
  if (ncolours < 0) goto fail;

  stats_cache_init(g);
  stats_num_components(g, 1, NULL, NULL);

  _write_graph(hd, g, opts, lblvals, colours, ncolours);

  free(lblvals);
  free(colours);
  return 0;

fail:
  if (lblvals != NULL) free(lblvals);
  if (colours != NULL) free(colours);
  return 1;
}

void _write_graph(
  FILE      *hd,
  graph_t   *g,
  uint16_t   opts,
  uint32_t  *lblvals,
  char     **colours,
  uint32_t   ncolours) {
  
  uint64_t   i;
  uint32_t   nnodes;

  nnodes = graph_num_nodes(g);

  fprintf(hd, "strict graph cnet {\n");
  fprintf(hd, "graph [outputorder=edgesfirst];\n");
  fprintf(hd, "node [fixedsize=true];\n");
  fprintf(hd, "node [width=0.2];\n");
  fprintf(hd, "node [height=0.2];\n");
  fprintf(hd, "node [penwidth=0.3];\n");
  fprintf(hd, "node [style=filled];\n");
  fprintf(hd, "node [fontsize=6];\n");
  fprintf(hd, "node [fontcolor=\"#33333388\"];\n");
  fprintf(hd, "edge [color=\"#33333344\"];\n");
  fprintf(hd, "edge [penwidth=0.3];\n");

  for (i = 0; i < nnodes; i++)
    _write_node( hd, g, i, opts, lblvals, colours, ncolours);

  if (!((opts >> DOT_OMIT_EDGES) & 1)) {
  for (i = 0; i < nnodes; i++)
    _write_edges(hd, g, i, opts, lblvals, colours, ncolours);
  }

  fprintf(hd, "}\n");
}

void _write_node(
  FILE      *hd,
  graph_t   *g,
  uint32_t   u,
  uint16_t   opts,
  uint32_t  *lblvals,
  char     **colours,
  uint32_t   ncolours) {
  
  uint64_t       i;
  graph_label_t *lbl;
  char           lblstr[50];
  char           posstr[50];
  char           clrstr[50];
  char          *tkns  [3];
  char           atts  [200];
  uint32_t       cmpnum;

  tkns  [0] = lblstr;
  tkns  [1] = posstr;
  tkns  [2] = clrstr;
  atts  [0] = '\0';
  lblstr[0] = '\0';
  posstr[0] = '\0';
  clrstr[0] = '\0';
  
  lbl = graph_get_nodelabel(g, u);

  if (((opts >> DOT_NODE_LABELVAL) & 1) &&
      ((opts >> DOT_NODE_NODEID)   & 1))
    sprintf(lblstr, "label=\"%u:%u\"", u, lbl->labelval);
  
  else if ((opts >> DOT_NODE_LABELVAL) & 1)
    sprintf(lblstr, "label=\"%u\"", lbl->labelval);
  
  else if ((opts >> DOT_NODE_NODEID) & 1) 
    sprintf(lblstr, "label=\"%u\"", u);
  
  else
    sprintf(lblstr, "label=\"\"");
  
  if ((opts >>  DOT_NODE_POS) & 1)
    sprintf(posstr,
            "pos=\"%0.6f,%0.6f,%0.6f\"",
            lbl->xval, lbl->yval, lbl->zval);

  if ((opts >> DOT_CMP_COLOUR) & 1) {

    stats_cache_node_component(g, u, &cmpnum);


    sprintf(clrstr, "fillcolor=\"#");
    _mk_rand_color(clrstr + 12, cmpnum);
    clrstr[18] = '"';
    clrstr[19] = '\0';
  }
  else if ((opts >> DOT_RAND_COLOUR) & 1) {

    sprintf(clrstr, "fillcolor=\"#");
    _mk_rand_color(clrstr + 12, lbl->labelval);
    clrstr[18] = '"';
    clrstr[19] = '\0';

  }
  else if (ncolours > 0) {

    for (i = 0; i < ncolours; i++) {
      if (lblvals[i] == lbl->labelval)
        break;
    }
    if (i < ncolours)
      sprintf(clrstr, "fillcolor=\"#%s\"", colours[i]);
  }

  _join(atts, tkns, 3, ',');

  if (atts[0] == '\0') fprintf(hd, "%u;\n",      u);
  else                 fprintf(hd, "%u [%s];\n", u, atts);
}

void _write_edges(
  FILE      *hd,
  graph_t   *g,
  uint32_t   u,
  uint16_t   opts,
  uint32_t  *lblvals,
  char     **colours,
  uint32_t   ncolours) {

  uint64_t  i;
  uint32_t  nnbrs;
  uint32_t *nbrs;
  float    *wts;

  char  lblstr[50];
  char  wtstr [50];
  char *tkns  [2];
  char  atts  [200];

  tkns  [0] = lblstr;
  tkns  [1] = wtstr;
  atts  [0] = '\0';
  lblstr[0] = '\0';
  wtstr [0] = '\0';

  nnbrs = graph_num_neighbours(g, u);
  nbrs  = graph_get_neighbours(g, u);
  wts   = graph_get_weights(   g, u);

  for (i = 0; i < nnbrs; i++) {

    if ((opts >> DOT_EDGE_LABELS) & 1)
      sprintf(lblstr, "label=%0.4f", wts[i]);
    if ((opts >> DOT_EDGE_WEIGHT) & 1)
      sprintf(lblstr, "penwidth=%0.4f", 0.5+wts[i]*4.5);

    _join(atts, tkns, 2, ',');

    if (atts[0] == '\0') fprintf(hd, "%u -- %u;\n", u, nbrs[i]);
    else                 fprintf(hd, "%u -- %u [%s];\n", u, nbrs[i], atts);
  }
}

int64_t _read_colourmap(char *file, uint32_t **lblvals, char ***colours) {

  char    **lcolours;
  uint32_t *llblvals;
  uint32_t  ncolours;
  uint32_t  cap;
  char     *line;
  size_t    len;
  uint32_t  lblval;
  char      colour[7];
  FILE     *f;

  if (file == NULL) return 0;

  lcolours = NULL;
  llblvals = NULL;
  f        = NULL;
  line     = NULL;
  ncolours = 0;
  cap      = 10;
  
  f = fopen(file, "rt");
  if (f == NULL) goto fail;

  lcolours = malloc(sizeof(char *) * cap);
  if (lcolours == NULL) goto fail;
  
  llblvals = malloc(sizeof(uint32_t) * cap);
  if (llblvals == NULL) goto fail;

  while (getline(&line, &len, f) != -1) {

    sscanf(line, "%u %6c", &lblval, colour);
    
    memcpy(line, colour, 6);
    line[6] = '\0';

    llblvals[ncolours] = lblval;
    lcolours[ncolours] = line;
    
    ncolours++;
    line = NULL;
    
    if (ncolours == cap) {
      
      cap += 10;
      
      lcolours = realloc(lcolours, sizeof(char *)   * cap);
      llblvals = realloc(llblvals, sizeof(uint32_t) * cap);
      
      if (lcolours == NULL) goto fail;
      if (llblvals == NULL) goto fail;
    }
  }

  *lblvals = llblvals;
  *colours = lcolours;
  fclose(f);
  
  return ncolours;

fail:
  if (f        != NULL) fclose(f);
  if (lcolours != NULL) free(lcolours);
  if (llblvals != NULL) free(llblvals);
  
  *lblvals = NULL;
  *colours = NULL;

  return -1;
}

void _mk_rand_color(char *hex, int seed) {

  srand(seed+105);

  uint8_t r;
  uint8_t g;
  uint8_t b;

  r = 80 + (uint8_t)(160*((double)random()/RAND_MAX));
  g = 80 + (uint8_t)(160*((double)random()/RAND_MAX));
  b = 80 + (uint8_t)(160*((double)random()/RAND_MAX));

  sprintf(hex, "%02x%02x%02x", r, g, b);
}

void _join(char *dst, char **tkns, uint32_t ntkns, char with) {

  uint64_t i;
  char    *odst;

  odst = dst;

  for (i = 0; i < ntkns; i++) {
    
    if (!tkns[i] || tkns[i][0] == '\0') continue;

    sprintf(dst, "%s%c", tkns[i], with);
    
    dst += 1 + strlen(tkns[i]);
  }

  odst[strlen(odst)-1] = '\0';
}
