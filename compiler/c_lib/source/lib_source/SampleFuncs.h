#ifndef _SAMPLEFUNCS_H
#define _SAMPLEFUNCS_H
/*
**	$VER: SampleFuncs.h 37.5 (24.1.97)
**
**	Demo functions for example.library
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/
#include "compiler.h"

struct ExampleBase; /* Pre-Declaration */

AROS_LP3 (ULONG, EXF_TestRequest,
    AROS_UFHA (UBYTE *, title_d1, D1),
    AROS_UFHA (UBYTE *, body, D2),
    AROS_UFHA (UBYTE *, gadgets, D3),
    struct ExampleBase *, ExampleBase, 5, Example
);

#endif /* _SAMPLEFUNCS_H */
