/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_BATTMEM_H
#define PROTO_BATTMEM_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/battmem_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/battmem.h>
#else
#include <defines/battmem.h>
#endif

#endif /* PROTO_BATTMEM_H */
