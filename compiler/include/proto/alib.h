/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_ALIB_H
#define PROTO_ALIB_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/alib_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/alib.h>
#else
#include <defines/alib.h>
#endif

#endif /* PROTO_ALIB_H */
