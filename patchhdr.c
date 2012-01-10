/**
 * Program which can be used to modify the fields of an ANALYZE75 header
 * file.
 * 
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */ 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "io/analyze75.h"

/**
 * Updates the given header so that the specified field has the new value.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _patch(
  dsr_t *hdr,   /**< header to update    */
  char  *field, /**< name of field       */
  char  *newval /**< new value for field */
);

/**
 * Copies at most maxlen characters from the source to the destination.
 */
static void _copy_string(
  char    *dst,   /**< destination string                   */
  char    *src,   /**< source string                        */
  uint32_t maxlen /**< maximum number of characters to copy */
);

int main(int argc, char *argv[]) {

  dsr_t    hdr;
  uint64_t i;

  if (argc < 4 || argc % 2) {
    printf("usage: patchhdr file field newvalue [field2 newvalue2 [...]]\n");
    goto fail;
  }

  if (analyze_load_hdr(argv[1], &hdr)) {
    printf("error reading header (%i)\n", errno);
    goto fail;
  }

  for (i = 2; i < argc; i+=2) {

    if (_patch(&hdr, argv[i], argv[i+1])) {
      printf("error patching header (%s: %s)\n", argv[i], argv[i+1]);
      goto fail;
    }
  }

  if (analyze_write_hdr(argv[1], &hdr)) {
    printf("error writing %s\n", argv[1]);
    goto fail;
  }

  return 0;
  
fail:
  return 1;
}

