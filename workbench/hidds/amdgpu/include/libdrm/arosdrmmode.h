/*
 * Copyright (c) 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2007-2008 Dave Airlie <airlied@linux.ie>
 * Copyright (c) 2007-2008 Jakob Bornecrantz <wallbraker@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _AROSDRMMODE_H_
#define _AROSDRMMODE_H_

#include <uapi/drm/drm.h>

typedef struct _drmModeRes {

	int count_fbs;
	uint32_t *fbs;

	int count_crtcs;
	uint32_t *crtcs;

	int count_connectors;
	uint32_t *connectors;

	int count_encoders;
	uint32_t *encoders;

	uint32_t min_width, max_width;
	uint32_t min_height, max_height;
} drmModeRes, *drmModeResPtr;

typedef struct _drmModeModeInfo {
    uint32_t clock;
    uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew;
    uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan;

    uint32_t vrefresh; /* vertical refresh * 1000 */

    uint32_t flags;
    uint32_t type;
    char name[DRM_DISPLAY_MODE_LEN];
} drmModeModeInfo, *drmModeModeInfoPtr;

typedef struct _drmModeEncoder {
	uint32_t encoder_id;
	uint32_t encoder_type;
	uint32_t crtc_id;
	uint32_t possible_crtcs;
	uint32_t possible_clones;
} drmModeEncoder, *drmModeEncoderPtr;

typedef enum {
	DRM_MODE_CONNECTED         = 1,
	DRM_MODE_DISCONNECTED      = 2,
	DRM_MODE_UNKNOWNCONNECTION = 3
} drmModeConnection;

typedef enum {
	DRM_MODE_SUBPIXEL_UNKNOWN        = 1,
	DRM_MODE_SUBPIXEL_HORIZONTAL_RGB = 2,
	DRM_MODE_SUBPIXEL_HORIZONTAL_BGR = 3,
	DRM_MODE_SUBPIXEL_VERTICAL_RGB   = 4,
	DRM_MODE_SUBPIXEL_VERTICAL_BGR   = 5,
	DRM_MODE_SUBPIXEL_NONE           = 6
} drmModeSubPixel;

typedef struct _drmModeConnector {
	uint32_t connector_id;
	uint32_t encoder_id; /**< Encoder currently connected to */
	uint32_t connector_type;
	uint32_t connector_type_id;
	drmModeConnection connection;
	uint32_t mmWidth, mmHeight; /**< HxW in millimeters */
	drmModeSubPixel subpixel;

	int count_modes;
	drmModeModeInfoPtr modes;

	int count_props;
	uint32_t *props; /**< List of property ids */
	uint64_t *prop_values; /**< List of property values */

	int count_encoders;
	uint32_t *encoders; /**< List of encoder ids */
} drmModeConnector, *drmModeConnectorPtr;

typedef struct _drmModeCrtc {
	uint32_t crtc_id;
	uint32_t buffer_id; /**< FB id to connect to 0 = disconnect */

	uint32_t x, y; /**< Position on the framebuffer */
	uint32_t width, height;
	int mode_valid;
	drmModeModeInfo mode;

	int gamma_size; /**< Number of gamma stops */

} drmModeCrtc, *drmModeCrtcPtr;

/* Add passed bo (handle) to list of frame buffers */
extern int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
            uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
            uint32_t *buf_id);
extern int drmModeRmFB(int fd, uint32_t bufferId);

/* Gets information about all resources of card:
    framebuffers
    crtcs
    connectors
    encoders */
extern drmModeResPtr drmModeGetResources(int fd);
extern void drmModeFreeResources(drmModeResPtr ptr);

/* Connector support */
extern drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connectorId);
extern void drmModeFreeConnector(drmModeConnectorPtr ptr);

/* CRTC support */ /* bufferId == id of nouveau_bo registered using drmModeAddFB */
extern int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId,
            uint32_t x, uint32_t y, uint32_t *connectors, int count,
            drmModeModeInfoPtr mode);
extern drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId);
extern void drmModeFreeCrtc( drmModeCrtcPtr ptr );

/* Encoder support */
extern drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id);
extern void drmModeFreeEncoder(drmModeEncoderPtr ptr);

/* Cursor support */
extern int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle, 
            uint32_t width, uint32_t height);
extern int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y);

/* Dumb (CPU rendered) buffers */
extern int drmModeCreateDumb(int fd, uint32_t width, uint32_t height, uint32_t bpp,
            uint32_t *handle, uint32_t *pitch, uint64_t *size);
extern int drmModeDestroyDumb(int fd, uint32_t handle);

/* Front buffer flushing */
typedef struct _drmModeClip {
	uint16_t x1, y1;
	uint16_t x2, y2;
} drmModeClip, *drmModeClipPtr;
extern int drmModeDirtyFB(int fd, uint32_t bufferId, drmModeClipPtr clips, uint32_t num_clips);

/* Cursor with hotspot */
extern int drmModeSetCursor2(int fd, uint32_t crtcId, uint32_t bo_handle,
            uint32_t width, uint32_t height, int32_t hot_x, int32_t hot_y);


/*
 * Planes, page flips and vblank waits (AROS additions for the display
 * compositor's scan-out planes; same shapes as libdrm's).
 */
typedef struct _drmModePlaneRes {
	uint32_t count_planes;
	uint32_t *planes;
} drmModePlaneRes, *drmModePlaneResPtr;

typedef struct _drmModePlane {
	uint32_t count_formats;
	uint32_t *formats;
	uint32_t plane_id;
	uint32_t crtc_id;
	uint32_t fb_id;
	uint32_t crtc_x, crtc_y;
	uint32_t x, y;
	uint32_t possible_crtcs;
	uint32_t gamma_size;
} drmModePlane, *drmModePlanePtr;

extern drmModePlaneResPtr drmModeGetPlaneResources(int fd);
extern void drmModeFreePlaneResources(drmModePlaneResPtr ptr);
extern drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id);
extern void drmModeFreePlane(drmModePlanePtr ptr);
extern int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id, uint32_t fb_id, uint32_t flags,
			   int32_t crtc_x, int32_t crtc_y, uint32_t crtc_w, uint32_t crtc_h,
			   uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h);
extern int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags, void *user_data);

typedef enum {
	DRM_VBLANK_ABSOLUTE = 0x0,
	DRM_VBLANK_RELATIVE = 0x1,
	DRM_VBLANK_HIGH_CRTC_MASK = 0x0000003e,
	DRM_VBLANK_EVENT = 0x4000000,
	DRM_VBLANK_FLIP = 0x8000000,
	DRM_VBLANK_NEXTONMISS = 0x10000000,
	DRM_VBLANK_SECONDARY = 0x20000000,
	DRM_VBLANK_SIGNAL = 0x40000000
} drmVBlankSeqType;
#define DRM_VBLANK_HIGH_CRTC_SHIFT 1

typedef struct _drmVBlankReq {
	drmVBlankSeqType type;
	unsigned int sequence;
	unsigned long signal;
} drmVBlankReq;

typedef struct _drmVBlankReply {
	drmVBlankSeqType type;
	unsigned int sequence;
	long tval_sec;
	long tval_usec;
} drmVBlankReply;

typedef union _drmVBlank {
	drmVBlankReq request;
	drmVBlankReply reply;
} drmVBlank, *drmVBlankPtr;

extern int drmWaitVBlank(int fd, drmVBlankPtr vbl);

#endif /* _AROSDRMMODE_H_ */
