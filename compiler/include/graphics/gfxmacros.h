#ifndef GRAPHICS_GFXMACROS_H
#define GRAPHICS_GFXMACROS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AmigaOS include file graphics/gfxmacros.h
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef  GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

/* NOTE: The following symbol must be defined in your source
         code if one needs the old style macros. Otherwise
	 one will get the new veresions, which are more robust */

#ifdef OLD_GRAPHICS_GFXMACROS_H
	 
/* Some macros which should be functions... */
#define SetDrPt(w,p) \
    { \
	(w)->LinePtrn = p; \
	(w)->Flags |= FRST_DOT|0x10; \
	(w)->linpatcnt = 15; \
    }

#define SetAfPt(w,p,n) \
    { \
	(w)->AreaPtrn = p; \
	(w)->AreaPtSz = n; \
    }

#define SetOPen(w,c) \
    { \
	(w)->AOlPen = c; \
	(w)->Flags |= AREAOUTLINE; \
    }

#define SetAOlPen(w,p)  SetOutlinePen(w,p)
#define SetWrMsk(w,m)   SetWriteMask(w,m)
#define BNDRYOFF(w)     {(w)->Flags &= ~AREAOUTLINE;}

/* Some macros for copper lists */
#define CINIT(c,n)      UCopperListInit(c,n);
#define CMOVE(c,a,b)    { CMove(c,&a,b); CBump(c); }
#define CWAIT(c,a,b)    { CWait(c,a,b); CBump(c); }
#define CEND(c)         { CWAIT(c,10000,255); }

/* Shortcuts */
#define DrawCircle(rp,cx,cy,r)  DrawEllipse(rp,cx,cy,r,r);
#define AreaCircle(rp,cx,cy,r)  AreaEllipse(rp,cx,cy,r,r);

#else /* OLD_GRAPHICS_GFXMACROS_H */

/* Some macros which should be functions... */
#define SetDrPt(w,p) \
    do { \
	(w)->LinePtrn = p; \
	(w)->Flags |= FRST_DOT|0x10; \
	(w)->linpatcnt = 15; \
    } while (0)

#define SetAfPt(w,p,n) \
    do { \
	(w)->AreaPtrn = p; \
	(w)->AreaPtSz = n; \
    } while (0)

#define SetOPen(w,c) \
    do { \
	(w)->AOlPen = c; \
	(w)->Flags |= AREAOUTLINE; \
    } while (0)

#define SetAOlPen(w,p)  SetOutlinePen(w,p)
#define SetWrMsk(w,m)   SetWriteMask(w,m)

#define BNDRYOFF(w) \
    do { \
       	(w)->Flags &= ~AREAOUTLINE; \
    } while (0)


/* Some macros for copper lists */
#define CINIT(c,n)      UCopperListInit(c,n)

#define CMOVE(c,a,b)    do { \
                            CMove(c,&a,b); \
                            CBump(c); \
                        } while (0)

#define CWAIT(c,a,b)    do { \
                            CWait(c,a,b); \
                            CBump(c); \
                        } while (0)

#define CEND(c)         do { \
                            CWAIT(c,10000,255); \
                        } while (0)

/* Shortcuts */
#define DrawCircle(rp,cx,cy,r)  DrawEllipse(rp,cx,cy,r,r);
#define AreaCircle(rp,cx,cy,r)  AreaEllipse(rp,cx,cy,r,r);

#endif /* OLD_GRAPHICS_GFXMACROS_H */

#endif /* GRAPHICS_GFXMACROS_H */
