/**
 * Resample an ANALYZE75 image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <argp.h>

#include "io/analyze75.h"

static char doc[] = "sampleimg -- resample ANALYZE75 3D image files";

static struct argp_option options[] = {
  {"newx", 'x', "INT", 0, "new X dimension size"},
  {"newy", 'y', "INT", 0, "new Y dimension size"},
  {"newz", 'z', "INT", 0, "new Z dimension size"},
  {"avg",  'a', NULL,  0, "average input voxels"},
  {0}
};

struct arguments {

  char   *input;
  char   *output;
  uint8_t newx;
  uint8_t newy;
  uint8_t newz;
  uint8_t avg;
};

/**
 * argp callback function; parses a single argument value.
 */
static error_t _parse_opt (
  int                key, 
  char              *arg, 
  struct argp_state *state
);

static void _resample_hdr(
  dsr_t  *inhdr,
  dsr_t  *outhdr,
  uint8_t newx,
  uint8_t newy,
  uint8_t newz
);

static void _resample_img(
  dsr_t   *inhdr, 
  dsr_t   *outhdr, 
  uint8_t *inimg, 
  uint8_t *outimg, 
  uint8_t  newx,
  uint8_t  newy,
  uint8_t  newz,
  uint8_t  avg
);

static double _resample_voxel_centre(
  dsr_t   *inhdr,
  dsr_t   *outhdr,
  uint8_t *inimg,
  uint32_t newx,
  uint32_t newy,
  uint32_t newz
);

static double _resample_voxel_average(
  dsr_t   *inhdr,
  dsr_t   *outhdr,
  uint8_t *inimg,
  uint32_t newx,
  uint32_t newy,
  uint32_t newz
);

int main(int argc, char *argv[]) {

  dsr_t            inhdr;
  dsr_t            outhdr;
  uint8_t         *inimg;
  uint8_t         *outimg;
  uint32_t         nbytes;
  struct arguments arguments;
  struct argp      argp = {options, _parse_opt, "INPUT OUTPUT", doc};

  inimg  = NULL;
  outimg = NULL;

  arguments.newx = 0;
  arguments.newy = 0;
  arguments.newz = 0;
  arguments.avg  = 0;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (analyze_load(arguments.input, &inhdr, &inimg)) {
    printf("error loading %s\n", arguments.input);
    goto fail;
  }

  if (arguments.newx == 0) arguments.newx = analyze_dim_size(&inhdr, 0);
  if (arguments.newy == 0) arguments.newy = analyze_dim_size(&inhdr, 1);
  if (arguments.newz == 0) arguments.newz = analyze_dim_size(&inhdr, 2);

  _resample_hdr(&inhdr, &outhdr,
    arguments.newx, arguments.newy, arguments.newz);

  nbytes = analyze_value_size(&outhdr)*analyze_num_vals(&outhdr);
  outimg = (uint8_t *)malloc(nbytes*sizeof(uint8_t));
  if (outimg == NULL) {
    printf("out of memory ?! (%u)\n", nbytes);
    goto fail;
  }

  _resample_img(
    &inhdr, &outhdr, inimg, outimg,
    arguments.newx, arguments.newy, arguments.newz, arguments.avg);

  if (analyze_write_hdr(arguments.output, &outhdr)) {
    printf("error writing header %s\n", arguments.output);
    goto fail;
  }

  if (analyze_write_img(arguments.output, &outhdr, outimg)) {
    printf("error writing image %s\n", arguments.output);
    goto fail;
  }

  free(inimg);
  free(outimg);
  return 0;

 fail:
  if (inimg  != NULL) free(inimg);
  if (outimg != NULL) free(outimg);
  return 1;
}

