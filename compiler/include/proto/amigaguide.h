/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_AMIGAGUIDE_H
#define PROTO_AMIGAGUIDE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/amigaguide_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/amigaguide.h>
#else
#include <defines/amigaguide.h>
#endif

#endif /* PROTO_AMIGAGUIDE_H */
