/*
    (C) 1995-97 AROS - The Amiga Research OS
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
