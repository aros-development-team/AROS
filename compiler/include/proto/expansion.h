/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_EXPANSION_H
#define PROTO_EXPANSION_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/expansion_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/expansion.h>
#else
#include <defines/expansion.h>
#endif

#endif /* PROTO_EXPANSION_H */
