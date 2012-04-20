/**
 * Searches through an ANALYZE75 image for a specific value, and prints the
 * indices of each match.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "io/analyze75.h"
#include "util/startup.h"


typedef struct _args {

  char    *input;
  double   value;
  double   precision;
} args_t;


static char *doc = "searchimg -- search for values in an ANALYZE75 image";


static struct argp_option options[] = {
  {"value",     'v', "FLOAT", 0, "value to find"},
  {"precision", 'p', "FLOAT", 0, "precision of equality check"},
  {0}
};


static error_t _parse_opt(int key, char *arg, struct argp_state *state) {
  
  args_t *args = state->input;

  switch (key) {

    case 'v': args->value     = atof(arg); break;
    case 'p': args->precision = atof(arg); break;

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


static void _search(
  dsr_t   *hdr,
  uint8_t *img,
  double   value,
  double   precision
);

int main(int argc, char *argv[]) {

  dsr_t       hdr;
  uint8_t    *img;
  args_t      args;
  struct argp argp = {options, _parse_opt, "INPUT", doc};

  memset(&args, 0, sizeof(args));

  startup("searchimg", argc, argv, &argp, &args);

  if (analyze_load(args.input, &hdr, &img)) {
    printf("error reading file %s\n", args.input);
    goto fail;
  }

  _search(&hdr, img, args.value, args.precision);


  return 0;
  
fail:
  return 1;
}

void _search(dsr_t *hdr, uint8_t *img, double value, double precision) {

  uint64_t i;
  uint64_t j;
  double   curval;
  uint32_t nvals;
  uint8_t  ndims;
  uint32_t dimidxs[8];

  ndims = analyze_num_dims(hdr);
  nvals = analyze_num_vals(hdr);

  for (i = 0; i < nvals; i++) {

    curval = analyze_read_by_idx(hdr, img, i);
    
    if (abs(curval - value) <= precision) {

      analyze_get_indices(hdr, i, dimidxs);
      for (j = 0; j < ndims; j++) {
        
        printf("%2u", dimidxs[j]);
        if (j < ndims-1) printf(" ");
      }
      
      printf("\n");
    }
  }
}
