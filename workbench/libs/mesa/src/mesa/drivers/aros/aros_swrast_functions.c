/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_swrast_functions.c 31763 2009-09-05 13:24:40Z deadwood $
*/

#include "aros_swrast_functions.h"

#define FAST_RASTERIZATION 0 /* Set this to 0 to get default MESA rasterization */

#include <aros/debug.h>
#include "swrast/swrast.h"

#if FAST_RASTERIZATION == 1

#define CHAN_PRODUCT(a, b)  ((GLubyte) (((GLint)(a) * ((GLint)(b) + 1)) >> 8))


#include "swrast/s_context.h"

#include "main/context.h"
#include "main/colormac.h"

/* Fast, low quality, textured, alpha, Z-disabled, GL_MODULATE */

#define NAME aros_simple_textured_triangle
#define INTERP_RGB 1
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   struct gl_renderbuffer *rb = GET_GL_RB_PTR(GET_AROS_CTX_PTR(ctx)->renderbuffer);\
   struct gl_texture_object *obj = ctx->Texture.Unit[0].CurrentTex[TEXTURE_2D_INDEX];	\
   const GLint b = obj->BaseLevel;					\
   const GLfloat twidth = (GLfloat) obj->Image[0][b]->Width;		\
   const GLfloat theight = (GLfloat) obj->Image[0][b]->Height;		\
   const GLint twidth_log2 = obj->Image[0][b]->WidthLog2;		\
   const GLchan *texture = (const GLchan *) obj->Image[0][b]->Data;	\
   const GLint smask = obj->Image[0][b]->Width - 1;			\
   const GLint tmask = obj->Image[0][b]->Height - 1;			\
   if (!texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }

#define RENDER_SPAN( span )						                            \
   GLuint i;								                                \
   ULONG * dp = (ULONG*)(GET_AROS_CTX_PTR(ctx)->renderbuffer->buffer);      \
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		            \
   span.intTex[1] -= FIXED_HALF;					                        \
   dp += (CorrectY(span.y) * rb->Width) + span.x;                           \
   for (i = 0; i < span.end; i++, dp++) {					                \
      GLint s = FixedToInt(span.intTex[0]) & smask;			                \
      GLint t = FixedToInt(span.intTex[1]) & tmask;			                \
      GLint pos = (t << twidth_log2) + s;				                    \
      pos = pos << 2;  /* multiply by 4 (RGBA) */			                \
      if (texture[pos+3]) /* alpha */                                       \
        *dp = TC_ARGB(CHAN_PRODUCT(FixedToChan(span.red), texture[pos]),                                         \
                        CHAN_PRODUCT(FixedToChan(span.green), texture[pos+1]),                                     \
                        CHAN_PRODUCT(FixedToChan(span.blue), texture[pos+2]), 255);                               \
      span.intTex[0] += span.intTexStep[0];				                    \
      span.intTex[1] += span.intTexStep[1];				                    \
      span.red += span.redStep;                                                 \
      span.green += span.greenStep;                                             \
      span.blue += span.blueStep;                                               \
   }									                                    \

#include "s_tritemp.h"

/* Fast, low quality, textured, alpha, Z-enabled, GL_REPLACE */

#define NAME aros_simple_z_textured_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   struct gl_renderbuffer *rb = GET_GL_RB_PTR(GET_AROS_CTX_PTR(ctx)->renderbuffer);\
   struct gl_texture_object *obj = ctx->Texture.Unit[0].CurrentTex[TEXTURE_2D_INDEX];	\
   const GLint b = obj->BaseLevel;					\
   const GLfloat twidth = (GLfloat) obj->Image[0][b]->Width;		\
   const GLfloat theight = (GLfloat) obj->Image[0][b]->Height;		\
   const GLint twidth_log2 = obj->Image[0][b]->WidthLog2;		\
   const GLchan *texture = (const GLchan *) obj->Image[0][b]->Data;	\
   const GLint smask = obj->Image[0][b]->Width - 1;			\
   const GLint tmask = obj->Image[0][b]->Height - 1;			\
   if (!texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }

