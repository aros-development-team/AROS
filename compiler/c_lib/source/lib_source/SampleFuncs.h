#ifndef _SAMPLEFUNCS_H
#define _SAMPLEFUNCS_H
/*
**	$VER: SampleFuncs.h 37.15 (14.8.97)
**
**	Demo functions for example.library
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/
#ifndef LIBCORE_COMPILER_H
#   include <libcore/compiler.h>
#endif
#ifndef _LIBDEFS_H
#   include "libdefs.h"
#endif

#ifndef EXAMPLE_EXAMPLEBASE_H
LIBBASETYPE; /* Pre-Declaration if necessary */
#endif

/* Declare functions for FuncTab[] */
AROS_LH3 (ULONG, EXF_TestRequest,
    AROS_LHA (UBYTE *, title_d1, D1),
    AROS_LHA (UBYTE *, body,     D2),
    AROS_LHA (UBYTE *, gadgets,  D3),
    LIBBASETYPEPTR, LIBBASE, 5, BASENAME
);

#endif /* _SAMPLEFUNCS_H */
