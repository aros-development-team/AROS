/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_MISC_H
#define PROTO_MISC_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/misc_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/misc.h>
#else
#include <defines/misc.h>
#endif

#endif /* PROTO_MISC_H */