#define RENDER_SPAN( span )						                                \
   GLuint i;				    				                                \
   ULONG * dp = (ULONG*)(GET_AROS_CTX_PTR(ctx)->renderbuffer->buffer);          \
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		                \
   span.intTex[1] -= FIXED_HALF;					                            \
   dp += (CorrectY(span.y) * rb->Width) + span.x;                               \
   for (i = 0; i < span.end; i++, dp++) {	                    				\
      const GLuint z = FixedToDepth(span.z);	                    			\
      if (z < zRow[i]) {						                                \
         GLint s = FixedToInt(span.intTex[0]) & smask;			                \
         GLint t = FixedToInt(span.intTex[1]) & tmask;			                \
         GLint pos = (t << twidth_log2) + s;				                    \
         pos = pos << 2;  /* multiply by 4 (RGBA)*/			                    \
         if (texture[pos+3]) { /* alpha */                                      \
        *dp = TC_ARGB(texture[pos],                                         \
                        texture[pos+1],                                     \
                        texture[pos+2], 255);                               \
            zRow[i] = z;							                            \
         }                                                                      \
      }									                                        \
      span.intTex[0] += span.intTexStep[0];				                        \
      span.intTex[1] += span.intTexStep[1];				                        \
      span.z += span.zStep;						                                \
   }									                                        \

#include "s_tritemp.h"


/*
 * Render a flat-shaded RGBA triangle (Z-enabled).
 */
#define NAME aros_flat_z_rgba_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define SETUP_CODE				\
   ASSERT(ctx->Texture._EnabledCoordUnits == 0);\
   ASSERT(ctx->Light.ShadeModel==GL_FLAT);	\
   struct gl_renderbuffer *rb = GET_GL_RB_PTR(GET_AROS_CTX_PTR(ctx)->renderbuffer);\
   span.interpMask |= SPAN_RGBA;		\
   span.red = ChanToFixed(v2->color[0]);	\
   span.green = ChanToFixed(v2->color[1]);	\
   span.blue = ChanToFixed(v2->color[2]);	\
   span.alpha = ChanToFixed(v2->color[3]);	\
   span.redStep = 0;				\
   span.greenStep = 0;				\
   span.blueStep = 0;				\
   span.alphaStep = 0;
#define RENDER_SPAN( span )						                                \
   GLuint i;				    				                                \
   ULONG * dp = (ULONG*)(GET_AROS_CTX_PTR(ctx)->renderbuffer->buffer);          \
   dp += (CorrectY(span.y) * rb->Width) + span.x;                               \
   for (i = 0; i < span.end; i++, dp++) {	                    				\
      const GLuint z = FixedToDepth(span.z);	                    			\
      if (z < zRow[i]) {						                                \
         if (span.alpha) { /* alpha */                                          \
            *dp = TC_ARGB(span.red, span.blue, span.green, 255);                \
            zRow[i] = z;							                            \
         }                                                                      \
      }									                                        \
      span.z += span.zStep;						                                \
   }									                                        \

#include "s_tritemp.h"

/*
 * Render a flat-shaded RGBA triangle (Z-disabled).
 */
#define NAME aros_flat_rgba_triangle
#define SETUP_CODE				\
   ASSERT(ctx->Texture._EnabledCoordUnits == 0);\
   ASSERT(ctx->Light.ShadeModel==GL_FLAT);	\
   struct gl_renderbuffer *rb = GET_GL_RB_PTR(GET_AROS_CTX_PTR(ctx)->renderbuffer);\
   span.interpMask |= SPAN_RGBA;		\
   span.red = ChanToFixed(v2->color[0]);	\
   span.green = ChanToFixed(v2->color[1]);	\
   span.blue = ChanToFixed(v2->color[2]);	\
   span.alpha = ChanToFixed(v2->color[3]);	\
   span.redStep = 0;				\
   span.greenStep = 0;				\
   span.blueStep = 0;				\
   span.alphaStep = 0;
#define RENDER_SPAN( span )						                                \
   GLuint i;				    				                                \
   ULONG * dp = (ULONG*)(GET_AROS_CTX_PTR(ctx)->renderbuffer->buffer);          \
   dp += (CorrectY(span.y) * rb->Width) + span.x;                               \
   for (i = 0; i < span.end; i++, dp++) {	                    				\
   if (span.alpha) /* alpha */                                                  \
      *dp = TC_ARGB(span.red, span.blue, span.green, 255);                      \
   }									                                        \

#include "s_tritemp.h"

/*
 * Render a smooth-shaded RGBA triangle (Z-disabled).
 */
#define NAME aros_smooth_rgba_triangle
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define SETUP_CODE				\
   struct gl_renderbuffer *rb = GET_GL_RB_PTR(GET_AROS_CTX_PTR(ctx)->renderbuffer);\
   {						\
      /* texturing must be off */		\
      ASSERT(ctx->Texture._EnabledCoordUnits == 0);	\
      ASSERT(ctx->Light.ShadeModel==GL_SMOOTH);	\
   }
