/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_DOS_H
#define PROTO_DOS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef DOSBase
extern struct DosLibrary * DOSBase;
#endif

#include <clib/dos_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/dos.h>
#else
#include <defines/dos.h>
#endif

#if defined(ENABLE_RT) && ENABLE_RT && !defined(ENABLE_RT_DOS)
#   define ENABLE_RT_DOS    1
#   include <aros/rt.h>
#endif

#endif /* PROTO_DOS_H */