error_t _parse_opt (int key, char *arg, struct argp_state *state) {

  struct arguments *arguments = state->input;

  switch (key) {

    case 'x':
      arguments->newx = atoi(arg);
      break;

    case 'y':
      arguments->newy = atoi(arg);
      break;

    case 'z':
      arguments->newz = atoi(arg);
      break;

    case 'a':
      arguments->avg = 0xFF;
      break;

    case ARGP_KEY_ARG:
      if      (state->arg_num == 0) arguments->input  = arg;
      else if (state->arg_num == 1) arguments->output = arg;
      else                          argp_usage(state);
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 2) argp_usage(state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

void _resample_hdr(
  dsr_t  *inhdr,
  dsr_t  *outhdr,
  uint8_t newx,
  uint8_t newy,
  uint8_t newz)
{

  memcpy(outhdr, inhdr, sizeof(dsr_t));

  outhdr->dime.dim[1] = newx;
  outhdr->dime.dim[2] = newy;
  outhdr->dime.dim[3] = newz;

  outhdr->dime.pixdim[1] *= (float)(inhdr->dime.dim[1])/newx;
  outhdr->dime.pixdim[2] *= (float)(inhdr->dime.dim[2])/newy;
  outhdr->dime.pixdim[3] *= (float)(inhdr->dime.dim[3])/newz;
}

void _resample_img(
  dsr_t   *inhdr, 
  dsr_t   *outhdr, 
  uint8_t *inimg, 
  uint8_t *outimg, 
  uint8_t  newx,
  uint8_t  newy,
  uint8_t  newz,
  uint8_t  avg)
{
  uint32_t newi[4];

  double val;

  memset(newi, 0, sizeof(newi));

  for (newi[0] = 0; newi[0] < newx; newi[0]++) {
    for (newi[1] = 0; newi[1] < newy; newi[1]++) {
      for (newi[2] = 0; newi[2] < newz; newi[2]++) {

        if (!avg) 
          val = _resample_voxel_centre(
            inhdr,outhdr,inimg,newi[0],newi[1],newi[2]);
          
        else 
          val = _resample_voxel_average(
            inhdr,outhdr,inimg,newi[0],newi[1],newi[2]);
        
        analyze_write_val(outhdr, outimg, newi, val);
      }
    }
  }
}

double _resample_voxel_centre(
  dsr_t   *inhdr,
  dsr_t   *outhdr,
  uint8_t *inimg,
  uint32_t newxi,
  uint32_t newyi,
  uint32_t newzi
) {

  uint32_t oldi[4];
  float    oldxf;
  float    oldyf;
  float    oldzf;
  float    newxf;
  float    newyf;
  float    newzf; 
  float    xfi;
  float    yfi;
  float    zfi;

  memset(oldi, 0, sizeof(oldi));

  oldxf = analyze_pixdim_size(inhdr,  0);
  oldyf = analyze_pixdim_size(inhdr,  1);
  oldzf = analyze_pixdim_size(inhdr,  2); 
  newxf = analyze_pixdim_size(outhdr, 0);
  newyf = analyze_pixdim_size(outhdr, 1);
  newzf = analyze_pixdim_size(outhdr, 2);

  /*centre of current voxel in real-world units*/
  xfi = (newxi*newxf) + newxf/2.0;
  yfi = (newyi*newyf) + newyf/2.0;
  zfi = (newzi*newzf) + newzf/2.0;

  /*indices for nearest voxel in old image*/
  oldi[0] = (uint32_t)floor(xfi/oldxf);
  oldi[1] = (uint32_t)floor(yfi/oldyf);
  oldi[2] = (uint32_t)floor(zfi/oldzf);

  return analyze_read_val(inhdr, inimg, oldi);
}

double _resample_voxel_average(
  dsr_t   *inhdr,
  dsr_t   *outhdr,
  uint8_t *inimg,
  uint32_t newxi,
  uint32_t newyi,
  uint32_t newzi
) {

  double   val;
  uint32_t vcount;
  uint32_t oldi[4];

  float oldx;
  float oldy;
  float oldz;
  float oldxf;
  float oldyf;
  float oldzf;
  float newxf;
  float newyf;
  float newzf;

  uint32_t oldxilo;
  uint32_t oldyilo;
  uint32_t oldzilo;
  uint32_t oldxihi;
  uint32_t oldyihi;
  uint32_t oldzihi;

  oldx  = analyze_dim_size(   inhdr,  0);
  oldy  = analyze_dim_size(   inhdr,  1);
  oldz  = analyze_dim_size(   inhdr,  2);
  oldxf = analyze_pixdim_size(inhdr,  0);
  oldyf = analyze_pixdim_size(inhdr,  1);
  oldzf = analyze_pixdim_size(inhdr,  2);
  newxf = analyze_pixdim_size(outhdr, 0);
  newyf = analyze_pixdim_size(outhdr, 1);
  newzf = analyze_pixdim_size(outhdr, 2); 

  val    = 0;
  vcount = 0;

  memset(oldi, 0, sizeof(oldi));

  oldxilo = round(( newxi   *newxf)/oldxf);
  oldyilo = round(( newyi   *newyf)/oldyf);
  oldzilo = round(( newzi   *newzf)/oldzf);
  oldxihi = round(((newxi+1)*newxf)/oldxf);
  oldyihi = round(((newyi+1)*newyf)/oldyf);
  oldzihi = round(((newzi+1)*newzf)/oldzf);


  for (oldi[0] = oldxilo; oldi[0] < oldxihi; oldi[0]++) {
    for (oldi[1] = oldyilo; oldi[1] < oldyihi; oldi[1]++) {
      for (oldi[2] = oldzilo; oldi[2] < oldzihi; oldi[2]++) {

        val += analyze_read_val(inhdr, inimg, oldi);
        vcount ++;
      }
    }
  }
  
  val /= vcount;
  return val;
}
