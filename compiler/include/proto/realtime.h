/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_REALTIME_H
#define PROTO_REALTIME_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/realtime_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/realtime.h>
#else
#include <defines/realtime.h>
#endif

#endif /* PROTO_REALTIME_H */
