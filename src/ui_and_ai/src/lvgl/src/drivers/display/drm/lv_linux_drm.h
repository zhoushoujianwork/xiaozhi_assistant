/**
 * @file lv_linux_drm.h
 *
 */

#ifndef LV_LINUX_DRM_H
#define LV_LINUX_DRM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../../display/lv_display.h"

#if LV_USE_LINUX_DRM

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_display_t * lv_linux_drm_create(int width, int height, bool rotate);

#define dbg(msg, ...)  {}

void lv_linux_drm_set_file(lv_display_t * disp, const char * file);

void lv_linux_drm_init(lv_display_t * disp, int fd, uint32_t conn_id, uint32_t crtc_id, uint32_t plane_id, uint32_t blob_id);

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_LINUX_DRM */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_LINUX_DRM_H */
