/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_INTUITION_H
#define PROTO_INTUITION_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef IntuitionBase
extern struct IntuitionBase * IntuitionBase;
#endif

#include <clib/intuition_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/intuition.h>
#else
#include <defines/intuition.h>
#endif

#if defined(ENABLE_RT) && ENABLE_RT && !defined(ENABLE_RT_INTUITION)
#   define ENABLE_RT_INTUITION	1
#   include <aros/rt.h>
#endif

#endif /* PROTO_INTUITION_H */
