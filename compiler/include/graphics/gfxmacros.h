#ifndef GRAPHICS_GFXMACROS_H
#define GRAPHICS_GFXMACROS_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
#define SetDrPt(w,p)    {(w)->LinePtrn = p;(w)->Flags |= FRST_DOT;(w)->linpatcnt=15;}
#define SetAfPt(w,p,n)  {(w)->AreaPtrn = p;(w)->AreaPtSz = n;}
#define SetWrMsk(w,m)   {(w)->Mask = m;}
#define BNDRYOFF(w)     {(w)->Flags &= ~AREAOUTLINE;}

/* Compatibility macros for pre V39 */
#define SafeSetOutlinePen(w,c)  {if (GfxBase->LibNode.lib_Version<39) { (w)->AOlPen = c;(w)->Flags |= AREAOUTLINE;} else SetOutlinePen(w,c); }
#define SafeSetWriteMask(w,m)   {if (GfxBase->LibNode.lib_Version<39) { (w)->Mask = (m);} else SetWriteMask(w,m); }

/* Shortcuts */
#define DrawCircle(rp,cx,cy,r)  DrawEllipse(rp,cx,cy,r,r);
#define AreaCircle(rp,cx,cy,r)  AreaEllipse(rp,cx,cy,r,r);

#endif /* GRAPHICS_GFXMACROS_H */
