#ifndef _INTERN_H
#define _INTERN_H

/*
**	$VER: LibInit.c 37.11 (24.6.97)
**
**	Common header file for all parts of the library.
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXAMPLE_EXAMPLEBASE_H
#   include <example/examplebase.h>
#endif
#ifndef LIBBASETYPE
#   include "libdefs.h"
#endif
#ifndef _COMPILER_H
#   include "compiler.h"
#endif

extern ULONG SAVEDS STDARGS L_OpenLibs(struct LIBBASETYPE *exb);
extern void  SAVEDS STDARGS L_CloseLibs(struct LIBBASETYPE *exb);

extern struct LIBBASETYPE *LIBBASE;

extern struct ExecBase	    *SysBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase	    *GfxBase;

extern const struct InitTable InitTab;
extern struct MyDataInit DataTab;

#endif /* _INTERN_H */
