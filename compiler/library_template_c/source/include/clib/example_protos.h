/*
**      $VER: example_protos.h 37.1 (4.12.96)
**
**      prototypes for example.library
**
**      (C) Copyright 1996 Andreas R. Kleinert
**      All Rights Reserved.
*/

#ifndef CLIB_EXAMPLE_PROTOS_H
#define CLIB_EXAMPLE_PROTOS_H

#ifndef EXAMPLE_EXAMPLE_H
#include <example/example.h>
#endif /* EXAMPLE_EXAMPLE_H */

ULONG EXF_TestRequest( UBYTE *title, UBYTE *body, UBYTE *gadgets);

#endif /* CLIB_EXAMPLE_PROTOS_H */
