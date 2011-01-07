/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "VG/openvg.h"

#include "mask.h"
#include "api.h"
#include "renderer.h"

#include "vg_context.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"

#include "util/u_pack_color.h"
#include "util/u_draw_quad.h"

void vegaMask(VGHandle mask, VGMaskOperation operation,
              VGint x, VGint y,
              VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();

   if (width <=0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (operation < VG_CLEAR_MASK || operation > VG_SUBTRACT_MASK) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }


   vg_validate_state(ctx);

   if (operation == VG_CLEAR_MASK) {
      mask_fill(x, y, width, height, 0.f);
   } else if (operation == VG_FILL_MASK) {
      mask_fill(x, y, width, height, 1.f);
   } else if (vg_object_is_valid((void*)mask, VG_OBJECT_IMAGE)) {
      struct vg_image *image = (struct vg_image *)mask;
      mask_using_image(image, operation, x, y, width, height);
   } else if (vg_object_is_valid((void*)mask, VG_OBJECT_MASK)) {
      struct vg_mask_layer *layer = (struct vg_mask_layer *)mask;
      mask_using_layer(layer, operation, x, y, width, height);
   } else {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
   }
}

void vegaClear(VGint x, VGint y,
               VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct st_framebuffer *stfb = ctx->draw_buffer;

   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);
#if 0
   debug_printf("Clear [%d, %d, %d, %d] with [%f, %f, %f, %f]\n",
                x, y, width, height,
                ctx->state.vg.clear_color[0],
                ctx->state.vg.clear_color[1],
                ctx->state.vg.clear_color[2],
                ctx->state.vg.clear_color[3]);
#endif

   /* check for a whole surface clear */
   if (!ctx->state.vg.scissoring &&
       (x == 0 && y == 0 && width == stfb->width && height == stfb->height)) {
      ctx->pipe->clear(ctx->pipe, PIPE_CLEAR_COLOR | PIPE_CLEAR_DEPTHSTENCIL,
                       ctx->state.vg.clear_color, 1., 0);
   } else if (renderer_clear_begin(ctx->renderer)) {
      /* XXX verify coord round-off */
      renderer_clear(ctx->renderer, x, y, width, height, ctx->state.vg.clear_color);
      renderer_clear_end(ctx->renderer);
   }
}


#ifdef OPENVG_VERSION_1_1


void vegaRenderToMask(VGPath path,
                      VGbitfield paintModes,
                      VGMaskOperation operation)
{
   struct vg_context *ctx = vg_current_context();

   if (path == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!paintModes || (paintModes&(~(VG_STROKE_PATH|VG_FILL_PATH)))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (operation < VG_CLEAR_MASK ||
       operation > VG_SUBTRACT_MASK) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)path, VG_OBJECT_PATH)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   vg_validate_state(ctx);

   mask_render_to((struct path *)path, paintModes, operation);
}

VGMaskLayer vegaCreateMaskLayer(VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();

   if (width <= 0 || height <= 0 ||
       width > vgGeti(VG_MAX_IMAGE_WIDTH) ||
       height > vgGeti(VG_MAX_IMAGE_HEIGHT)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   return (VGMaskLayer)mask_layer_create(width, height);
}

void vegaDestroyMaskLayer(VGMaskLayer maskLayer)
{
   struct vg_mask_layer *mask = 0;
   struct vg_context *ctx = vg_current_context();

   if (maskLayer == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)maskLayer, VG_OBJECT_MASK)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   mask = (struct vg_mask_layer *)maskLayer;
   mask_layer_destroy(mask);
}

void vegaFillMaskLayer(VGMaskLayer maskLayer,
                       VGint x, VGint y,
                       VGint width, VGint height,
                       VGfloat value)
{
   struct vg_mask_layer *mask = 0;
   struct vg_context *ctx = vg_current_context();

   if (maskLayer == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (value < 0 || value > 1) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (x < 0 || y < 0 || (x + width) < 0 || (y + height) < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (!vg_object_is_valid((void*)maskLayer, VG_OBJECT_MASK)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   mask = (struct vg_mask_layer*)maskLayer;

   if (x + width > mask_layer_width(mask) ||
       y + height > mask_layer_height(mask)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);

   mask_layer_fill(mask, x, y, width, height, value);
}

void vegaCopyMask(VGMaskLayer maskLayer,
                  VGint sx, VGint sy,
                  VGint dx, VGint dy,
                  VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_mask_layer *mask = 0;

   if (maskLayer == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)maskLayer, VG_OBJECT_MASK)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   vg_validate_state(ctx);

   mask = (struct vg_mask_layer*)maskLayer;
   mask_copy(mask, sx, sy, dx, dy, width, height);
}

#endif
