/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/


#ifndef AROSMESA_H
#define AROSMESA_H

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
 * This is the AROS/Mesa context structure.  This usually contains
 * info about what window/buffer we're rendering too, the current
 * drawing color, etc.
 */

struct arosmesa_context;

typedef struct arosmesa_context * AROSMesaContext;

/*
 * AROS Mesa Attribute tag ID's.  These are used in the ti_Tag field of
 * TagItem arrays passed to AROSMesaCreateContext()
 */
#define AMA_Dummy               (TAG_USER + 32)

/* Not handled */
/* #define AMA_Context             (AMA_Dummy + 0x0001) */

/*
Offset to use. WARNING AMA_Left, AMA_Bottom Specifies the low left corner
of the drawing area in deltapixles from the lowest left corner
typical AMA_Left,window->BorderLeft
        AMA_Bottom,window->BorderBottom
This is since ALL gl drawing actions is specified with this point as 0,0
and with y positive uppwards (like in real graphs).

Untuched (default) will result in 
AMA_Left=0;
AMA_Bottom=0;
*/
#define AMA_Left                (AMA_Dummy + 0x0002)
#define AMA_Right               (AMA_Dummy + 0x0003)
#define AMA_Top                 (AMA_Dummy + 0x0004)
#define AMA_Bottom              (AMA_Dummy + 0x0005)

/*
Size in pixels of drawing area if others than the whole rastport.
All internal drawingbuffers will be in this size

Untuched (default) will result in 
AMA_Width =rp->BitMap->BytesPerRow*8;
AMA_Height=rp->BitMap->Rows;
*/
#define AMA_Width               (AMA_Dummy + 0x0006)
#define AMA_Height              (AMA_Dummy + 0x0007)

/*
This may become unneaded, and code to autodetect the gfx-card should be added

AMA_DrawMode: Specifies the drawing hardware and should be one of
              AGA,(CYBERGFX,RETINA)
              default value: AGA
if AMESA_AGA AROS native drawigns
   this has to be filled with data
      AMA_Window = (ptr) Window to draw on
   or
      AMA_Screen =(ptr) Screen to draw on.
      AMA_RastPort =(ptr) RastPort to draw on.

if AMESA_AGA_C2P AROS native drawing using a chunky buffer
             thats converted when switching drawbuffer
             only works on doublebuffered drawings.
   this has to be filled with data
      AMA_DoubleBuf = GL_TRUE
      AMA_Window = (ptr) Window to draw on
   or
      AMA_DoubleBuf = GL_TRUE
      AMA_Screen =(ptr) Screen to draw on.
      AMA_RastPort =(ptr) RastPort to draw on.

else
   here should all needed gfx-card tagitem be specified
*/

/* Not handled */
/* enum DrawModeID                 {AMESA_AGA, AMESA_AGA_C2P, AMESA_CYBERGFX, AMESA_RETINA}; */
/* #define AMA_DrawMode            (AMA_Dummy + 0x0010) */
#define AMA_Screen              (AMA_Dummy + 0x0011)
#define AMA_Window              (AMA_Dummy + 0x0012)
#define AMA_RastPort            (AMA_Dummy + 0x0013)

/** booleans **/
/*
AMA_DoubleBuf: If specified it uses double Buffering (change buffer with
               AROSMesaSwapBuffers()) Turn this on as much as posible
               it will result in smother looking and faster rendering
               default value: GL_FALSE
AMA_RGBMode: If specified it uses 24bit when drawing (on non 24bit displays it
             it emuletes 24bit)
             default value: GL_TRUE
AMA_AlphaFlag: Alphachanel ?
               Defule value: GL_FALSE
AMA_DirectRender: if set to GL_TRUE in non-fullscreen-mode, the frame is rendered
		directly into the gfx RAM, the frame is then copied using the blitter.
		(not available for AGA)
AMA_NoDepth:    don't allocate ZBuffer if GL_TRUE
AMA_NoStencil:  don't allocate StencilBuffer if GL_TRUE
AMA_NoAccum:    don't allocate AccumulationBuffer if GL_TRUE
*/
#define AMA_DoubleBuf           (AMA_Dummy + 0x0030)    /* Not handled */ /* Always GL_TRUE */
#define AMA_RGBMode             (AMA_Dummy + 0x0031)    /* Not handled */ /* Always GL_TRUE */
#define AMA_AlphaFlag           (AMA_Dummy + 0x0032)    /* Not handled */ /* Always GL_TRUE */
/* Not handled */
/* #define AMA_DirectRender        (AMA_Dummy + 0x0035) */
#define AMA_NoDepth             (AMA_Dummy + 0x0039)
#define AMA_NoStencil           (AMA_Dummy + 0x003a)
#define AMA_NoAccum             (AMA_Dummy + 0x003b)

/** Special **/
/*
AMA_ShareGLContext: Set the "friend" context (use multiple contexts) 
                    See the GL maual or Mesa to get more info
AMA_Visual: If you want to implement your own arosmesa_visual 
AMA_Buffer: If you want to implement your own arosmesa_buffer
AMA_WindowID: A windowID to use when I alloc AMA_Buffer for you if
              you didn't supply one.(default=1)
*/

/* Not handled */
/* #define AMA_ShareGLContext      (AMA_Dummy + 0x0040) */
/* #define AMA_Visual              (AMA_Dummy + 0x0041) */
/* #define AMA_Buffer              (AMA_Dummy + 0x0042) */
/* #define AMA_WindowID            (AMA_Dummy + 0x0043) */

/**********************************************************************/
/*****                  AROS/Mesa API Functions                   *****/
/**********************************************************************/

typedef void (*AROSMesaProc)();

GLAPI AROSMesaContext       GLAPIENTRY AROSMesaCreateContextTags(long Tag1, ...);
GLAPI AROSMesaContext       GLAPIENTRY AROSMesaCreateContext(struct TagItem *tagList);
GLAPI void                  GLAPIENTRY AROSMesaDestroyContext(AROSMesaContext amesa);
GLAPI void                  GLAPIENTRY AROSMesaMakeCurrent(AROSMesaContext amesa);
GLAPI void                  GLAPIENTRY AROSMesaSwapBuffers(AROSMesaContext amesa);
GLAPI AROSMesaProc          GLAPIENTRY AROSMesaGetProcAddress(const GLubyte * procname);
GLAPI AROSMesaContext       GLAPIENTRY AROSMesaGetCurrentContext();

#ifdef __cplusplus
}
#endif

#endif /* AROSMESA_H */
