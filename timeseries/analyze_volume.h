/**
 * Functions for reading a collection of 3D ANALYZE75 images.
 *
 * Author: Paul McCarthy <pauld.mccarthy@gmail.com>
 */
#ifndef __ANALYZE_VOLUME_H__
#define __ANALYZE_VOLUME_H__

#include <stdint.h>

#include "io/analyze75.h"

typedef struct __analyze_volume {
  
  uint16_t  nimgs; /**< number of images in the volume */
  char    **files; /**< image file names               */
  dsr_t    *hdrs;  /**< image headers                  */
  uint8_t **imgs;  /**< image data                     */
  
} analyze_volume_t;

/**
 * Opens the volume in the specified directory for reading.
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t analyze_open_volume(
  char             *path, /**< directory containing volume images */
  analyze_volume_t *vol   /**< pointer to an uninitialised
                               analyze_volume_t struct            */
);

/**
 * Frees the memory used by the given volume.
 */
void analyze_free_volume(
  analyze_volume_t *vol /**< volume to be freed */
);

/**
 * Reads the time series data for the specified voxel.
 * 
 * \return 0 on success, non-0 on failure.
 */
uint8_t analyze_read_timeseries(
  analyze_volume_t *vol,       /**< volume to query                  */
  uint32_t          x,         /**< x voxel index                    */
  uint32_t          y,         /**< y voxel index                    */
  uint32_t          z,         /**< z voxel index                    */
  double           *timeseries /**< space in which to store the data */
);

/**
 * Reads the time series data for the specified voxel. The idx value is
 * converted to a (x,y,z) coordinate (order of fastest to slowest changing
 * dimension is [x,y,z]).
 *
 * \return 0 on success, non-0 on failure.
 */
uint8_t analyze_read_timeseries_by_idx(
  analyze_volume_t *vol,       /**< volume to query                  */
  uint32_t          idx,       /**< voxel index                      */
  double           *timeseries /**< space in which to store the data */  
);
  
#endif
