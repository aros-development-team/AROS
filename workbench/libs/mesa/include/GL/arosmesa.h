/*
    Copyright 2009-2011, The AROS Development Team. All rights reserved.
    $Id$
*/


#ifndef AROSMESA_H
#define AROSMESA_H

/* General note: The interface AROSMesa is based on StormMesa. However, not
   all functions and attributes have been implemented. Some attributes also have
   different IDs than original StormMesa */

#ifdef __cplusplus
extern "C" {
#endif

#include <intuition/intuition.h>
#include <GL/gl.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

enum AMesaError
{
   AMESA_OUT_OF_MEM = 1,
   AMESA_RASTPORT_TAG_MISSING,
   AMESA_SCREEN_TAG_MISSING,
   AMESA_WINDOW_TAG_MISSING
};

/*
 * This is the AROS/Mesa context structure. This is an opaque pointer.
 * All information is hidden away from user.
 */

struct arosmesa_context;

typedef struct arosmesa_context * AROSMesaContext;

/*
 * AROS Mesa Attribute tag ID's.  These are used in the ti_Tag field of
 * TagItem arrays passed to AROSMesaCreateContext()
 */
#define AMA_Dummy               (TAG_USER + 32)

/*
 * Offsets from rastport layer dimensions. 
 * Typically AMA_Left = window->BorderLeft, AMA_Top = window->BorderTop
 * Defaults: if rendering to non-GZZ window BorderLeft, BorderTop, in all other
 *           cases 0,0
 */
#define AMA_Left                (AMA_Dummy + 0x0002)
#define AMA_Right               (AMA_Dummy + 0x0003)
#define AMA_Top                 (AMA_Dummy + 0x0004)
#define AMA_Bottom              (AMA_Dummy + 0x0005)

/*
 * Size in pixel of requested drawing area. Used to calculate AMA_Right, AMA_Bottom
 * if they are not passed. No defaults. Size of render area is calculated using:
 * rastport layer width - left - right, rastport layer height - top - bottom
 */
#define AMA_Width               (AMA_Dummy + 0x0006)
#define AMA_Height              (AMA_Dummy + 0x0007)

/*
 * Target to which rendering will be done. Currently always pass AMA_Window
 */
#define AMA_Screen              (AMA_Dummy + 0x0011)
#define AMA_Window              (AMA_Dummy + 0x0012)
#define AMA_RastPort            (AMA_Dummy + 0x0013)

/*
 * Following boolean flags are ignored and always considered GL_TRUE
 */
#define AMA_DoubleBuf           (AMA_Dummy + 0x0030)
#define AMA_RGBMode             (AMA_Dummy + 0x0031)
#define AMA_AlphaFlag           (AMA_Dummy + 0x0032)

/* 
 * AMA_NoDepth:    don't allocate ZBuffer if GL_TRUE, default is GL_FALSE
 * AMA_NoStencil:  don't allocate StencilBuffer if GL_TRUE, default is GL_FALSE
 * AMA_NoAccum:    don't allocate AccumulationBuffer if GL_TRUE, default is GL_FALSE
 */
#define AMA_NoDepth             (AMA_Dummy + 0x0039)
#define AMA_NoStencil           (AMA_Dummy + 0x003a)
#define AMA_NoAccum             (AMA_Dummy + 0x003b)

#define AMA_AROS_Extension      (AMA_Dummy + 0x0050)

/*
 * AROSMesa API calls
 */

typedef void (*AROSMesaProc)();

GLAPI AROSMesaContext       GLAPIENTRY AROSMesaCreateContextTags(long Tag1, ...);
GLAPI AROSMesaContext       GLAPIENTRY AROSMesaCreateContext(struct TagItem *tagList);
GLAPI void                  GLAPIENTRY AROSMesaDestroyContext(AROSMesaContext amesa);
GLAPI void                  GLAPIENTRY AROSMesaMakeCurrent(AROSMesaContext amesa);
GLAPI void                  GLAPIENTRY AROSMesaSwapBuffers(AROSMesaContext amesa);
GLAPI AROSMesaProc          GLAPIENTRY AROSMesaGetProcAddress(const GLubyte * procname);
GLAPI AROSMesaContext       GLAPIENTRY AROSMesaGetCurrentContext();
GLAPI void                  GLAPIENTRY AROSMesaSetRast(AROSMesaContext amesa, struct TagItem *tagList);

#ifdef __cplusplus
}
#endif

#endif /* AROSMESA_H */
