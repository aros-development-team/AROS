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
#ifndef EXAMPLE_EXAMPLEBASE_H
#   include <example/examplebase.h>
#endif
#ifndef _LIBDEFS_H
#   include "libdefs.h"
#endif

extern struct ExecBase	    *SysBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase	    *GfxBase;

#endif /* _INTERN_H */
