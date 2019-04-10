/* check that all macros are correctly set
 */
#ifndef GDST_INCR
#error "GDST_INCR not defined"
#endif

#ifndef GDST_TYPE
#error "GDST_TYPE not defined"
#endif

#ifndef GDST_READ
#error "GDST_READ not defined"
#endif

#ifdef GBLENDER_STORE_BYTES
#  ifndef GDST_STOREB
#    error "GDST_STOREB not defined"
#  endif
#else
#  ifndef GDST_STOREP
#    error "GDST_STOREP not defined"
#  endif
#endif /* !STORE_BYTES */

#ifndef GDST_STOREC
#error "GDST_STOREC not defined"
#endif

#ifndef GDST_COPY
#error "GDST_COPY not defined"
#endif

#ifndef GDST_COPY_VAR
#error  "GDST_COPY_VAR not defined"
#endif

#undef  GCONCAT
#undef  GCONCATX
#define GCONCAT(x,y)  GCONCATX(x,y)
#define GCONCATX(x,y)  x ## y


static void
GCONCAT( _gblender_blit_gray8_, GDST_TYPE )( GBlenderBlit   blit,
                                             GBlenderPixel  color )
{
  GBlender      blender = blit->blender;
  unsigned int  r       = (color >> 16) & 255;
  unsigned int  g       = (color >> 8)  & 255;
  unsigned int  b       = (color)       & 255;

  GDST_COPY_VAR

#include "gblcolor.h"
}


static void
GCONCAT( _gblender_blit_hrgb_, GDST_TYPE )( GBlenderBlit   blit,
                                            GBlenderPixel  color )
{
  GBlender      blender = blit->blender;
  unsigned int  r       = (color >> 16) & 255;
  unsigned int  g       = (color >> 8)  & 255;
  unsigned int  b       = (color)       & 255;

  GDST_COPY_VAR

#include "gblhrgb.h"
}


static void
GCONCAT( _gblender_blit_hbgr_, GDST_TYPE )( GBlenderBlit   blit,
                                            GBlenderPixel  color )
{
  GBlender      blender = blit->blender;
  unsigned int  r       = (color >> 16) & 255;
  unsigned int  g       = (color >> 8)  & 255;
  unsigned int  b       = (color)       & 255;

  GDST_COPY_VAR

#include "gblhbgr.h"
}


static void
GCONCAT( _gblender_blit_vrgb_, GDST_TYPE )( GBlenderBlit   blit,
                                            GBlenderPixel  color )
{
  GBlender      blender = blit->blender;
  unsigned int  r       = (color >> 16) & 255;
  unsigned int  g       = (color >> 8)  & 255;
  unsigned int  b       = (color)       & 255;

  GDST_COPY_VAR

#include "gblvrgb.h"
}


static void
GCONCAT( _gblender_blit_vbgr_, GDST_TYPE )( GBlenderBlit   blit,
                                            GBlenderPixel  color )
{
  GBlender      blender = blit->blender;
  unsigned int  r       = (color >> 16) & 255;
  unsigned int  g       = (color >> 8)  & 255;
  unsigned int  b       = (color)       & 255;

  GDST_COPY_VAR

#include "gblvbgr.h"
}


static void
GCONCAT( _gblender_blit_bgra_, GDST_TYPE )( GBlenderBlit   blit,
                                            GBlenderPixel  color )
{
  (void)color; /* unused */

#include "gblbgra.h"
}


/* unset the macros, to prevent accidental re-use
 */

#undef GCONCATX
#undef GCONCAT
#undef GDST_TYPE
#undef GDST_INCR
#undef GDST_READ
#undef GDST_COPY
#undef GDST_STOREB
#undef GDST_STOREP
#undef GDST_STOREC
#undef GDST_COPY_VAR

/* EOF */
