/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_TIMER_H
#define PROTO_TIMER_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/timer_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/timer.h>
#else
#include <defines/timer.h>
#endif

#endif /* PROTO_TIMER_H */
