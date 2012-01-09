/**
 * Little function which programs call when they start.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <argp.h>

void startup(
  char        *progname,
  int          argc,
  char        *argv[],
  struct argp *argp,
  void        *argp_input
);

#endif /* __STARTUP_H__ */
