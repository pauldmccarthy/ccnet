/**
 * Prints a count of the number of voxels greater than or equal to a given
 * value in an ANALYZE75 image.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <argp.h>

#include "io/analyze75.h"
#include "util/startup.h"

typedef struct _args {

  char    *input;
  double   threshold;
  uint8_t  lessthan;
  uint8_t  absolute;
  uint8_t  printidx;
} args_t;


static char *doc = "countimg -- count values in an ANALYZE75 image "\
                   "greater than or equal to a specified threshold";


static struct argp_option options[] = {
  {"threshold", 't', "FLOAT", 0, "threshold"},
  {"lessthan",  'l',  NULL,   0, "count values <= threshold, rather than >="},
  {"absolute",  'a',  NULL,   0, "Use absolute values for comparison"},
  {"printidx",  'p',  NULL,   0, "Print indices of voxels passing the test"},
  {0}
};


static error_t _parse_opt(int key, char *arg, struct argp_state *state) {
  
  args_t *args = state->input;

  switch (key) {

    case 't': args->threshold = atof(arg); break;
    case 'l': args->lessthan  = 0xFF;      break;
    case 'a': args->absolute  = 0xFF;      break;
    case 'p': args->printidx  = 0xFF;      break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0) args->input = arg;
      else                      argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num != 1) argp_usage(state);
      break;

    default: return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

int main(int argc, char *argv[]) {

  dsr_t       hdr;
  uint8_t    *data;
  double      val;
  uint32_t    count;
  uint32_t    nnormalvals;
  uint32_t    nvals;
  uint64_t    i;
  uint32_t    dims[4];
  args_t      args;
  uint8_t     passed;
  struct argp argp = {options, _parse_opt, "INPUT", doc}; 


  nnormalvals = 0;
  count       = 0;
  data        = NULL;

  memset(&args, 0, sizeof(args));

  startup("countimg", argc, argv, &argp, &args);

  if (analyze_load(args.input, &hdr, &data)) {
    printf("error loading image (%i)\n", errno);
    goto fail;
  }

  nvals = analyze_num_vals(&hdr);
  if (args.absolute)
    args.threshold = abs(args.threshold);

  for (i = 0; i < nvals; i++) {

    passed = 0;
    val    = analyze_read_by_idx(&hdr, data, i);

    if (!isnormal(val))
      continue;

    nnormalvals ++;

    if (args.absolute)
      val = abs(val);

    if (args.lessthan) {
      if (val <= args.threshold) passed = 1;
    }
    else {
      if (val >= args.threshold) passed = 1;
    }

    if (!passed) continue;

    count ++;
    if (args.printidx) {
      analyze_get_indices(&hdr, i, dims);
      printf("%2u %2u %2u: %6.4f\n" , dims[0], dims[1], dims[2], val);
    }
  }

  printf(
    "%u / %u values %s %0.3f\n",
    count,
    nnormalvals,
    args.lessthan ? "below" : "above",
    args.threshold);

  free(data);
  exit(0);

 fail:
  if (data != NULL) free(data);
  exit(1);
}
