/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_POTGO_H
#define PROTO_POTGO_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/potgo_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/potgo.h>
#else
#include <defines/potgo.h>
#endif

#endif /* PROTO_POTGO_H */
