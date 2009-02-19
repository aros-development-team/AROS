#ifndef GDIGFX_INTERN_H
#define GDIGFX_INTERN_H

/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id: gdigfx_intern.h 23833 2005-12-20 14:41:50Z stegerg $

    Desc: GDI gfx HIDD for AROS.
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

/****************************************************************************************/

/* Private Attrs and methods for the GDIGfx Hidd */

#define CLID_Hidd_GDIGfx	"hidd.gfx.gdi"
#define IID_Hidd_GDIGfx     	"hidd.gfx.gdigfx"

#define HiddGDIGfxAB  	    	__abHidd_GDIGfx

extern OOP_AttrBase HiddGDIGfxAB;

enum
{
    aoHidd_GDIGfx_SysDisplay,
/*  aoHidd_GDIGfx_SysScreen,
    aoHidd_GDIGfx_Hidd2GDICMap,
    aoHidd_GDIGfx_SysCursor,
    aoHidd_GDIGfx_ColorMap,*/
    
    num_Hidd_GDIGfx_Attrs
    
};

#define aHidd_GDIGfx_SysDisplay		(HiddGDIGfxAB + aoHidd_GDIGfx_SysDisplay)
/*
#define aHidd_GDIGfx_SysScreen		(HiddGDIGfxAB + aoHidd_GDIGfx_SysScreen)
#define aHidd_GDIGfx_Hidd2GDICMap	(HiddGDIGfxAB + aoHidd_GDIGfx_Hidd2GDICMap)
#define aHidd_GDIGfx_SysCursor		(HiddGDIGfxAB + aoHidd_GDIGfx_SysCursor)
#define aHidd_GDIGfx_ColorMap		(HiddGDIGfxAB + aoHidd_GDIGfx_ColorMap)
#define aHidd_GDIGfx_VisualClass	(HiddGDIGfxAB + aoHidd_GDIGfx_VisualClass) ** stegerg */

#define expunge() \
    AROS_LC0(BPTR, expunge, struct gdigfxbase *, LIBBASE, 3, GDIGfx)

#endif /* GDIGFX_INTERN_H */
