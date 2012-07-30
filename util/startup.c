/**
 * Little function which programs call when they start. Parses options,
 * prints out some stuff, seeds the random number generator.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <argp.h>

#include "util/startup.h"

static struct argp_option options[] = {
  {"seed", 0x5EED, "INT", 0, "seed for random number generator"},
  {0}
};

struct args {

  int64_t seed;
  void   *child_input;
};

static error_t _parse_opt(int key, char *arg, struct argp_state *state) {

  struct args *args = state->input;

  switch (key) {

    case ARGP_KEY_INIT:
      state->child_inputs[0] = args->child_input;
      break;

    case 0x5EED:
      args->seed = atoi(arg);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

void startup(
  char        *progname,
  int          argc,
  char        *argv[],
  struct argp *child_argp,
  void        *child_input) {

  unsigned int      i;
  struct timeval    t;
  struct argp       my_argp;
  struct args       my_input;
  struct argp_child children[] = {
    {child_argp, 0, "", 0},
    {0}
  };

  printf("%s revision: %s", progname, GIT_REVISION);
  printf(" (");
  for (i = 0; i < argc; i++) {
    if (i < argc-1) printf("%s ", argv[i]);
    else            printf("%s",  argv[i]);
  }
  printf(")\n");

  memset(&my_argp, 0, sizeof(struct argp));
  my_argp.options  = options;
  my_argp.parser   = _parse_opt;
  my_argp.children = children;

  my_input.seed        = -1;
  my_input.child_input = child_input;

  if (child_argp != NULL && child_input != NULL)
    argp_parse(&my_argp, argc, argv, 0, 0, &my_input);

  gettimeofday(&t, NULL);
  if (my_input.seed == -1) my_input.seed = t.tv_usec;

  srand(my_input.seed);
}