uint8_t _patch(dsr_t *hdr, char *field, char *newval) {

  int   inewval;
  float fnewval;

  inewval = atoi(newval);
  fnewval = atof(newval);

  /* header key fields are prefixed with 'hk.' */
  if      (!strcmp(field, "hk.sizeof_hdr")) hdr->hk.sizeof_hdr = inewval;
  
  else if (!strcmp(field, "hk.data_type"))
    _copy_string(hdr->hk.data_type, newval, 10);
  
  else if (!strcmp(field, "hk.db_name"))
    _copy_string(hdr->hk.db_name, newval, 18);
  
  else if (!strcmp(field, "hk.extents"))
    hdr->hk.extents = inewval;
  
  else if (!strcmp(field, "hk.session_error"))
    hdr->hk.session_error = inewval;
      
  else if (!strcmp(field, "hk.regular"))
    hdr->hk.regular = newval[0];
  
  else if (!strcmp(field, "hk.hkey_un0"))
    hdr->hk.hkey_un0 = newval[0];

  /* image dimension fields are prefixed with 'dime.'*/
  else if (!strcmp(field, "dime.dim.0")) hdr->dime.dim[0] = inewval;
  else if (!strcmp(field, "dime.dim.1")) hdr->dime.dim[1] = inewval;
  else if (!strcmp(field, "dime.dim.2")) hdr->dime.dim[2] = inewval;
  else if (!strcmp(field, "dime.dim.3")) hdr->dime.dim[3] = inewval;
  else if (!strcmp(field, "dime.dim.4")) hdr->dime.dim[4] = inewval;
  else if (!strcmp(field, "dime.dim.5")) hdr->dime.dim[5] = inewval;
  else if (!strcmp(field, "dime.dim.6")) hdr->dime.dim[6] = inewval;
  else if (!strcmp(field, "dime.dim.7")) hdr->dime.dim[7] = inewval;
  
  else if (!strcmp(field, "dime.unused8"))  hdr->dime.unused8  = inewval;
  else if (!strcmp(field, "dime.unused9"))  hdr->dime.unused9  = inewval;
  else if (!strcmp(field, "dime.unused10")) hdr->dime.unused10 = inewval;
  else if (!strcmp(field, "dime.unused11")) hdr->dime.unused11 = inewval;
  else if (!strcmp(field, "dime.unused12")) hdr->dime.unused12 = inewval;
  else if (!strcmp(field, "dime.unused13")) hdr->dime.unused13 = inewval;
  else if (!strcmp(field, "dime.unused14")) hdr->dime.unused14 = inewval;
  else if (!strcmp(field, "dime.datatype")) hdr->dime.datatype = inewval;
  
  else if (!strcmp(field, "dime.bitpix")) hdr->dime.bitpix = inewval;
  
  else if (!strcmp(field, "dime.dim_un0")) hdr->dime.dim_un0 = inewval;
  
  else if (!strcmp(field, "dime.pixdim.0")) hdr->dime.pixdim[0] = fnewval;
  else if (!strcmp(field, "dime.pixdim.1")) hdr->dime.pixdim[1] = fnewval;
  else if (!strcmp(field, "dime.pixdim.2")) hdr->dime.pixdim[2] = fnewval;
  else if (!strcmp(field, "dime.pixdim.3")) hdr->dime.pixdim[3] = fnewval;
  else if (!strcmp(field, "dime.pixdim.4")) hdr->dime.pixdim[4] = fnewval;
  else if (!strcmp(field, "dime.pixdim.5")) hdr->dime.pixdim[5] = fnewval;
  else if (!strcmp(field, "dime.pixdim.6")) hdr->dime.pixdim[6] = fnewval;
  else if (!strcmp(field, "dime.pixdim.7")) hdr->dime.pixdim[7] = fnewval;
  
  else if (!strcmp(field, "dime.vox_offset")) hdr->dime.vox_offset = fnewval;
  else if (!strcmp(field, "dime.funused1"))   hdr->dime.funused1   = fnewval;
  else if (!strcmp(field, "dime.funused2"))   hdr->dime.funused2   = fnewval;
  else if (!strcmp(field, "dime.funused3"))   hdr->dime.funused3   = fnewval;
  else if (!strcmp(field, "dime.cal_max"))    hdr->dime.cal_max    = fnewval;
  else if (!strcmp(field, "dime.cal_min"))    hdr->dime.cal_min    = fnewval;
  else if (!strcmp(field, "dime.compressed")) hdr->dime.compressed = fnewval;
  else if (!strcmp(field, "dime.verified"))   hdr->dime.verified   = fnewval;
  else if (!strcmp(field, "dime.glmax"))      hdr->dime.glmax      = inewval;
  else if (!strcmp(field, "dime.glmin"))      hdr->dime.glmin      = inewval;

  /* data history fields are prefixed with 'hist.' */
  else if (!strcmp(field, "hist.descrip"))
    _copy_string(hdr->hist.descrip, newval, 80);
  
  else if (!strcmp(field, "hist.aux_file"))
    _copy_string(hdr->hist.aux_file, newval, 24);
  
  else if (!strcmp(field, "hist.orient"))
    hdr->hist.orient = newval[0];
  
  else if (!strcmp(field, "hist.originator"))
    _copy_string(hdr->hist.originator, newval, 10);
  
  else if (!strcmp(field, "hist.generated"))
    _copy_string(hdr->hist.generated, newval, 10);
  
  else if (!strcmp(field, "hist.scannum"))
    _copy_string(hdr->hist.scannum, newval, 10);
  
  else if (!strcmp(field, "hist.patient_id"))
    _copy_string(hdr->hist.patient_id, newval, 10);
  
  else if (!strcmp(field, "hist.exp_date"))
    _copy_string(hdr->hist.exp_date, newval, 10);
  
  else if (!strcmp(field, "hist.exp_time"))
    _copy_string(hdr->hist.exp_time, newval, 10);
  
  else if (!strcmp(field, "hist.hist_un0"))
    _copy_string(hdr->hist.hist_un0, newval, 3);
  
  else if (!strcmp(field, "hist.views"))       hdr->hist.views       = inewval;
  else if (!strcmp(field, "hist.vols_added"))  hdr->hist.vols_added  = inewval;
  else if (!strcmp(field, "hist.start_field")) hdr->hist.start_field = inewval;
  else if (!strcmp(field, "hist.field_skip"))  hdr->hist.field_skip  = inewval;
  else if (!strcmp(field, "hist.omax"))        hdr->hist.omax        = inewval;
  else if (!strcmp(field, "hist.omin"))        hdr->hist.omin        = inewval;
  else if (!strcmp(field, "hist.smax"))        hdr->hist.smax        = inewval;
  else if (!strcmp(field, "hist.smin"))        hdr->hist.smin        = inewval;

  else return 1;

  return 0;
}

void _copy_string(char *dst, char *src, uint32_t maxlen) {

  int nchars;
  int srclen;

  srclen = strlen(src);
  nchars = (srclen > maxlen) ? maxlen : srclen;

  memcpy(dst, src, nchars);
  
  dst[nchars-1] = '\0';
}
