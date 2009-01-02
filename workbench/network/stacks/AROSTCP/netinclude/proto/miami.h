#ifndef PROTO_MIAMI_H
#define PROTO_MIAMI_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <aros/system.h>

#include <clib/miami_protos.h>

#if !defined(MiamiBase) && !defined(__NOLIBBASE__) && !defined(__MIAMI_NOLIBBASE__)
 #ifdef __MIAMI_STDLIBBASE__
  extern struct Library *MiamiBase;
 #else
  extern struct Library *MiamiBase;
 #endif
#endif

#if !defined(NOLIBDEFINES) && !defined(MIAMI_NOLIBDEFINES)
#   include <defines/miami.h>
#endif

#endif /* PROTO_MIAMI_H */
