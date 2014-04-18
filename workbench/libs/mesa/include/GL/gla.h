/*
    Copyright 2014, The AROS Development Team. All rights reserved.
    $Id$
*/


#ifndef GLA_H
#define GLA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <intuition/intuition.h>
#include <GL/gl.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

enum GLAError
{
   GLA_OUT_OF_MEM = 1,
   GLA_RASTPORT_TAG_MISSING,
   GLA_SCREEN_TAG_MISSING,
   GLA_WINDOW_TAG_MISSING
};

/*
 * This is the GLA context. This is an opaque pointer.
 * All information is hidden away from user.
 */


typedef APTR GLAContext;

/*
 * GLA Attribute tag ID's.  These are used in the ti_Tag field of
 * TagItem arrays passed to glACreateContext()
 */
#define GLA_Dummy               (TAG_USER + 32)

/*
 * Offsets from rastport layer dimensions. 
 * Typically GLA_Left = window->BorderLeft, GLA_Top = window->BorderTop
 * Defaults: if rendering to non-GZZ window BorderLeft, BorderTop, in all other
 *           cases 0,0
 */
#define GLA_Left                (GLA_Dummy + 0x0002)
#define GLA_Right               (GLA_Dummy + 0x0003)
#define GLA_Top                 (GLA_Dummy + 0x0004)
#define GLA_Bottom              (GLA_Dummy + 0x0005)

/*
 * Size in pixel of requested drawing area. Used to calculate GLA_Right, GLA_Bottom
 * if they are not passed. No defaults. Size of render area is calculated using:
 * rastport layer width - left - right, rastport layer height - top - bottom
 */
#define GLA_Width               (GLA_Dummy + 0x0006)
#define GLA_Height              (GLA_Dummy + 0x0007)

/*
 * Target to which rendering will be done. Currently always pass GLA_Window
 */
#define GLA_Screen              (GLA_Dummy + 0x0011)
#define GLA_Window              (GLA_Dummy + 0x0012)
#define GLA_RastPort            (GLA_Dummy + 0x0013)

/*
 * Following boolean flags are ignored and always considered GL_TRUE
 */
#define GLA_DoubleBuf           (GLA_Dummy + 0x0030)
#define GLA_RGBMode             (GLA_Dummy + 0x0031)
#define GLA_AlphaFlag           (GLA_Dummy + 0x0032)

/* 
 * GLA_NoDepth:    don't allocate ZBuffer if GL_TRUE, default is GL_FALSE
 * GLA_NoStencil:  don't allocate StencilBuffer if GL_TRUE, default is GL_FALSE
 * GLA_NoAccum:    don't allocate AccumulationBuffer if GL_TRUE, default is GL_FALSE
 */
#define GLA_NoDepth             (GLA_Dummy + 0x0039)
#define GLA_NoStencil           (GLA_Dummy + 0x003a)
#define GLA_NoAccum             (GLA_Dummy + 0x003b)

/*
 * GLA API calls
 */

typedef void (*GLAProc)();

GLAPI GLAContext            GLAPIENTRY glACreateContextTags(Tag Tag1, ...);
GLAPI GLAContext            GLAPIENTRY glACreateContext(struct TagItem *tagList);
GLAPI void                  GLAPIENTRY glADestroyContext(GLAContext ctx);
GLAPI void                  GLAPIENTRY glAMakeCurrent(GLAContext ctx);
GLAPI void                  GLAPIENTRY glASwapBuffers(GLAContext ctx);
GLAPI GLAProc               GLAPIENTRY glAGetProcAddress(const GLubyte * procname);
GLAPI GLAContext            GLAPIENTRY glAGetCurrentContext();
GLAPI void                  GLAPIENTRY glASetRast(GLAContext ctx, struct TagItem *tagList);
GLAPI void                  GLAPIENTRY glAGetConfig(GLAContext ctx, GLenum pname, GLint* params);

#ifdef __cplusplus
}
#endif

#endif /* GLA_H */
