/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_COLORWHEEL_H
#define PROTO_COLORWHEEL_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/colorwheel_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/colorwheel.h>
#else
#include <defines/colorwheel.h>
#endif

#endif /* PROTO_COLORWHEEL_H */
