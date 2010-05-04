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

#include "arosdrm.h"
#include "arosdrmmode.h"

#include <string.h>

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
                uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
                uint32_t *buf_id)
{
    struct drm_mode_fb_cmd f;
    int ret;

    f.width  = width;
    f.height = height;
    f.pitch  = pitch;
    f.bpp    = bpp;
    f.depth  = depth;
    f.handle = bo_handle;

    if ((ret = drmIoctl(fd, DRM_IOCTL_MODE_ADDFB, &f)))
        return ret;

    *buf_id = f.fb_id;
    return 0;
}

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId,
                    uint32_t x, uint32_t y, uint32_t *connectors, int count,
                    drmModeModeInfoPtr mode)
{
    struct drm_mode_crtc crtc;

    crtc.x          = x;
    crtc.y          = y;
    crtc.crtc_id    = crtcId;
    crtc.fb_id      = bufferId;
    crtc.set_connectors_ptr = VOID2U64(connectors);
    crtc.count_connectors = count;
    if (mode) {
      memcpy(&crtc.mode, mode, sizeof(struct drm_mode_modeinfo));
      crtc.mode_valid = 1;
    } else
      crtc.mode_valid = 0;

    return drmIoctl(fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
}

