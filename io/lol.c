/**
 * Read in Radatools lol files, and convert to node_partition_t struct.
 * 
 * See http://deim.urv.cat/~sgomez/radatools.php
 *
 * The parsing logic expects something like the following, and will probably
 * break on anything else:
 *
 * ---------
 * Parameters: UN e 10
 * Q = 0.257235
 * ---
 * Number of elements: 128
 * Number of lists: 5
 * 
 * 30: 5 19 33 34 36 37 39 40 41 42 44 46 49 50 51 52 53 54 55 56 57 58 60 61 62 63 97 109 112 115
 * 28: 64 98 99 101 102 103 104 105 106 107 108 110 111 113 114 116 117 118 119 120 121 122 123 124 125 126 127 128
 * 28: 1 3 4 6 7 8 9 10 11 12 13 14 15 17 18 20 21 22 23 24 25 26 27 28 29 30 48 100
 * 24: 65 66 67 70 71 72 73 74 75 76 77 79 80 82 83 85 86 87 89 91 93 94 95 96
 * 18: 2 16 31 32 35 38 43 45 47 59 68 69 78 81 84 88 90 92
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util/getline.h"
#include "graph/graph.h"
#include "util/array.h"
#include "util/compare.h"
#include "io/lol.h"

/**
 * Reads the header section of the lolfile; specifically, reads the number of
 * elements, and nunbmer of partitions, and initialises the fields of the
 * node_partition_t struct.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_hdr(
  FILE             *f,  /**< open file handle to the lolfile             */
  node_partition_t *lol /**< pointer to an empty node_partition_t struct */
);

/**
 * Reads the list of partitions from the lolfile, adding th node IDs to the 
 * partition arrays in the given node_partition_t struct.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_parts(
  FILE             *f,  /**< open file handle to the lolfile, pointing 
                             to the start of the partition list        */
  node_partition_t *lol /**< pointer to a node_partition_t struct,
                             with all fields initialised               */
);

/**
 * Reads one partition line, storing the node IDs in the given array.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _read_part(
  char    *partline, /**< string containing the partition description */
  array_t *part      /**< array to put the node IDs                   */
);


uint8_t lol_load(char *fname, node_partition_t *lol) {

  FILE    *f;
  uint64_t i;

  f          = NULL;
  lol->parts = NULL;

  f = fopen(fname, "rt");
  if (f == NULL) goto fail;

  if (_read_hdr(f, lol)) goto fail;

  lol->parts = calloc(lol->nparts, sizeof(array_t));
  if (lol->parts == NULL) goto fail;

  for (i = 0; i < lol->nparts; i++) {
    if (array_create(&lol->parts[i], sizeof(uint32_t), 10)) 
      goto fail;
    array_set_cmps(&lol->parts[i], compare_u32, compare_u32_insert);
  }

  if (_read_parts(f, lol)) goto fail;

  fclose(f);

  return 0;

fail:

  if (f != NULL) fclose(f);

  if (lol->parts != NULL) {

    for (i = 0; i < lol->nparts; i++) {

      if (lol->parts[i].data != NULL) 
        array_free(&lol->parts[i]);
    }

    free(lol->parts);
  }
  return 1;
}

static uint8_t _read_hdr(FILE *f, node_partition_t *lol) {

  char   *line;
  size_t  len;

  line        = NULL;
  len         = 0;
  lol->nparts = 0xFFFFFFFF;
  lol->nnodes = 0xFFFFFFFF;

  while (1) {

    if (getline(&line, &len, f) < 0) goto fail;

    if (sscanf(line, "Number of elements: %u\n", &(lol->nnodes)) == 1) 
      continue;

    if (sscanf(line, "Number of lists: %u\n", &(lol->nparts)) == 1) 
      break;

    free(line);
    line = NULL;
  }

  if (lol->nparts == 0xFFFFFFFF ||
      lol->nnodes == 0xFFFFFFFF)
    goto fail;

  if (line != NULL) free(line);
  return 0;

fail:
  if (line != NULL) free(line);
  return 1;
}


static uint8_t _read_parts(FILE *f, node_partition_t *lol) {

  char    *line;
  size_t   len;
  uint64_t part;
  uint32_t partsz;
  char    *clnpos;

  line = NULL;
  len  = 0;
  part = 0;

  while (getline(&line, &len, f) > 0)  {

    if (sscanf(line, "%u: ", &partsz) != 1) 
      continue;

    clnpos = strchr(line, ':');
    if (clnpos == NULL) goto fail;

    if (_read_part(clnpos+1, &lol->parts[part])) goto fail;

    part ++;

    free(line);
    line = NULL;
  }

  if (part != lol->nparts) goto fail;

  if (line != NULL) free(line);
  return 0;

fail:
  if (line != NULL) free(line);
  return 1;
}

static uint8_t _read_part(char *partline, array_t *part) {

  char    *tkn;
  char    *saveptr;
  uint32_t nid;

  saveptr = NULL;

  tkn = strtok_r(partline, " ", &saveptr);

  while (tkn != NULL) {

    nid = atoi(tkn) - 1;
    if (array_insert_sorted(part, &nid, 1, NULL)) goto fail;
    tkn = strtok_r(NULL, " ", &saveptr);
  }

  return 0;

fail:
  return 1;
}
