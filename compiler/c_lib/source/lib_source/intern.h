#ifndef _INTERN_H
#define _INTERN_H

/*
**	$VER: intern.h 37.15 (14.8.97)
**
**	Common header file for all parts of the library.
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef LIBCORE_COMPILER_H
#   include <libcore/compiler.h>
#endif
#ifndef LIBCORE_BASE_H
#   include <libcore/base.h>
#endif
#ifndef LIBBASETYPE
#   include "libdefs.h"
#endif

struct ExampleBase
{
    struct LibHeader	   exb_LibHeader;
    struct IntuitionBase  *exb_IntuitionBase;
    struct GfxBase	  *exb_GfxBase;
};

extern ULONG SAVEDS STDARGS L_OpenLibs	(LIBBASETYPEPTR exb);
extern void  SAVEDS STDARGS L_CloseLibs (LIBBASETYPEPTR exb);

extern LIBBASETYPEPTR LIBBASE;

extern struct ExecBase	    *SysBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase	    *GfxBase;

extern const struct InitTable InitTab;
extern struct MyDataInit DataTab;

#endif /* _INTERN_H */
