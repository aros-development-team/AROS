#ifndef _SAMPLEFUNCS_H
#define _SAMPLEFUNCS_H
/*
**	$VER: SampleFuncs.h 37.11 (24.6.97)
**
**	Demo functions for example.library
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/
#include "compiler.h"

#ifndef EXAMPLE_EXAMPLEBASE_H
struct ExampleBase; /* Pre-Declaration if necessary */
#endif

AROS_LP3 (ULONG, EXF_TestRequest,
    AROS_LHA (UBYTE *, title_d1, D1),
    AROS_LHA (UBYTE *, body, D2),
    AROS_LHA (UBYTE *, gadgets, D3),
    struct ExampleBase *, ExampleBase, 5, Example
);

#endif /* _SAMPLEFUNCS_H */
