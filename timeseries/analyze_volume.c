/**
 * Functions for reading a collection of 3D ANALYZE75 images, or a single 4D
 * image.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include "io/analyze75.h"
#include "util/suffix.h"
#include "util/compare.h"
#include "timeseries/analyze_volume.h"

/**
 * Converts a 4D ANALYZE75 image to an analyze_volume_t struct.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _4d_to_volume(
  char             *imgfile, /**< 4D image file name           */
  analyze_volume_t *vol      /**< empty volume to be populated */
);

/**
 * Converts a collection of 3D ANALYZE75 image files to an analyze_volume_t
 * struct.
 *
 * \return 0 on success, non-0 on failure.
 */
static uint8_t _3d_to_volume(
  char            **imgfiles, /**< list of image file names     */
  uint16_t          nfiles,   /**< number of files              */
  analyze_volume_t *vol       /**< empty volume to be populated */
);

/**
 * Lists all of the .img files in the specified path.
 * Memory is allocated to store all the files, and the
 * given list pointer updated to point to said memory.
 * The list is ordered numerically by the file name
 * prefixes.
 *
 * \return the number of .img files in the specified path,
 * -1 on failure.
 */
static int32_t _list_files(
  char   *path, /**< directory to search       */
  char ***list  /**< place to store file names */
);

/**
 * Used by the _list_files function.
 *
 * \return 1 if the given dirent has '.img' as a suffix, 0 otherwise.
 */
static int _select_img_files(
  const struct dirent *entry /**< entry to check */
);

/**
 * Compares the prefixes of two filenames numerically.
 * Used by the _list_files function.
 */
static int _cmp_filenames(
  const struct dirent **a, /**< pointer to a dirent struct */
  const struct dirent **b  /**< pointer to a dirent struct */
);

uint8_t analyze_open_volume(char *path, analyze_volume_t *vol) {

  uint32_t i;
  char   **files;
  int32_t  nfiles;

  nfiles = 0;
  files  = NULL;
  memset(vol, 0, sizeof(analyze_volume_t));

  nfiles = _list_files(path, &files);
  if (nfiles <= 0) goto fail;

  if (nfiles == 1) {
    
    if (_4d_to_volume(files[0], vol)) goto fail;
    free(files[0]);
    free(files);
  }
  else {
    if (_3d_to_volume(files, nfiles, vol)) goto fail;
  }

  return 0;

fail:
  if (files != NULL) {
    for (i = 0; i < nfiles; i++) {
      if (files[i] != NULL)
        free(files[i]);
    }
    free(files);
  } 
  return 1;
}

void analyze_free_volume(analyze_volume_t *vol) {

  uint32_t i;

  if (vol        == NULL) return;
  if (vol->hdrs  == NULL) return;
  if (vol->imgs  == NULL) return;
  if (vol->files == NULL) return;
  if (vol->nimgs == 0)    return;

  for (i = 0; i < vol->nimgs; i++) {
    free(vol->imgs[i]);
    free(vol->files[i]);
  }

  free(vol->hdrs);
  free(vol->imgs);
  free(vol->files);
}


uint8_t analyze_read_timeseries(
  analyze_volume_t *vol,
  uint32_t          x,
  uint32_t          y,
  uint32_t          z,
  double           *timeseries) {

  uint64_t i;
  uint32_t idx[4];
  
  idx[0] = x;
  idx[1] = y;
  idx[2] = z;
  idx[3] = 0;

  for (i = 0; i < vol->nimgs; i++) {
    timeseries[i] = analyze_read_val(&(vol->hdrs[i]), vol->imgs[i], idx);
  }

  return 0;
}

uint8_t analyze_read_timeseries_by_idx(
  analyze_volume_t *vol, uint32_t idx, double *timeseries) {

  uint32_t dims[5];

  analyze_get_indices(vol->hdrs, idx, dims);

  return analyze_read_timeseries(vol, dims[0], dims[1], dims[2], timeseries);
}

int32_t _list_files(char *path, char ***list) {

  uint32_t        i;
  int32_t         nentries;
  struct dirent **entries;
  struct stat     buf;
  char          **llist;

  llist    = NULL;  
  entries  = NULL;
  nentries = 0;

  if (stat(path, &buf)) goto fail;

  /* the path points to a file, not a directory */
  if (S_ISREG(buf.st_mode)) {

    llist = calloc(1, sizeof(char *));
    if (llist == NULL) goto fail;

    llist[0] = malloc(strlen(path)+1);
    if (llist[0] == NULL) goto fail;
    
    strcpy(llist[0], path);
    nentries = 1;
  }

  /* the path is a directory */
  else if (S_ISDIR(buf.st_mode)) {
  
    nentries = scandir(path, &entries, _select_img_files, _cmp_filenames);

    if (nentries == 0) goto fail;

    llist = calloc(nentries, sizeof(char *));
    if (llist == NULL) goto fail;

    for (i = 0; i < nentries; i++) {

      llist[i] = join_path(path, entries[i]->d_name);
      if (llist[i] == NULL) goto fail;
    }

    for (i = 0; i < nentries; i++) free(entries[i]);
    free(entries);
  }

  /* path is neither a file nor a directory */
  else goto fail;

  *list = llist;                              
  return nentries;
  
fail:

  if (llist != NULL) {
    for (i = 0; i < nentries; i++) {
      if (llist[i] != NULL)
        free(llist[i]);
    }
    free(llist);
  }

  if (entries != NULL) {
    for (i = 0; i < nentries; i++) {
      if (entries[i] != NULL) 
        free(entries[i]);
    }
  }
  free(entries); 
  
  return -1;
}

