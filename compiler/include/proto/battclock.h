/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_BATTCLOCK_H
#define PROTO_BATTCLOCK_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/battclock_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/battclock.h>
#else
#include <defines/battclock.h>
#endif

#endif /* PROTO_BATTCLOCK_H */
