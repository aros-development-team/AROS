#ifndef PROTO_GRAPHICS_H
#define PROTO_GRAPHICS_H

/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$
*/

#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

#include <clib/graphics_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#   include <inline/graphics.h>
#else
#   include <defines/graphics.h>
#endif

#endif /* PROTO_GRAPHICS_H */