int _select_img_files(const struct dirent *entry) {

  int   select;
  char *fname;
  char *suff;

  suff  = NULL;
  fname = (char *)entry->d_name;

  suff = malloc(strlen(fname)+1);
  if (suff == NULL) goto fail;

  get_suffix(fname, suff);

  if (!strcmp(suff, "img")) select = 1;
  else                      select = 0;

  free(suff);
  return select;

fail:
  if (suff != NULL) free(suff);
  return 0;
}

int _cmp_filenames(const struct dirent **a, const struct dirent **b) {

  int   cmp;
  char *ca;
  char *cb;

  char *capref;
  char *cbpref;

  capref = NULL;
  cbpref = NULL;
  ca     = (char *)((*a)->d_name);
  cb     = (char *)((*b)->d_name);

  capref = malloc(strlen(ca)+1);
  if (capref == NULL) goto fail;
  
  cbpref = malloc(strlen(cb)+1);
  if (cbpref == NULL) goto fail;

  get_prefix(ca, capref);
  get_prefix(cb, cbpref);

  cmp = compare_str_numeric(capref, cbpref);

  free(capref);
  free(cbpref);
  return cmp;
  
fail:
  if (capref != NULL) free(capref);
  if (cbpref != NULL) free(cbpref);
  return 0;
}


uint8_t _4d_to_volume(char *imgfile, analyze_volume_t *vol) {

  uint32_t i;
  uint32_t imgsz;
  dsr_t    volhdr;
  uint8_t *volimg;
  uint32_t imgoff;
  uint32_t dimidxs[4];

  volimg = NULL;
  memset(vol,     0, sizeof(analyze_volume_t));
  memset(dimidxs, 0, sizeof(dimidxs));

  if (analyze_load(imgfile, &volhdr, &volimg)) goto fail;
  if (analyze_num_dims(&volhdr) != 4)          goto fail;

  vol->nimgs = analyze_dim_size(&volhdr, 3);

  /*
   * figure out the size of a single 3D
   * image, as contained in the 4D volume
   */
  imgsz  = analyze_num_vals(&volhdr);
  imgsz /= vol->nimgs;
  imgsz *= analyze_value_size(&volhdr);

  vol->files = calloc(vol->nimgs, sizeof(char *));
  if (vol->files == NULL) goto fail;

  vol->hdrs = calloc(vol->nimgs, sizeof(dsr_t));
  if (vol->hdrs == NULL) goto fail;
  
  vol->imgs = calloc(vol->nimgs, sizeof(uint8_t *));
  if (vol->imgs == NULL) goto fail;

  for (i = 0; i < vol->nimgs; i++) {

    vol->imgs[i] = malloc(imgsz);
    if (vol->imgs[i] == NULL) goto fail;

    vol->files[i] = malloc(strlen(imgfile)+1);
    if (vol->files[i] == NULL) goto fail;
    
    strcpy(vol->files[i], imgfile);
    memcpy(vol->hdrs+i, &volhdr, sizeof(dsr_t));

    /* hack the 4D volume header so it looks 3D */
    vol->hdrs[i].dime.dim   [0] = 3;
    vol->hdrs[i].dime.dim   [4] = 1;
    vol->hdrs[i].dime.pixdim[4] = 0;

    /*copy the portion of the volume at the current time step */
    dimidxs[3] = i;
    imgoff = analyze_get_offset(&volhdr, dimidxs);
    memcpy(vol->imgs[i], volimg+imgoff, imgsz);
  }
  
  free(volimg);
  
  return 0;
fail:

  if (volimg != NULL) free(volimg);
  
  if (vol->hdrs != NULL) free(vol->hdrs);
  
  if (vol->files != NULL) {
    for (i = 0; i < vol->nimgs; i++) {
      if (vol->files[i] != NULL)
        free(vol->files[i]);
    }
    free(vol->files); 
  }

  if (vol->imgs != NULL) {
    for (i = 0; i < vol->nimgs; i++) {
      if (vol->imgs[i] != NULL)
        free(vol->imgs[i]);
    }
    free(vol->imgs);
  } 

  return 1;
}

uint8_t _3d_to_volume(
  char **imgfiles, uint16_t nfiles, analyze_volume_t *vol) {

  uint32_t i;

  memset(vol, 0, sizeof(analyze_volume_t));  

  vol->files = imgfiles;
  vol->nimgs = nfiles;
  vol->hdrs  = calloc(vol->nimgs, sizeof(dsr_t));
  if (vol->hdrs == NULL) goto fail;

  vol->imgs = calloc(vol->nimgs, sizeof(uint8_t *));
  if (vol->imgs == NULL) goto fail;

  for (i = 0; i < vol->nimgs; i++) {
    if (analyze_load(vol->files[i], (vol->hdrs)+i, (vol->imgs)+i)) 
      goto fail;
  }
  
  if (analyze_hdr_compat(vol->nimgs, vol->hdrs, 0)) goto fail;
  if (analyze_num_dims(&(vol->hdrs[0])) != 3)       goto fail;
    
  return 0;

fail:

  if (vol->hdrs != NULL) free(vol->hdrs);
  if (vol->imgs != NULL) {
    for (i = 0; i < vol->nimgs; i++) {
      if (vol->imgs[i] != NULL)
        free(vol->imgs[i]);
    }
    free(vol->imgs);
  }
  return 1;  
}