#define RENDER_SPAN( span )						                                \
   GLuint i;				    				                                \
   ULONG * dp = (ULONG*)(GET_AROS_CTX_PTR(ctx)->renderbuffer->buffer);          \
   dp += (CorrectY(span.y) * rb->Width) + span.x;                               \
   for (i = 0; i < span.end; i++, dp++) {	                    				\
      if (FixedToChan(span.alpha)) /* alpha */                                  \
         *dp = TC_ARGB(FixedToChan(span.red), FixedToChan(span.green),          \
                        FixedToChan(span.blue), FixedToChan(span.alpha));       \
      span.red += span.redStep;                                                 \
      span.green += span.greenStep;                                             \
      span.blue += span.blueStep;                                               \
      span.alpha += span.alphaStep;                                             \
   }									                                        \

#include "s_tritemp.h"

/*
 * Render a smooth-shaded RGBA triangle (Z-enabled).
 */
#define NAME aros_smooth_z_rgba_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define SETUP_CODE				\
   struct gl_renderbuffer *rb = GET_GL_RB_PTR(GET_AROS_CTX_PTR(ctx)->renderbuffer);\
   {						\
      /* texturing must be off */		\
      ASSERT(ctx->Texture._EnabledCoordUnits == 0);	\
      ASSERT(ctx->Light.ShadeModel==GL_SMOOTH);	\
   }
#define RENDER_SPAN( span )						                                \
   GLuint i;				    				                                \
   ULONG * dp = (ULONG*)(GET_AROS_CTX_PTR(ctx)->renderbuffer->buffer);          \
   dp += (CorrectY(span.y) * rb->Width) + span.x;                               \
   for (i = 0; i < span.end; i++, dp++) {	                    				\
      const GLuint z = FixedToDepth(span.z);	                    			\
      if (z < zRow[i]) {						                                \
         if (FixedToChan(span.alpha)) { /* alpha */                             \
            *dp = TC_ARGB(FixedToChan(span.red), FixedToChan(span.green),       \
                          FixedToChan(span.blue), FixedToChan(span.alpha));     \
            zRow[i] = z;							                            \
         }                                                                      \
      }                                                                         \
      span.red += span.redStep;                                                 \
      span.green += span.greenStep;                                             \
      span.blue += span.blueStep;                                               \
      span.alpha += span.alphaStep;                                             \
      span.z += span.zStep;						                                \
   }									                                        \


#include "s_tritemp.h"

static void aros_triangle_noop(GLcontext *ctx, const SWvertex *v0,
                                 const SWvertex *v1,
                                 const SWvertex *v2 )
{
}

static void aros_nodraw_triangle( GLcontext *ctx, const SWvertex *v0, 
                const SWvertex *v1, const SWvertex *v2 )
{
   (void) (ctx && v0 && v1 && v2);
}

static void aros_choose_triangle(GLcontext * ctx)
{
    /* Functions listed below ARE NOT generic functions
     * They handle only a very limited set of most frequent
     * combination of states/attributes
     */

    SWcontext * swrast = SWRAST_CONTEXT(ctx);

    if (ctx->Polygon.CullFlag &&
        ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) 
    {
        swrast->Triangle = aros_nodraw_triangle;
        return;
    }

    if (ctx->RenderMode==GL_RENDER)
    {
        if (ctx->Texture._EnabledCoordUnits)
        {
            if (swrast->_RasterMask & DEPTH_BIT)
                swrast->Triangle = aros_simple_z_textured_triangle;
            else
                swrast->Triangle = aros_simple_textured_triangle;
        }
        else /* ctx->Texture._EnabledCoordUnits */
        {
            if (ctx->Light.ShadeModel==GL_SMOOTH) 
            {
                /* smooth shaded, no texturing, stippled or some raster ops */
                if (swrast->_RasterMask & DEPTH_BIT)
                    swrast->Triangle = aros_smooth_z_rgba_triangle;
                else
                    swrast->Triangle = aros_smooth_rgba_triangle;
            }
            else 
            {
                /* flat shaded, no texturing, stippled or some raster ops */
                if (swrast->_RasterMask & DEPTH_BIT)
                    swrast->Triangle = aros_flat_z_rgba_triangle;
                else
                    swrast->Triangle = aros_flat_rgba_triangle;
            }
        }
    }
    else /* ctx->RenderMode==GL_RENDER */
    {
        /* TODO implement other modes*/
        swrast->Triangle = aros_triangle_noop;
    }
}

#endif /* FAST_RASTERIZATION */

void aros_swrast_initialize(GLcontext * ctx)
{
#if FAST_RASTERIZATION == 1
    SWRAST_CONTEXT(ctx)->choose_triangle = aros_choose_triangle;
#endif
}
