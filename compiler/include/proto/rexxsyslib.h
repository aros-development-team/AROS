/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_REXXSYSLIB_H
#define PROTO_REXXSYSLIB_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/rexxsyslib_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/rexxsyslib.h>
#else
#include <defines/rexxsyslib.h>
#endif

#endif /* PROTO_REXXSYSLIB_H */
