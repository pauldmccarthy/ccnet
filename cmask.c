/**
 * Mask a ngdb graph file using values from an ANALYZE75 image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com> 
 */
#include <argp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "graph/graph.h"
#include "util/startup.h"
#include "io/ngdb_graph.h"
#include "io/analyze75.h"

/**
 * The masking operators that are available.
 */
typedef enum {
  CMASK_EQ,  /**< Equal to                 */
  CMASK_NEQ, /**< Not equal to             */
  CMASK_GT,  /**< Greater than             */
  CMASK_GE,  /**< Greater than or equal to */
  CMASK_LT,  /**< Less than                */
  CMASK_LE   /**< Less than or equal to    */
} op_t;

typedef struct _args {

  char   *input;
  char   *maskf;
  char   *output;
  op_t    op;
  uint8_t real;

} args_t;

int main(int argc, char *argv[]) {

}
