#ifndef GRAPHICS_GFXMACROS_H
#define GRAPHICS_GFXMACROS_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
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

/* Shortcuts */
#define DrawCircle(rp,cx,cy,r)  DrawEllipse(rp,cx,cy,r,r);
#define AreaCircle(rp,cx,cy,r)  AreaEllipse(rp,cx,cy,r,r);

#endif /* GRAPHICS_GFXMACROS_H */
